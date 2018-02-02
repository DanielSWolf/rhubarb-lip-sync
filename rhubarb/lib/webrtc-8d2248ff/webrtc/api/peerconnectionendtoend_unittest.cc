/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>

#include "webrtc/api/test/peerconnectiontestwrapper.h"
// Notice that mockpeerconnectionobservers.h must be included after the above!
#include "webrtc/api/test/mockpeerconnectionobservers.h"
#ifdef WEBRTC_ANDROID
#include "webrtc/api/test/androidtestinitializer.h"
#endif
#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/sslstreamadapter.h"
#include "webrtc/base/stringencode.h"
#include "webrtc/base/stringutils.h"

#define MAYBE_SKIP_TEST(feature)                    \
  if (!(feature())) {                               \
    LOG(LS_INFO) << "Feature disabled... skipping"; \
    return;                                         \
  }

using webrtc::DataChannelInterface;
using webrtc::FakeConstraints;
using webrtc::MediaConstraintsInterface;
using webrtc::MediaStreamInterface;
using webrtc::PeerConnectionInterface;

namespace {

const int kMaxWait = 10000;

}  // namespace

class PeerConnectionEndToEndTest
    : public sigslot::has_slots<>,
      public testing::Test {
 public:
  typedef std::vector<rtc::scoped_refptr<DataChannelInterface> >
      DataChannelList;

  PeerConnectionEndToEndTest() {
    RTC_CHECK(network_thread_.Start());
    RTC_CHECK(worker_thread_.Start());
    caller_ = new rtc::RefCountedObject<PeerConnectionTestWrapper>(
        "caller", &network_thread_, &worker_thread_);
    callee_ = new rtc::RefCountedObject<PeerConnectionTestWrapper>(
        "callee", &network_thread_, &worker_thread_);
#ifdef WEBRTC_ANDROID
    webrtc::InitializeAndroidObjects();
#endif
  }

  void CreatePcs() {
    CreatePcs(NULL);
  }

  void CreatePcs(const MediaConstraintsInterface* pc_constraints) {
    EXPECT_TRUE(caller_->CreatePc(pc_constraints));
    EXPECT_TRUE(callee_->CreatePc(pc_constraints));
    PeerConnectionTestWrapper::Connect(caller_.get(), callee_.get());

    caller_->SignalOnDataChannel.connect(
        this, &PeerConnectionEndToEndTest::OnCallerAddedDataChanel);
    callee_->SignalOnDataChannel.connect(
        this, &PeerConnectionEndToEndTest::OnCalleeAddedDataChannel);
  }

  void GetAndAddUserMedia() {
    FakeConstraints audio_constraints;
    FakeConstraints video_constraints;
    GetAndAddUserMedia(true, audio_constraints, true, video_constraints);
  }

  void GetAndAddUserMedia(bool audio, FakeConstraints audio_constraints,
                          bool video, FakeConstraints video_constraints) {
    caller_->GetAndAddUserMedia(audio, audio_constraints,
                                video, video_constraints);
    callee_->GetAndAddUserMedia(audio, audio_constraints,
                                video, video_constraints);
  }

  void Negotiate() {
    caller_->CreateOffer(NULL);
  }

  void WaitForCallEstablished() {
    caller_->WaitForCallEstablished();
    callee_->WaitForCallEstablished();
  }

  void WaitForConnection() {
    caller_->WaitForConnection();
    callee_->WaitForConnection();
  }

  void OnCallerAddedDataChanel(DataChannelInterface* dc) {
    caller_signaled_data_channels_.push_back(dc);
  }

  void OnCalleeAddedDataChannel(DataChannelInterface* dc) {
    callee_signaled_data_channels_.push_back(dc);
  }

  // Tests that |dc1| and |dc2| can send to and receive from each other.
  void TestDataChannelSendAndReceive(
      DataChannelInterface* dc1, DataChannelInterface* dc2) {
    std::unique_ptr<webrtc::MockDataChannelObserver> dc1_observer(
        new webrtc::MockDataChannelObserver(dc1));

    std::unique_ptr<webrtc::MockDataChannelObserver> dc2_observer(
        new webrtc::MockDataChannelObserver(dc2));

    static const std::string kDummyData = "abcdefg";
    webrtc::DataBuffer buffer(kDummyData);
    EXPECT_TRUE(dc1->Send(buffer));
    EXPECT_EQ_WAIT(kDummyData, dc2_observer->last_message(), kMaxWait);

    EXPECT_TRUE(dc2->Send(buffer));
    EXPECT_EQ_WAIT(kDummyData, dc1_observer->last_message(), kMaxWait);

    EXPECT_EQ(1U, dc1_observer->received_message_count());
    EXPECT_EQ(1U, dc2_observer->received_message_count());
  }

  void WaitForDataChannelsToOpen(DataChannelInterface* local_dc,
                                 const DataChannelList& remote_dc_list,
                                 size_t remote_dc_index) {
    EXPECT_EQ_WAIT(DataChannelInterface::kOpen, local_dc->state(), kMaxWait);

    EXPECT_TRUE_WAIT(remote_dc_list.size() > remote_dc_index, kMaxWait);
    EXPECT_EQ_WAIT(DataChannelInterface::kOpen,
                   remote_dc_list[remote_dc_index]->state(),
                   kMaxWait);
    EXPECT_EQ(local_dc->id(), remote_dc_list[remote_dc_index]->id());
  }

  void CloseDataChannels(DataChannelInterface* local_dc,
                         const DataChannelList& remote_dc_list,
                         size_t remote_dc_index) {
    local_dc->Close();
    EXPECT_EQ_WAIT(DataChannelInterface::kClosed, local_dc->state(), kMaxWait);
    EXPECT_EQ_WAIT(DataChannelInterface::kClosed,
                   remote_dc_list[remote_dc_index]->state(),
                   kMaxWait);
  }

 protected:
  rtc::Thread network_thread_;
  rtc::Thread worker_thread_;
  rtc::scoped_refptr<PeerConnectionTestWrapper> caller_;
  rtc::scoped_refptr<PeerConnectionTestWrapper> callee_;
  DataChannelList caller_signaled_data_channels_;
  DataChannelList callee_signaled_data_channels_;
};

// Disabled for TSan v2, see
// https://bugs.chromium.org/p/webrtc/issues/detail?id=4719 for details.
// Disabled for Mac, see
// https://bugs.chromium.org/p/webrtc/issues/detail?id=5231 for details.
#if !defined(THREAD_SANITIZER) && !defined(WEBRTC_MAC)
TEST_F(PeerConnectionEndToEndTest, Call) {
  CreatePcs();
  GetAndAddUserMedia();
  Negotiate();
  WaitForCallEstablished();
}
#endif // if !defined(THREAD_SANITIZER) && !defined(WEBRTC_MAC)

TEST_F(PeerConnectionEndToEndTest, CallWithLegacySdp) {
  FakeConstraints pc_constraints;
  pc_constraints.AddMandatory(MediaConstraintsInterface::kEnableDtlsSrtp,
                              false);
  CreatePcs(&pc_constraints);
  GetAndAddUserMedia();
  Negotiate();
  WaitForCallEstablished();
}

// Verifies that a DataChannel created before the negotiation can transition to
// "OPEN" and transfer data.
TEST_F(PeerConnectionEndToEndTest, CreateDataChannelBeforeNegotiate) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;
  rtc::scoped_refptr<DataChannelInterface> caller_dc(
      caller_->CreateDataChannel("data", init));
  rtc::scoped_refptr<DataChannelInterface> callee_dc(
      callee_->CreateDataChannel("data", init));

  Negotiate();
  WaitForConnection();

  WaitForDataChannelsToOpen(caller_dc, callee_signaled_data_channels_, 0);
  WaitForDataChannelsToOpen(callee_dc, caller_signaled_data_channels_, 0);

  TestDataChannelSendAndReceive(caller_dc, callee_signaled_data_channels_[0]);
  TestDataChannelSendAndReceive(callee_dc, caller_signaled_data_channels_[0]);

  CloseDataChannels(caller_dc, callee_signaled_data_channels_, 0);
  CloseDataChannels(callee_dc, caller_signaled_data_channels_, 0);
}

// Verifies that a DataChannel created after the negotiation can transition to
// "OPEN" and transfer data.
#if defined(MEMORY_SANITIZER)
// Fails under MemorySanitizer:
// See https://code.google.com/p/webrtc/issues/detail?id=3980.
#define MAYBE_CreateDataChannelAfterNegotiate DISABLED_CreateDataChannelAfterNegotiate
#else
#define MAYBE_CreateDataChannelAfterNegotiate CreateDataChannelAfterNegotiate
#endif
TEST_F(PeerConnectionEndToEndTest, MAYBE_CreateDataChannelAfterNegotiate) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;

  // This DataChannel is for creating the data content in the negotiation.
  rtc::scoped_refptr<DataChannelInterface> dummy(
      caller_->CreateDataChannel("data", init));
  Negotiate();
  WaitForConnection();

  // Creates new DataChannels after the negotiation and verifies their states.
  rtc::scoped_refptr<DataChannelInterface> caller_dc(
      caller_->CreateDataChannel("hello", init));
  rtc::scoped_refptr<DataChannelInterface> callee_dc(
      callee_->CreateDataChannel("hello", init));

  WaitForDataChannelsToOpen(caller_dc, callee_signaled_data_channels_, 1);
  WaitForDataChannelsToOpen(callee_dc, caller_signaled_data_channels_, 0);

  TestDataChannelSendAndReceive(caller_dc, callee_signaled_data_channels_[1]);
  TestDataChannelSendAndReceive(callee_dc, caller_signaled_data_channels_[0]);

  CloseDataChannels(caller_dc, callee_signaled_data_channels_, 1);
  CloseDataChannels(callee_dc, caller_signaled_data_channels_, 0);
}

// Verifies that DataChannel IDs are even/odd based on the DTLS roles.
TEST_F(PeerConnectionEndToEndTest, DataChannelIdAssignment) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;
  rtc::scoped_refptr<DataChannelInterface> caller_dc_1(
      caller_->CreateDataChannel("data", init));
  rtc::scoped_refptr<DataChannelInterface> callee_dc_1(
      callee_->CreateDataChannel("data", init));

  Negotiate();
  WaitForConnection();

  EXPECT_EQ(1U, caller_dc_1->id() % 2);
  EXPECT_EQ(0U, callee_dc_1->id() % 2);

  rtc::scoped_refptr<DataChannelInterface> caller_dc_2(
      caller_->CreateDataChannel("data", init));
  rtc::scoped_refptr<DataChannelInterface> callee_dc_2(
      callee_->CreateDataChannel("data", init));

  EXPECT_EQ(1U, caller_dc_2->id() % 2);
  EXPECT_EQ(0U, callee_dc_2->id() % 2);
}

// Verifies that the message is received by the right remote DataChannel when
// there are multiple DataChannels.
TEST_F(PeerConnectionEndToEndTest,
       MessageTransferBetweenTwoPairsOfDataChannels) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;

  rtc::scoped_refptr<DataChannelInterface> caller_dc_1(
      caller_->CreateDataChannel("data", init));
  rtc::scoped_refptr<DataChannelInterface> caller_dc_2(
      caller_->CreateDataChannel("data", init));

  Negotiate();
  WaitForConnection();
  WaitForDataChannelsToOpen(caller_dc_1, callee_signaled_data_channels_, 0);
  WaitForDataChannelsToOpen(caller_dc_2, callee_signaled_data_channels_, 1);

  std::unique_ptr<webrtc::MockDataChannelObserver> dc_1_observer(
      new webrtc::MockDataChannelObserver(callee_signaled_data_channels_[0]));

  std::unique_ptr<webrtc::MockDataChannelObserver> dc_2_observer(
      new webrtc::MockDataChannelObserver(callee_signaled_data_channels_[1]));

  const std::string message_1 = "hello 1";
  const std::string message_2 = "hello 2";

  caller_dc_1->Send(webrtc::DataBuffer(message_1));
  EXPECT_EQ_WAIT(message_1, dc_1_observer->last_message(), kMaxWait);

  caller_dc_2->Send(webrtc::DataBuffer(message_2));
  EXPECT_EQ_WAIT(message_2, dc_2_observer->last_message(), kMaxWait);

  EXPECT_EQ(1U, dc_1_observer->received_message_count());
  EXPECT_EQ(1U, dc_2_observer->received_message_count());
}

// Verifies that a DataChannel added from an OPEN message functions after
// a channel has been previously closed (webrtc issue 3778).
// This previously failed because the new channel re-uses the ID of the closed
// channel, and the closed channel was incorrectly still assigned to the id.
// TODO(deadbeef): This is disabled because there's currently a race condition
// caused by the fact that a data channel signals that it's closed before it
// really is. Re-enable this test once that's fixed.
TEST_F(PeerConnectionEndToEndTest,
       DISABLED_DataChannelFromOpenWorksAfterClose) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;
  rtc::scoped_refptr<DataChannelInterface> caller_dc(
      caller_->CreateDataChannel("data", init));

  Negotiate();
  WaitForConnection();

  WaitForDataChannelsToOpen(caller_dc, callee_signaled_data_channels_, 0);
  CloseDataChannels(caller_dc, callee_signaled_data_channels_, 0);

  // Create a new channel and ensure it works after closing the previous one.
  caller_dc = caller_->CreateDataChannel("data2", init);

  WaitForDataChannelsToOpen(caller_dc, callee_signaled_data_channels_, 1);
  TestDataChannelSendAndReceive(caller_dc, callee_signaled_data_channels_[1]);

  CloseDataChannels(caller_dc, callee_signaled_data_channels_, 1);
}

// This tests that if a data channel is closed remotely while not referenced
// by the application (meaning only the PeerConnection contributes to its
// reference count), no memory access violation will occur.
// See: https://code.google.com/p/chromium/issues/detail?id=565048
TEST_F(PeerConnectionEndToEndTest, CloseDataChannelRemotelyWhileNotReferenced) {
  MAYBE_SKIP_TEST(rtc::SSLStreamAdapter::HaveDtlsSrtp);

  CreatePcs();

  webrtc::DataChannelInit init;
  rtc::scoped_refptr<DataChannelInterface> caller_dc(
      caller_->CreateDataChannel("data", init));

  Negotiate();
  WaitForConnection();

  WaitForDataChannelsToOpen(caller_dc, callee_signaled_data_channels_, 0);
  // This removes the reference to the remote data channel that we hold.
  callee_signaled_data_channels_.clear();
  caller_dc->Close();
  EXPECT_EQ_WAIT(DataChannelInterface::kClosed, caller_dc->state(), kMaxWait);

  // Wait for a bit longer so the remote data channel will receive the
  // close message and be destroyed.
  rtc::Thread::Current()->ProcessMessages(100);
}
