/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <string>
#include <utility>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/peerconnectionfactory.h"
#ifdef WEBRTC_ANDROID
#include "webrtc/api/test/androidtestinitializer.h"
#endif
#include "webrtc/api/test/fakertccertificategenerator.h"
#include "webrtc/api/test/fakevideotrackrenderer.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/thread.h"
#include "webrtc/media/base/fakevideocapturer.h"
#include "webrtc/media/engine/webrtccommon.h"
#include "webrtc/media/engine/webrtcvoe.h"
#include "webrtc/p2p/base/fakeportallocator.h"

using webrtc::DataChannelInterface;
using webrtc::DtlsIdentityStoreInterface;
using webrtc::FakeVideoTrackRenderer;
using webrtc::MediaStreamInterface;
using webrtc::PeerConnectionFactoryInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::VideoTrackSourceInterface;
using webrtc::VideoTrackInterface;

namespace {

static const char kStunIceServer[] = "stun:stun.l.google.com:19302";
static const char kTurnIceServer[] = "turn:test%40hello.com@test.com:1234";
static const char kTurnIceServerWithTransport[] =
    "turn:test@hello.com?transport=tcp";
static const char kSecureTurnIceServer[] =
    "turns:test@hello.com?transport=tcp";
static const char kSecureTurnIceServerWithoutTransportParam[] =
    "turns:test_no_transport@hello.com:443";
static const char kSecureTurnIceServerWithoutTransportAndPortParam[] =
    "turns:test_no_transport@hello.com";
static const char kTurnIceServerWithNoUsernameInUri[] =
    "turn:test.com:1234";
static const char kTurnPassword[] = "turnpassword";
static const int kDefaultStunPort = 3478;
static const int kDefaultStunTlsPort = 5349;
static const char kTurnUsername[] = "test";
static const char kStunIceServerWithIPv4Address[] = "stun:1.2.3.4:1234";
static const char kStunIceServerWithIPv4AddressWithoutPort[] = "stun:1.2.3.4";
static const char kStunIceServerWithIPv6Address[] = "stun:[2401:fa00:4::]:1234";
static const char kStunIceServerWithIPv6AddressWithoutPort[] =
    "stun:[2401:fa00:4::]";
static const char kTurnIceServerWithIPv6Address[] =
    "turn:test@[2401:fa00:4::]:1234";

class NullPeerConnectionObserver : public PeerConnectionObserver {
 public:
  virtual ~NullPeerConnectionObserver() = default;
  virtual void OnMessage(const std::string& msg) {}
  virtual void OnSignalingMessage(const std::string& msg) {}
  virtual void OnSignalingChange(
      PeerConnectionInterface::SignalingState new_state) {}
  virtual void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) {}
  virtual void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream) {
  }
  virtual void OnDataChannel(
      rtc::scoped_refptr<DataChannelInterface> data_channel) {}
  virtual void OnRenegotiationNeeded() {}
  virtual void OnIceConnectionChange(
      PeerConnectionInterface::IceConnectionState new_state) {}
  virtual void OnIceGatheringChange(
      PeerConnectionInterface::IceGatheringState new_state) {}
  virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {}
};

}  // namespace

class PeerConnectionFactoryTest : public testing::Test {
  void SetUp() {
#ifdef WEBRTC_ANDROID
    webrtc::InitializeAndroidObjects();
#endif
    factory_ = webrtc::CreatePeerConnectionFactory(
        rtc::Thread::Current(), rtc::Thread::Current(), rtc::Thread::Current(),
        nullptr, nullptr, nullptr);

    ASSERT_TRUE(factory_.get() != NULL);
    port_allocator_.reset(
        new cricket::FakePortAllocator(rtc::Thread::Current(), nullptr));
    raw_port_allocator_ = port_allocator_.get();
  }

 protected:
  void VerifyStunServers(cricket::ServerAddresses stun_servers) {
    EXPECT_EQ(stun_servers, raw_port_allocator_->stun_servers());
  }

  void VerifyTurnServers(std::vector<cricket::RelayServerConfig> turn_servers) {
    EXPECT_EQ(turn_servers.size(), raw_port_allocator_->turn_servers().size());
    for (size_t i = 0; i < turn_servers.size(); ++i) {
      ASSERT_EQ(1u, turn_servers[i].ports.size());
      EXPECT_EQ(1u, raw_port_allocator_->turn_servers()[i].ports.size());
      EXPECT_EQ(
          turn_servers[i].ports[0].address.ToString(),
          raw_port_allocator_->turn_servers()[i].ports[0].address.ToString());
      EXPECT_EQ(turn_servers[i].ports[0].proto,
                raw_port_allocator_->turn_servers()[i].ports[0].proto);
      EXPECT_EQ(turn_servers[i].credentials.username,
                raw_port_allocator_->turn_servers()[i].credentials.username);
      EXPECT_EQ(turn_servers[i].credentials.password,
                raw_port_allocator_->turn_servers()[i].credentials.password);
    }
  }

  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory_;
  NullPeerConnectionObserver observer_;
  std::unique_ptr<cricket::FakePortAllocator> port_allocator_;
  // Since the PC owns the port allocator after it's been initialized,
  // this should only be used when known to be safe.
  cricket::FakePortAllocator* raw_port_allocator_;
};

// Verify creation of PeerConnection using internal ADM, video factory and
// internal libjingle threads.
TEST(PeerConnectionFactoryTestInternal, CreatePCUsingInternalModules) {
#ifdef WEBRTC_ANDROID
  webrtc::InitializeAndroidObjects();
#endif

  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      webrtc::CreatePeerConnectionFactory());

  NullPeerConnectionObserver observer;
  webrtc::PeerConnectionInterface::RTCConfiguration config;

  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory->CreatePeerConnection(
      config, nullptr, nullptr, std::move(cert_generator), &observer));

  EXPECT_TRUE(pc.get() != nullptr);
}

// This test verifies creation of PeerConnection with valid STUN and TURN
// configuration. Also verifies the URL's parsed correctly as expected.
TEST_F(PeerConnectionFactoryTest, CreatePCUsingIceServers) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = kStunIceServer;
  config.servers.push_back(ice_server);
  ice_server.uri = kTurnIceServer;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  ice_server.uri = kTurnIceServerWithTransport;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  cricket::ServerAddresses stun_servers;
  rtc::SocketAddress stun1("stun.l.google.com", 19302);
  stun_servers.insert(stun1);
  VerifyStunServers(stun_servers);
  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn1("test.com", 1234, "test@hello.com",
                                   kTurnPassword, cricket::PROTO_UDP, false);
  turn_servers.push_back(turn1);
  cricket::RelayServerConfig turn2("hello.com", kDefaultStunPort, "test",
                                   kTurnPassword, cricket::PROTO_TCP, false);
  turn_servers.push_back(turn2);
  VerifyTurnServers(turn_servers);
}

// This test verifies creation of PeerConnection with valid STUN and TURN
// configuration. Also verifies the list of URL's parsed correctly as expected.
TEST_F(PeerConnectionFactoryTest, CreatePCUsingIceServersUrls) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.urls.push_back(kStunIceServer);
  ice_server.urls.push_back(kTurnIceServer);
  ice_server.urls.push_back(kTurnIceServerWithTransport);
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  cricket::ServerAddresses stun_servers;
  rtc::SocketAddress stun1("stun.l.google.com", 19302);
  stun_servers.insert(stun1);
  VerifyStunServers(stun_servers);
  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn1("test.com", 1234, "test@hello.com",
                                   kTurnPassword, cricket::PROTO_UDP, false);
  turn_servers.push_back(turn1);
  cricket::RelayServerConfig turn2("hello.com", kDefaultStunPort, "test",
                                   kTurnPassword, cricket::PROTO_TCP, false);
  turn_servers.push_back(turn2);
  VerifyTurnServers(turn_servers);
}

TEST_F(PeerConnectionFactoryTest, CreatePCUsingNoUsernameInUri) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = kStunIceServer;
  config.servers.push_back(ice_server);
  ice_server.uri = kTurnIceServerWithNoUsernameInUri;
  ice_server.username = kTurnUsername;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn("test.com", 1234, kTurnUsername,
                                  kTurnPassword, cricket::PROTO_UDP, false);
  turn_servers.push_back(turn);
  VerifyTurnServers(turn_servers);
}

// This test verifies the PeerConnection created properly with TURN url which
// has transport parameter in it.
TEST_F(PeerConnectionFactoryTest, CreatePCUsingTurnUrlWithTransportParam) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = kTurnIceServerWithTransport;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn("hello.com", kDefaultStunPort, "test",
                                  kTurnPassword, cricket::PROTO_TCP, false);
  turn_servers.push_back(turn);
  VerifyTurnServers(turn_servers);
}

TEST_F(PeerConnectionFactoryTest, CreatePCUsingSecureTurnUrl) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = kSecureTurnIceServer;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  ice_server.uri = kSecureTurnIceServerWithoutTransportParam;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  ice_server.uri = kSecureTurnIceServerWithoutTransportAndPortParam;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn1("hello.com", kDefaultStunTlsPort, "test",
                                   kTurnPassword, cricket::PROTO_TCP, true);
  turn_servers.push_back(turn1);
  // TURNS with transport param should be default to tcp.
  cricket::RelayServerConfig turn2("hello.com", 443, "test_no_transport",
                                   kTurnPassword, cricket::PROTO_TCP, true);
  turn_servers.push_back(turn2);
  cricket::RelayServerConfig turn3("hello.com", kDefaultStunTlsPort,
                                   "test_no_transport", kTurnPassword,
                                   cricket::PROTO_TCP, true);
  turn_servers.push_back(turn3);
  VerifyTurnServers(turn_servers);
}

TEST_F(PeerConnectionFactoryTest, CreatePCUsingIPLiteralAddress) {
  PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = kStunIceServerWithIPv4Address;
  config.servers.push_back(ice_server);
  ice_server.uri = kStunIceServerWithIPv4AddressWithoutPort;
  config.servers.push_back(ice_server);
  ice_server.uri = kStunIceServerWithIPv6Address;
  config.servers.push_back(ice_server);
  ice_server.uri = kStunIceServerWithIPv6AddressWithoutPort;
  config.servers.push_back(ice_server);
  ice_server.uri = kTurnIceServerWithIPv6Address;
  ice_server.password = kTurnPassword;
  config.servers.push_back(ice_server);
  std::unique_ptr<FakeRTCCertificateGenerator> cert_generator(
      new FakeRTCCertificateGenerator());
  rtc::scoped_refptr<PeerConnectionInterface> pc(factory_->CreatePeerConnection(
      config, nullptr, std::move(port_allocator_), std::move(cert_generator),
      &observer_));
  ASSERT_TRUE(pc.get() != NULL);
  cricket::ServerAddresses stun_servers;
  rtc::SocketAddress stun1("1.2.3.4", 1234);
  stun_servers.insert(stun1);
  rtc::SocketAddress stun2("1.2.3.4", 3478);
  stun_servers.insert(stun2);  // Default port
  rtc::SocketAddress stun3("2401:fa00:4::", 1234);
  stun_servers.insert(stun3);
  rtc::SocketAddress stun4("2401:fa00:4::", 3478);
  stun_servers.insert(stun4);  // Default port
  VerifyStunServers(stun_servers);

  std::vector<cricket::RelayServerConfig> turn_servers;
  cricket::RelayServerConfig turn1("2401:fa00:4::", 1234, "test", kTurnPassword,
                                   cricket::PROTO_UDP, false);
  turn_servers.push_back(turn1);
  VerifyTurnServers(turn_servers);
}

// This test verifies the captured stream is rendered locally using a
// local video track.
TEST_F(PeerConnectionFactoryTest, LocalRendering) {
  cricket::FakeVideoCapturer* capturer = new cricket::FakeVideoCapturer();
  // The source take ownership of |capturer|.
  rtc::scoped_refptr<VideoTrackSourceInterface> source(
      factory_->CreateVideoSource(capturer, NULL));
  ASSERT_TRUE(source.get() != NULL);
  rtc::scoped_refptr<VideoTrackInterface> track(
      factory_->CreateVideoTrack("testlabel", source));
  ASSERT_TRUE(track.get() != NULL);
  FakeVideoTrackRenderer local_renderer(track);

  EXPECT_EQ(0, local_renderer.num_rendered_frames());
  EXPECT_TRUE(capturer->CaptureFrame());
  EXPECT_EQ(1, local_renderer.num_rendered_frames());
  EXPECT_FALSE(local_renderer.black_frame());

  track->set_enabled(false);
  EXPECT_TRUE(capturer->CaptureFrame());
  EXPECT_EQ(2, local_renderer.num_rendered_frames());
  EXPECT_TRUE(local_renderer.black_frame());

  track->set_enabled(true);
  EXPECT_TRUE(capturer->CaptureFrame());
  EXPECT_EQ(3, local_renderer.num_rendered_frames());
  EXPECT_FALSE(local_renderer.black_frame());
}
