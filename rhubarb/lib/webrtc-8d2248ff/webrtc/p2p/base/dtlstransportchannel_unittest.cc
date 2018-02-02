/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <set>

#include "webrtc/p2p/base/dtlstransport.h"
#include "webrtc/p2p/base/faketransportcontroller.h"
#include "webrtc/base/common.h"
#include "webrtc/base/dscp.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/sslidentity.h"
#include "webrtc/base/sslstreamadapter.h"
#include "webrtc/base/stringutils.h"

#define MAYBE_SKIP_TEST(feature)                              \
  if (!(rtc::SSLStreamAdapter::feature())) {                  \
    LOG(LS_INFO) << #feature " feature disabled... skipping"; \
    return;                                                   \
  }

static const char kIceUfrag1[] = "TESTICEUFRAG0001";
static const char kIcePwd1[] = "TESTICEPWD00000000000001";
static const size_t kPacketNumOffset = 8;
static const size_t kPacketHeaderLen = 12;
static const int kFakePacketId = 0x1234;
static const int kTimeout = 10000;

static bool IsRtpLeadByte(uint8_t b) {
  return ((b & 0xC0) == 0x80);
}

cricket::TransportDescription MakeTransportDescription(
    const rtc::scoped_refptr<rtc::RTCCertificate>& cert,
    cricket::ConnectionRole role) {
  std::unique_ptr<rtc::SSLFingerprint> fingerprint;
  if (cert) {
    std::string digest_algorithm;
    cert->ssl_certificate().GetSignatureDigestAlgorithm(&digest_algorithm);
    fingerprint.reset(
        rtc::SSLFingerprint::Create(digest_algorithm, cert->identity()));
  }
  return cricket::TransportDescription(std::vector<std::string>(), kIceUfrag1,
                                       kIcePwd1, cricket::ICEMODE_FULL, role,
                                       fingerprint.get());
}

using cricket::ConnectionRole;

enum Flags { NF_REOFFER = 0x1, NF_EXPECT_FAILURE = 0x2 };

class DtlsTestClient : public sigslot::has_slots<> {
 public:
  DtlsTestClient(const std::string& name) : name_(name) {}
  void CreateCertificate(rtc::KeyType key_type) {
    certificate_ =
        rtc::RTCCertificate::Create(std::unique_ptr<rtc::SSLIdentity>(
            rtc::SSLIdentity::Generate(name_, key_type)));
  }
  const rtc::scoped_refptr<rtc::RTCCertificate>& certificate() {
    return certificate_;
  }
  void SetupSrtp() {
    ASSERT(certificate_);
    use_dtls_srtp_ = true;
  }
  void SetupMaxProtocolVersion(rtc::SSLProtocolVersion version) {
    ASSERT(!transport_);
    ssl_max_version_ = version;
  }
  void SetupChannels(int count, cricket::IceRole role) {
    transport_.reset(new cricket::DtlsTransport<cricket::FakeTransport>(
        "dtls content name", nullptr, certificate_));
    transport_->SetAsync(true);
    transport_->SetIceRole(role);
    transport_->SetIceTiebreaker(
        (role == cricket::ICEROLE_CONTROLLING) ? 1 : 2);

    for (int i = 0; i < count; ++i) {
      cricket::DtlsTransportChannelWrapper* channel =
          static_cast<cricket::DtlsTransportChannelWrapper*>(
              transport_->CreateChannel(i));
      ASSERT_TRUE(channel != NULL);
      channel->SetSslMaxProtocolVersion(ssl_max_version_);
      channel->SignalWritableState.connect(this,
        &DtlsTestClient::OnTransportChannelWritableState);
      channel->SignalReadPacket.connect(this,
        &DtlsTestClient::OnTransportChannelReadPacket);
      channel->SignalSentPacket.connect(
          this, &DtlsTestClient::OnTransportChannelSentPacket);
      channels_.push_back(channel);

      // Hook the raw packets so that we can verify they are encrypted.
      channel->channel()->SignalReadPacket.connect(
          this, &DtlsTestClient::OnFakeTransportChannelReadPacket);
    }
  }

  cricket::Transport* transport() { return transport_.get(); }

  cricket::FakeTransportChannel* GetFakeChannel(int component) {
    cricket::TransportChannelImpl* ch = transport_->GetChannel(component);
    cricket::DtlsTransportChannelWrapper* wrapper =
        static_cast<cricket::DtlsTransportChannelWrapper*>(ch);
    return (wrapper) ?
        static_cast<cricket::FakeTransportChannel*>(wrapper->channel()) : NULL;
  }

  // Offer DTLS if we have an identity; pass in a remote fingerprint only if
  // both sides support DTLS.
  void Negotiate(DtlsTestClient* peer, cricket::ContentAction action,
                 ConnectionRole local_role, ConnectionRole remote_role,
                 int flags) {
    Negotiate(certificate_, certificate_ ? peer->certificate_ : nullptr, action,
              local_role, remote_role, flags);
  }

  // Allow any DTLS configuration to be specified (including invalid ones).
  void Negotiate(const rtc::scoped_refptr<rtc::RTCCertificate>& local_cert,
                 const rtc::scoped_refptr<rtc::RTCCertificate>& remote_cert,
                 cricket::ContentAction action,
                 ConnectionRole local_role,
                 ConnectionRole remote_role,
                 int flags) {
    std::unique_ptr<rtc::SSLFingerprint> local_fingerprint;
    std::unique_ptr<rtc::SSLFingerprint> remote_fingerprint;
    if (local_cert) {
      std::string digest_algorithm;
      ASSERT_TRUE(local_cert->ssl_certificate().GetSignatureDigestAlgorithm(
          &digest_algorithm));
      ASSERT_FALSE(digest_algorithm.empty());
      local_fingerprint.reset(rtc::SSLFingerprint::Create(
          digest_algorithm, local_cert->identity()));
      ASSERT_TRUE(local_fingerprint.get() != NULL);
      EXPECT_EQ(rtc::DIGEST_SHA_256, digest_algorithm);
    }
    if (remote_cert) {
      std::string digest_algorithm;
      ASSERT_TRUE(remote_cert->ssl_certificate().GetSignatureDigestAlgorithm(
          &digest_algorithm));
      ASSERT_FALSE(digest_algorithm.empty());
      remote_fingerprint.reset(rtc::SSLFingerprint::Create(
          digest_algorithm, remote_cert->identity()));
      ASSERT_TRUE(remote_fingerprint.get() != NULL);
      EXPECT_EQ(rtc::DIGEST_SHA_256, digest_algorithm);
    }

    if (use_dtls_srtp_ && !(flags & NF_REOFFER)) {
      // SRTP ciphers will be set only in the beginning.
      for (std::vector<cricket::DtlsTransportChannelWrapper*>::iterator it =
           channels_.begin(); it != channels_.end(); ++it) {
        std::vector<int> ciphers;
        ciphers.push_back(rtc::SRTP_AES128_CM_SHA1_80);
        ASSERT_TRUE((*it)->SetSrtpCryptoSuites(ciphers));
      }
    }

    cricket::TransportDescription local_desc(
        std::vector<std::string>(), kIceUfrag1, kIcePwd1, cricket::ICEMODE_FULL,
        local_role,
        // If remote if the offerer and has no DTLS support, answer will be
        // without any fingerprint.
        (action == cricket::CA_ANSWER && !remote_cert)
            ? nullptr
            : local_fingerprint.get());

    cricket::TransportDescription remote_desc(
        std::vector<std::string>(), kIceUfrag1, kIcePwd1, cricket::ICEMODE_FULL,
        remote_role, remote_fingerprint.get());

    bool expect_success = (flags & NF_EXPECT_FAILURE) ? false : true;
    // If |expect_success| is false, expect SRTD or SLTD to fail when
    // content action is CA_ANSWER.
    if (action == cricket::CA_OFFER) {
      ASSERT_TRUE(transport_->SetLocalTransportDescription(
          local_desc, cricket::CA_OFFER, NULL));
      ASSERT_EQ(expect_success, transport_->SetRemoteTransportDescription(
          remote_desc, cricket::CA_ANSWER, NULL));
    } else {
      ASSERT_TRUE(transport_->SetRemoteTransportDescription(
          remote_desc, cricket::CA_OFFER, NULL));
      ASSERT_EQ(expect_success, transport_->SetLocalTransportDescription(
          local_desc, cricket::CA_ANSWER, NULL));
    }
    negotiated_dtls_ = (local_cert && remote_cert);
  }

  bool Connect(DtlsTestClient* peer, bool asymmetric) {
    transport_->SetDestination(peer->transport_.get(), asymmetric);
    return true;
  }

  bool all_channels_writable() const {
    if (channels_.empty()) {
      return false;
    }
    for (cricket::DtlsTransportChannelWrapper* channel : channels_) {
      if (!channel->writable()) {
        return false;
      }
    }
    return true;
  }

  bool all_raw_channels_writable() const {
    if (channels_.empty()) {
      return false;
    }
    for (cricket::DtlsTransportChannelWrapper* channel : channels_) {
      if (!channel->channel()->writable()) {
        return false;
      }
    }
    return true;
  }

  int received_dtls_client_hellos() const {
    return received_dtls_client_hellos_;
  }

  void CheckRole(rtc::SSLRole role) {
    if (role == rtc::SSL_CLIENT) {
      ASSERT_EQ(0, received_dtls_client_hellos_);
      ASSERT_GT(received_dtls_server_hellos_, 0);
    } else {
      ASSERT_GT(received_dtls_client_hellos_, 0);
      ASSERT_EQ(0, received_dtls_server_hellos_);
    }
  }

  void CheckSrtp(int expected_crypto_suite) {
    for (std::vector<cricket::DtlsTransportChannelWrapper*>::iterator it =
           channels_.begin(); it != channels_.end(); ++it) {
      int crypto_suite;

      bool rv = (*it)->GetSrtpCryptoSuite(&crypto_suite);
      if (negotiated_dtls_ && expected_crypto_suite) {
        ASSERT_TRUE(rv);

        ASSERT_EQ(crypto_suite, expected_crypto_suite);
      } else {
        ASSERT_FALSE(rv);
      }
    }
  }

  void CheckSsl() {
    for (std::vector<cricket::DtlsTransportChannelWrapper*>::iterator it =
           channels_.begin(); it != channels_.end(); ++it) {
      int cipher;

      bool rv = (*it)->GetSslCipherSuite(&cipher);
      if (negotiated_dtls_) {
        ASSERT_TRUE(rv);

        EXPECT_TRUE(
            rtc::SSLStreamAdapter::IsAcceptableCipher(cipher, rtc::KT_DEFAULT));
      } else {
        ASSERT_FALSE(rv);
      }
    }
  }

  void SendPackets(size_t channel, size_t size, size_t count, bool srtp) {
    ASSERT(channel < channels_.size());
    std::unique_ptr<char[]> packet(new char[size]);
    size_t sent = 0;
    do {
      // Fill the packet with a known value and a sequence number to check
      // against, and make sure that it doesn't look like DTLS.
      memset(packet.get(), sent & 0xff, size);
      packet[0] = (srtp) ? 0x80 : 0x00;
      rtc::SetBE32(packet.get() + kPacketNumOffset,
                   static_cast<uint32_t>(sent));

      // Only set the bypass flag if we've activated DTLS.
      int flags = (certificate_ && srtp) ? cricket::PF_SRTP_BYPASS : 0;
      rtc::PacketOptions packet_options;
      packet_options.packet_id = kFakePacketId;
      int rv = channels_[channel]->SendPacket(
          packet.get(), size, packet_options, flags);
      ASSERT_GT(rv, 0);
      ASSERT_EQ(size, static_cast<size_t>(rv));
      ++sent;
    } while (sent < count);
  }

  int SendInvalidSrtpPacket(size_t channel, size_t size) {
    ASSERT(channel < channels_.size());
    std::unique_ptr<char[]> packet(new char[size]);
    // Fill the packet with 0 to form an invalid SRTP packet.
    memset(packet.get(), 0, size);

    rtc::PacketOptions packet_options;
    return channels_[channel]->SendPacket(
        packet.get(), size, packet_options, cricket::PF_SRTP_BYPASS);
  }

  void ExpectPackets(size_t channel, size_t size) {
    packet_size_ = size;
    received_.clear();
  }

  size_t NumPacketsReceived() {
    return received_.size();
  }

  bool VerifyPacket(const char* data, size_t size, uint32_t* out_num) {
    if (size != packet_size_ ||
        (data[0] != 0 && static_cast<uint8_t>(data[0]) != 0x80)) {
      return false;
    }
    uint32_t packet_num = rtc::GetBE32(data + kPacketNumOffset);
    for (size_t i = kPacketHeaderLen; i < size; ++i) {
      if (static_cast<uint8_t>(data[i]) != (packet_num & 0xff)) {
        return false;
      }
    }
    if (out_num) {
      *out_num = packet_num;
    }
    return true;
  }
  bool VerifyEncryptedPacket(const char* data, size_t size) {
    // This is an encrypted data packet; let's make sure it's mostly random;
    // less than 10% of the bytes should be equal to the cleartext packet.
    if (size <= packet_size_) {
      return false;
    }
    uint32_t packet_num = rtc::GetBE32(data + kPacketNumOffset);
    int num_matches = 0;
    for (size_t i = kPacketNumOffset; i < size; ++i) {
      if (static_cast<uint8_t>(data[i]) == (packet_num & 0xff)) {
        ++num_matches;
      }
    }
    return (num_matches < ((static_cast<int>(size) - 5) / 10));
  }

  // Transport channel callbacks
  void OnTransportChannelWritableState(cricket::TransportChannel* channel) {
    LOG(LS_INFO) << name_ << ": Channel '" << channel->component()
                 << "' is writable";
  }

  void OnTransportChannelReadPacket(cricket::TransportChannel* channel,
                                    const char* data, size_t size,
                                    const rtc::PacketTime& packet_time,
                                    int flags) {
    uint32_t packet_num = 0;
    ASSERT_TRUE(VerifyPacket(data, size, &packet_num));
    received_.insert(packet_num);
    // Only DTLS-SRTP packets should have the bypass flag set.
    int expected_flags =
        (certificate_ && IsRtpLeadByte(data[0])) ? cricket::PF_SRTP_BYPASS : 0;
    ASSERT_EQ(expected_flags, flags);
  }

  void OnTransportChannelSentPacket(cricket::TransportChannel* channel,
                                    const rtc::SentPacket& sent_packet) {
    sent_packet_ = sent_packet;
  }

  rtc::SentPacket sent_packet() const { return sent_packet_; }

  // Hook into the raw packet stream to make sure DTLS packets are encrypted.
  void OnFakeTransportChannelReadPacket(cricket::TransportChannel* channel,
                                        const char* data, size_t size,
                                        const rtc::PacketTime& time,
                                        int flags) {
    // Flags shouldn't be set on the underlying TransportChannel packets.
    ASSERT_EQ(0, flags);

    // Look at the handshake packets to see what role we played.
    // Check that non-handshake packets are DTLS data or SRTP bypass.
    if (data[0] == 22 && size > 17) {
      if (data[13] == 1) {
        ++received_dtls_client_hellos_;
      } else if (data[13] == 2) {
        ++received_dtls_server_hellos_;
      }
    } else if (negotiated_dtls_ && !(data[0] >= 20 && data[0] <= 22)) {
      ASSERT_TRUE(data[0] == 23 || IsRtpLeadByte(data[0]));
      if (data[0] == 23) {
        ASSERT_TRUE(VerifyEncryptedPacket(data, size));
      } else if (IsRtpLeadByte(data[0])) {
        ASSERT_TRUE(VerifyPacket(data, size, NULL));
      }
    }
  }

 private:
  std::string name_;
  rtc::scoped_refptr<rtc::RTCCertificate> certificate_;
  std::unique_ptr<cricket::FakeTransport> transport_;
  std::vector<cricket::DtlsTransportChannelWrapper*> channels_;
  size_t packet_size_ = 0u;
  std::set<int> received_;
  bool use_dtls_srtp_ = false;
  rtc::SSLProtocolVersion ssl_max_version_ = rtc::SSL_PROTOCOL_DTLS_12;
  bool negotiated_dtls_ = false;
  int received_dtls_client_hellos_ = 0;
  int received_dtls_server_hellos_ = 0;
  rtc::SentPacket sent_packet_;
};

// Note that this test always uses a FakeClock, due to the |fake_clock_| member
// variable.
class DtlsTransportChannelTest : public testing::Test {
 public:
  DtlsTransportChannelTest()
      : client1_("P1"),
        client2_("P2"),
        channel_ct_(1),
        use_dtls_(false),
        use_dtls_srtp_(false),
        ssl_expected_version_(rtc::SSL_PROTOCOL_DTLS_12) {}

  void SetChannelCount(size_t channel_ct) {
    channel_ct_ = static_cast<int>(channel_ct);
  }
  void SetMaxProtocolVersions(rtc::SSLProtocolVersion c1,
                              rtc::SSLProtocolVersion c2) {
    client1_.SetupMaxProtocolVersion(c1);
    client2_.SetupMaxProtocolVersion(c2);
    ssl_expected_version_ = std::min(c1, c2);
  }
  void PrepareDtls(bool c1, bool c2, rtc::KeyType key_type) {
    if (c1) {
      client1_.CreateCertificate(key_type);
    }
    if (c2) {
      client2_.CreateCertificate(key_type);
    }
    if (c1 && c2)
      use_dtls_ = true;
  }
  void PrepareDtlsSrtp(bool c1, bool c2) {
    if (!use_dtls_)
      return;

    if (c1)
      client1_.SetupSrtp();
    if (c2)
      client2_.SetupSrtp();

    if (c1 && c2)
      use_dtls_srtp_ = true;
  }

  bool Connect(ConnectionRole client1_role, ConnectionRole client2_role) {
    Negotiate(client1_role, client2_role);

    bool rv = client1_.Connect(&client2_, false);
    EXPECT_TRUE(rv);
    if (!rv)
      return false;

    EXPECT_TRUE_WAIT(
        client1_.all_channels_writable() && client2_.all_channels_writable(),
        kTimeout);
    if (!client1_.all_channels_writable() || !client2_.all_channels_writable())
      return false;

    // Check that we used the right roles.
    if (use_dtls_) {
      rtc::SSLRole client1_ssl_role =
          (client1_role == cricket::CONNECTIONROLE_ACTIVE ||
           (client2_role == cricket::CONNECTIONROLE_PASSIVE &&
            client1_role == cricket::CONNECTIONROLE_ACTPASS)) ?
              rtc::SSL_CLIENT : rtc::SSL_SERVER;

      rtc::SSLRole client2_ssl_role =
          (client2_role == cricket::CONNECTIONROLE_ACTIVE ||
           (client1_role == cricket::CONNECTIONROLE_PASSIVE &&
            client2_role == cricket::CONNECTIONROLE_ACTPASS)) ?
              rtc::SSL_CLIENT : rtc::SSL_SERVER;

      client1_.CheckRole(client1_ssl_role);
      client2_.CheckRole(client2_ssl_role);
    }

    // Check that we negotiated the right ciphers.
    if (use_dtls_srtp_) {
      client1_.CheckSrtp(rtc::SRTP_AES128_CM_SHA1_80);
      client2_.CheckSrtp(rtc::SRTP_AES128_CM_SHA1_80);
    } else {
      client1_.CheckSrtp(rtc::SRTP_INVALID_CRYPTO_SUITE);
      client2_.CheckSrtp(rtc::SRTP_INVALID_CRYPTO_SUITE);
    }

    client1_.CheckSsl();
    client2_.CheckSsl();

    return true;
  }

  bool Connect() {
    // By default, Client1 will be Server and Client2 will be Client.
    return Connect(cricket::CONNECTIONROLE_ACTPASS,
                   cricket::CONNECTIONROLE_ACTIVE);
  }

  void Negotiate() {
    Negotiate(cricket::CONNECTIONROLE_ACTPASS, cricket::CONNECTIONROLE_ACTIVE);
  }

  void Negotiate(ConnectionRole client1_role, ConnectionRole client2_role) {
    client1_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLING);
    client2_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLED);
    // Expect success from SLTD and SRTD.
    client1_.Negotiate(&client2_, cricket::CA_OFFER,
                       client1_role, client2_role, 0);
    client2_.Negotiate(&client1_, cricket::CA_ANSWER,
                       client2_role, client1_role, 0);
  }

  // Negotiate with legacy client |client2|. Legacy client doesn't use setup
  // attributes, except NONE.
  void NegotiateWithLegacy() {
    client1_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLING);
    client2_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLED);
    // Expect success from SLTD and SRTD.
    client1_.Negotiate(&client2_, cricket::CA_OFFER,
                       cricket::CONNECTIONROLE_ACTPASS,
                       cricket::CONNECTIONROLE_NONE, 0);
    client2_.Negotiate(&client1_, cricket::CA_ANSWER,
                       cricket::CONNECTIONROLE_ACTIVE,
                       cricket::CONNECTIONROLE_NONE, 0);
  }

  void Renegotiate(DtlsTestClient* reoffer_initiator,
                   ConnectionRole client1_role, ConnectionRole client2_role,
                   int flags) {
    if (reoffer_initiator == &client1_) {
      client1_.Negotiate(&client2_, cricket::CA_OFFER,
                         client1_role, client2_role, flags);
      client2_.Negotiate(&client1_, cricket::CA_ANSWER,
                         client2_role, client1_role, flags);
    } else {
      client2_.Negotiate(&client1_, cricket::CA_OFFER,
                         client2_role, client1_role, flags);
      client1_.Negotiate(&client2_, cricket::CA_ANSWER,
                         client1_role, client2_role, flags);
    }
  }

  void TestTransfer(size_t channel, size_t size, size_t count, bool srtp) {
    LOG(LS_INFO) << "Expect packets, size=" << size;
    client2_.ExpectPackets(channel, size);
    client1_.SendPackets(channel, size, count, srtp);
    EXPECT_EQ_WAIT(count, client2_.NumPacketsReceived(), kTimeout);
  }

 protected:
  rtc::ScopedFakeClock fake_clock_;
  DtlsTestClient client1_;
  DtlsTestClient client2_;
  int channel_ct_;
  bool use_dtls_;
  bool use_dtls_srtp_;
  rtc::SSLProtocolVersion ssl_expected_version_;
};

// Test that transport negotiation of ICE, no DTLS works properly.
TEST_F(DtlsTransportChannelTest, TestChannelSetupIce) {
  Negotiate();
  cricket::FakeTransportChannel* channel1 = client1_.GetFakeChannel(0);
  cricket::FakeTransportChannel* channel2 = client2_.GetFakeChannel(0);
  ASSERT_TRUE(channel1 != NULL);
  ASSERT_TRUE(channel2 != NULL);
  EXPECT_EQ(cricket::ICEROLE_CONTROLLING, channel1->GetIceRole());
  EXPECT_EQ(1U, channel1->IceTiebreaker());
  EXPECT_EQ(kIceUfrag1, channel1->ice_ufrag());
  EXPECT_EQ(kIcePwd1, channel1->ice_pwd());
  EXPECT_EQ(cricket::ICEROLE_CONTROLLED, channel2->GetIceRole());
  EXPECT_EQ(2U, channel2->IceTiebreaker());
}

// Connect without DTLS, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransfer) {
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
}

// Connect without DTLS, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestOnSentPacket) {
  ASSERT_TRUE(Connect());
  EXPECT_EQ(client1_.sent_packet().send_time_ms, -1);
  TestTransfer(0, 1000, 100, false);
  EXPECT_EQ(kFakePacketId, client1_.sent_packet().packet_id);
  EXPECT_GE(client1_.sent_packet().send_time_ms, 0);
}

// Create two channels without DTLS, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransferTwoChannels) {
  SetChannelCount(2);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
  TestTransfer(1, 1000, 100, false);
}

// Connect without DTLS, and transfer SRTP data.
TEST_F(DtlsTransportChannelTest, TestTransferSrtp) {
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, true);
}

// Create two channels without DTLS, and transfer SRTP data.
TEST_F(DtlsTransportChannelTest, TestTransferSrtpTwoChannels) {
  SetChannelCount(2);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Connect with DTLS, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransferDtls) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
}

// Create two channels with DTLS, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsTwoChannels) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
  TestTransfer(1, 1000, 100, false);
}

// Connect with A doing DTLS and B not, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsRejected) {
  PrepareDtls(true, false, rtc::KT_DEFAULT);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
}

// Connect with B doing DTLS and A not, and transfer some data.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsNotOffered) {
  PrepareDtls(false, true, rtc::KT_DEFAULT);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
}

// Create two channels with DTLS 1.0 and check ciphers.
TEST_F(DtlsTransportChannelTest, TestDtls12None) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  SetMaxProtocolVersions(rtc::SSL_PROTOCOL_DTLS_10, rtc::SSL_PROTOCOL_DTLS_10);
  ASSERT_TRUE(Connect());
}

// Create two channels with DTLS 1.2 and check ciphers.
TEST_F(DtlsTransportChannelTest, TestDtls12Both) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  SetMaxProtocolVersions(rtc::SSL_PROTOCOL_DTLS_12, rtc::SSL_PROTOCOL_DTLS_12);
  ASSERT_TRUE(Connect());
}

// Create two channels with DTLS 1.0 / DTLS 1.2 and check ciphers.
TEST_F(DtlsTransportChannelTest, TestDtls12Client1) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  SetMaxProtocolVersions(rtc::SSL_PROTOCOL_DTLS_12, rtc::SSL_PROTOCOL_DTLS_10);
  ASSERT_TRUE(Connect());
}

// Create two channels with DTLS 1.2 / DTLS 1.0 and check ciphers.
TEST_F(DtlsTransportChannelTest, TestDtls12Client2) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  SetMaxProtocolVersions(rtc::SSL_PROTOCOL_DTLS_10, rtc::SSL_PROTOCOL_DTLS_12);
  ASSERT_TRUE(Connect());
}

// Connect with DTLS, negotiate DTLS-SRTP, and transfer SRTP using bypass.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsSrtp) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, true);
}

// Connect with DTLS-SRTP, transfer an invalid SRTP packet, and expects -1
// returned.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsInvalidSrtpPacket) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect());
  int result = client1_.SendInvalidSrtpPacket(0, 100);
  ASSERT_EQ(-1, result);
}

// Connect with DTLS. A does DTLS-SRTP but B does not.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsSrtpRejected) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, false);
  ASSERT_TRUE(Connect());
}

// Connect with DTLS. B does DTLS-SRTP but A does not.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsSrtpNotOffered) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(false, true);
  ASSERT_TRUE(Connect());
}

// Create two channels with DTLS, negotiate DTLS-SRTP, and transfer bypass SRTP.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsSrtpTwoChannels) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Create a single channel with DTLS, and send normal data and SRTP data on it.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsSrtpDemux) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect());
  TestTransfer(0, 1000, 100, false);
  TestTransfer(0, 1000, 100, true);
}

// Testing when the remote is passive.
TEST_F(DtlsTransportChannelTest, TestTransferDtlsAnswererIsPassive) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect(cricket::CONNECTIONROLE_ACTPASS,
                      cricket::CONNECTIONROLE_PASSIVE));
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Testing with the legacy DTLS client which doesn't use setup attribute.
// In this case legacy is the answerer.
TEST_F(DtlsTransportChannelTest, TestDtlsSetupWithLegacyAsAnswerer) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  NegotiateWithLegacy();
  rtc::SSLRole channel1_role;
  rtc::SSLRole channel2_role;
  EXPECT_TRUE(client1_.transport()->GetSslRole(&channel1_role));
  EXPECT_TRUE(client2_.transport()->GetSslRole(&channel2_role));
  EXPECT_EQ(rtc::SSL_SERVER, channel1_role);
  EXPECT_EQ(rtc::SSL_CLIENT, channel2_role);
}

// Testing re offer/answer after the session is estbalished. Roles will be
// kept same as of the previous negotiation.
TEST_F(DtlsTransportChannelTest, TestDtlsReOfferFromOfferer) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  // Initial role for client1 is ACTPASS and client2 is ACTIVE.
  ASSERT_TRUE(Connect(cricket::CONNECTIONROLE_ACTPASS,
                      cricket::CONNECTIONROLE_ACTIVE));
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
  // Using input roles for the re-offer.
  Renegotiate(&client1_, cricket::CONNECTIONROLE_ACTPASS,
              cricket::CONNECTIONROLE_ACTIVE, NF_REOFFER);
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

TEST_F(DtlsTransportChannelTest, TestDtlsReOfferFromAnswerer) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  // Initial role for client1 is ACTPASS and client2 is ACTIVE.
  ASSERT_TRUE(Connect(cricket::CONNECTIONROLE_ACTPASS,
                      cricket::CONNECTIONROLE_ACTIVE));
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
  // Using input roles for the re-offer.
  Renegotiate(&client2_, cricket::CONNECTIONROLE_PASSIVE,
              cricket::CONNECTIONROLE_ACTPASS, NF_REOFFER);
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Test that any change in role after the intial setup will result in failure.
TEST_F(DtlsTransportChannelTest, TestDtlsRoleReversal) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect(cricket::CONNECTIONROLE_ACTPASS,
                      cricket::CONNECTIONROLE_PASSIVE));

  // Renegotiate from client2 with actpass and client1 as active.
  Renegotiate(&client2_, cricket::CONNECTIONROLE_ACTPASS,
              cricket::CONNECTIONROLE_ACTIVE,
              NF_REOFFER | NF_EXPECT_FAILURE);
}

// Test that using different setup attributes which results in similar ssl
// role as the initial negotiation will result in success.
TEST_F(DtlsTransportChannelTest, TestDtlsReOfferWithDifferentSetupAttr) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  ASSERT_TRUE(Connect(cricket::CONNECTIONROLE_ACTPASS,
                      cricket::CONNECTIONROLE_PASSIVE));
  // Renegotiate from client2 with actpass and client1 as active.
  Renegotiate(&client2_, cricket::CONNECTIONROLE_ACTIVE,
              cricket::CONNECTIONROLE_ACTPASS, NF_REOFFER);
  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Test that re-negotiation can be started before the clients become connected
// in the first negotiation.
TEST_F(DtlsTransportChannelTest, TestRenegotiateBeforeConnect) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  SetChannelCount(2);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  PrepareDtlsSrtp(true, true);
  Negotiate();

  Renegotiate(&client1_, cricket::CONNECTIONROLE_ACTPASS,
              cricket::CONNECTIONROLE_ACTIVE, NF_REOFFER);
  bool rv = client1_.Connect(&client2_, false);
  EXPECT_TRUE(rv);
  EXPECT_TRUE_WAIT(
      client1_.all_channels_writable() && client2_.all_channels_writable(),
      kTimeout);

  TestTransfer(0, 1000, 100, true);
  TestTransfer(1, 1000, 100, true);
}

// Test Certificates state after negotiation but before connection.
TEST_F(DtlsTransportChannelTest, TestCertificatesBeforeConnect) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  Negotiate();

  rtc::scoped_refptr<rtc::RTCCertificate> certificate1;
  rtc::scoped_refptr<rtc::RTCCertificate> certificate2;
  std::unique_ptr<rtc::SSLCertificate> remote_cert1;
  std::unique_ptr<rtc::SSLCertificate> remote_cert2;

  // After negotiation, each side has a distinct local certificate, but still no
  // remote certificate, because connection has not yet occurred.
  ASSERT_TRUE(client1_.transport()->GetLocalCertificate(&certificate1));
  ASSERT_TRUE(client2_.transport()->GetLocalCertificate(&certificate2));
  ASSERT_NE(certificate1->ssl_certificate().ToPEMString(),
            certificate2->ssl_certificate().ToPEMString());
  ASSERT_FALSE(client1_.transport()->GetRemoteSSLCertificate());
  ASSERT_FALSE(client2_.transport()->GetRemoteSSLCertificate());
}

// Test Certificates state after connection.
TEST_F(DtlsTransportChannelTest, TestCertificatesAfterConnect) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  ASSERT_TRUE(Connect());

  rtc::scoped_refptr<rtc::RTCCertificate> certificate1;
  rtc::scoped_refptr<rtc::RTCCertificate> certificate2;

  // After connection, each side has a distinct local certificate.
  ASSERT_TRUE(client1_.transport()->GetLocalCertificate(&certificate1));
  ASSERT_TRUE(client2_.transport()->GetLocalCertificate(&certificate2));
  ASSERT_NE(certificate1->ssl_certificate().ToPEMString(),
            certificate2->ssl_certificate().ToPEMString());

  // Each side's remote certificate is the other side's local certificate.
  std::unique_ptr<rtc::SSLCertificate> remote_cert1 =
      client1_.transport()->GetRemoteSSLCertificate();
  ASSERT_TRUE(remote_cert1);
  ASSERT_EQ(remote_cert1->ToPEMString(),
            certificate2->ssl_certificate().ToPEMString());
  std::unique_ptr<rtc::SSLCertificate> remote_cert2 =
      client2_.transport()->GetRemoteSSLCertificate();
  ASSERT_TRUE(remote_cert2);
  ASSERT_EQ(remote_cert2->ToPEMString(),
            certificate1->ssl_certificate().ToPEMString());
}

// Test that DTLS completes promptly if a ClientHello is received before the
// transport channel is writable (allowing a ServerHello to be sent).
TEST_F(DtlsTransportChannelTest, TestReceiveClientHelloBeforeWritable) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  // Exchange transport descriptions.
  Negotiate(cricket::CONNECTIONROLE_ACTPASS, cricket::CONNECTIONROLE_ACTIVE);

  // Make client2_ writable, but not client1_.
  EXPECT_TRUE(client2_.Connect(&client1_, true));
  EXPECT_TRUE_WAIT(client2_.all_raw_channels_writable(), kTimeout);

  // Expect a DTLS ClientHello to be sent even while client1_ isn't writable.
  EXPECT_EQ_WAIT(1, client1_.received_dtls_client_hellos(), kTimeout);
  EXPECT_FALSE(client1_.all_raw_channels_writable());

  // Now make client1_ writable and expect the handshake to complete
  // without client2_ needing to retransmit the ClientHello.
  EXPECT_TRUE(client1_.Connect(&client2_, true));
  EXPECT_TRUE_WAIT(
      client1_.all_channels_writable() && client2_.all_channels_writable(),
      kTimeout);
  EXPECT_EQ(1, client1_.received_dtls_client_hellos());
}

// Test that DTLS completes promptly if a ClientHello is received before the
// transport channel has a remote fingerprint (allowing a ServerHello to be
// sent).
TEST_F(DtlsTransportChannelTest,
       TestReceiveClientHelloBeforeRemoteFingerprint) {
  MAYBE_SKIP_TEST(HaveDtls);
  PrepareDtls(true, true, rtc::KT_DEFAULT);
  client1_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLING);
  client2_.SetupChannels(channel_ct_, cricket::ICEROLE_CONTROLLED);

  // Make client2_ writable and give it local/remote certs, but don't yet give
  // client1_ a remote fingerprint.
  client1_.transport()->SetLocalTransportDescription(
      MakeTransportDescription(client1_.certificate(),
                               cricket::CONNECTIONROLE_ACTPASS),
      cricket::CA_OFFER, nullptr);
  client2_.Negotiate(&client1_, cricket::CA_ANSWER,
                     cricket::CONNECTIONROLE_ACTIVE,
                     cricket::CONNECTIONROLE_ACTPASS, 0);
  EXPECT_TRUE(client2_.Connect(&client1_, true));
  EXPECT_TRUE_WAIT(client2_.all_raw_channels_writable(), kTimeout);

  // Expect a DTLS ClientHello to be sent even while client1_ doesn't have a
  // remote fingerprint.
  EXPECT_EQ_WAIT(1, client1_.received_dtls_client_hellos(), kTimeout);
  EXPECT_FALSE(client1_.all_raw_channels_writable());

  // Now make give client1_ its remote fingerprint and make it writable, and
  // expect the handshake to complete without client2_ needing to retransmit
  // the ClientHello.
  client1_.transport()->SetRemoteTransportDescription(
      MakeTransportDescription(client2_.certificate(),
                               cricket::CONNECTIONROLE_ACTIVE),
      cricket::CA_ANSWER, nullptr);
  EXPECT_TRUE(client1_.Connect(&client2_, true));
  EXPECT_TRUE_WAIT(
      client1_.all_channels_writable() && client2_.all_channels_writable(),
      kTimeout);
  EXPECT_EQ(1, client1_.received_dtls_client_hellos());
}

// Test that packets are retransmitted according to the expected schedule.
// Each time a timeout occurs, the retransmission timer should be doubled up to
// 60 seconds. The timer defaults to 1 second, but for WebRTC we should be
// initializing it to 50ms.
TEST_F(DtlsTransportChannelTest, TestRetransmissionSchedule) {
  MAYBE_SKIP_TEST(HaveDtls);
  // We can only change the retransmission schedule with a recently-added
  // BoringSSL API. Skip the test if not built with BoringSSL.
  MAYBE_SKIP_TEST(IsBoringSsl);

  PrepareDtls(true, true, rtc::KT_DEFAULT);
  // Exchange transport descriptions.
  Negotiate(cricket::CONNECTIONROLE_ACTPASS, cricket::CONNECTIONROLE_ACTIVE);

  // Make client2_ writable, but not client1_.
  // This means client1_ will send DTLS client hellos but get no response.
  EXPECT_TRUE(client2_.Connect(&client1_, true));
  EXPECT_TRUE_WAIT(client2_.all_raw_channels_writable(), kTimeout);

  // Wait for the first client hello to be sent.
  EXPECT_EQ_WAIT(1, client1_.received_dtls_client_hellos(), kTimeout);
  EXPECT_FALSE(client1_.all_raw_channels_writable());

  static int timeout_schedule_ms[] = {50,   100,  200,   400,   800,   1600,
                                      3200, 6400, 12800, 25600, 51200, 60000};

  int expected_hellos = 1;
  for (size_t i = 0;
       i < (sizeof(timeout_schedule_ms) / sizeof(timeout_schedule_ms[0]));
       ++i) {
    // For each expected retransmission time, advance the fake clock a
    // millisecond before the expected time and verify that no unexpected
    // retransmissions were sent. Then advance it the final millisecond and
    // verify that the expected retransmission was sent.
    fake_clock_.AdvanceTime(
        rtc::TimeDelta::FromMilliseconds(timeout_schedule_ms[i] - 1));
    EXPECT_EQ(expected_hellos, client1_.received_dtls_client_hellos());
    fake_clock_.AdvanceTime(rtc::TimeDelta::FromMilliseconds(1));
    EXPECT_EQ(++expected_hellos, client1_.received_dtls_client_hellos());
  }
}
