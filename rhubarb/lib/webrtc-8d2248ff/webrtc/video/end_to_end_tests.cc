/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/base/checks.h"
#include "webrtc/base/event.h"
#include "webrtc/call.h"
#include "webrtc/call/transport_adapter.h"
#include "webrtc/common_video/include/frame_callback.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/source/byte_io.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_utility.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8.h"
#include "webrtc/modules/video_coding/codecs/vp9/include/vp9.h"
#include "webrtc/modules/video_coding/include/video_coding_defines.h"
#include "webrtc/system_wrappers/include/metrics.h"
#include "webrtc/system_wrappers/include/metrics_default.h"
#include "webrtc/system_wrappers/include/sleep.h"
#include "webrtc/test/call_test.h"
#include "webrtc/test/direct_transport.h"
#include "webrtc/test/encoder_settings.h"
#include "webrtc/test/fake_decoder.h"
#include "webrtc/test/fake_encoder.h"
#include "webrtc/test/frame_generator.h"
#include "webrtc/test/frame_generator_capturer.h"
#include "webrtc/test/null_transport.h"
#include "webrtc/test/rtcp_packet_parser.h"
#include "webrtc/test/rtp_rtcp_observer.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/perf_test.h"
#include "webrtc/video_encoder.h"

namespace webrtc {

static const int kSilenceTimeoutMs = 2000;

class EndToEndTest : public test::CallTest {
 public:
  EndToEndTest() {}

  virtual ~EndToEndTest() {
    EXPECT_EQ(nullptr, video_send_stream_);
    EXPECT_TRUE(video_receive_streams_.empty());
  }

 protected:
  class UnusedTransport : public Transport {
   private:
    bool SendRtp(const uint8_t* packet,
                 size_t length,
                 const PacketOptions& options) override {
      ADD_FAILURE() << "Unexpected RTP sent.";
      return false;
    }

    bool SendRtcp(const uint8_t* packet, size_t length) override {
      ADD_FAILURE() << "Unexpected RTCP sent.";
      return false;
    }
  };

  class RequiredTransport : public Transport {
   public:
    RequiredTransport(bool rtp_required, bool rtcp_required)
        : need_rtp_(rtp_required), need_rtcp_(rtcp_required) {}
    ~RequiredTransport() {
      if (need_rtp_) {
        ADD_FAILURE() << "Expected RTP packet not sent.";
      }
      if (need_rtcp_) {
        ADD_FAILURE() << "Expected RTCP packet not sent.";
      }
    }

   private:
    bool SendRtp(const uint8_t* packet,
                 size_t length,
                 const PacketOptions& options) override {
      need_rtp_ = false;
      return true;
    }

    bool SendRtcp(const uint8_t* packet, size_t length) override {
      need_rtcp_ = false;
      return true;
    }
    bool need_rtp_;
    bool need_rtcp_;
  };

  void DecodesRetransmittedFrame(bool enable_rtx, bool enable_red);
  void ReceivesPliAndRecovers(int rtp_history_ms);
  void RespectsRtcpMode(RtcpMode rtcp_mode);
  void TestXrReceiverReferenceTimeReport(bool enable_rrtr);
  void TestSendsSetSsrcs(size_t num_ssrcs, bool send_single_ssrc_first);
  void TestRtpStatePreservation(bool use_rtx);
  void VerifyHistogramStats(bool use_rtx, bool use_red, bool screenshare);
  void VerifyNewVideoSendStreamsRespectNetworkState(
      MediaType network_to_bring_down,
      VideoEncoder* encoder,
      Transport* transport);
  void VerifyNewVideoReceiveStreamsRespectNetworkState(
      MediaType network_to_bring_down,
      Transport* transport);
};

TEST_F(EndToEndTest, ReceiverCanBeStartedTwice) {
  CreateCalls(Call::Config(), Call::Config());

  test::NullTransport transport;
  CreateSendConfig(1, 0, &transport);
  CreateMatchingReceiveConfigs(&transport);

  CreateVideoStreams();

  video_receive_streams_[0]->Start();
  video_receive_streams_[0]->Start();

  DestroyStreams();
}

TEST_F(EndToEndTest, ReceiverCanBeStoppedTwice) {
  CreateCalls(Call::Config(), Call::Config());

  test::NullTransport transport;
  CreateSendConfig(1, 0, &transport);
  CreateMatchingReceiveConfigs(&transport);

  CreateVideoStreams();

  video_receive_streams_[0]->Stop();
  video_receive_streams_[0]->Stop();

  DestroyStreams();
}

TEST_F(EndToEndTest, RendersSingleDelayedFrame) {
  static const int kWidth = 320;
  static const int kHeight = 240;
  // This constant is chosen to be higher than the timeout in the video_render
  // module. This makes sure that frames aren't dropped if there are no other
  // frames in the queue.
  static const int kDelayRenderCallbackMs = 1000;

  class Renderer : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    Renderer() : event_(false, false) {}

    void OnFrame(const VideoFrame& video_frame) override { event_.Set(); }

    bool Wait() { return event_.Wait(kDefaultTimeoutMs); }

    rtc::Event event_;
  } renderer;

  class TestFrameCallback : public I420FrameCallback {
   public:
    TestFrameCallback() : event_(false, false) {}

    bool Wait() { return event_.Wait(kDefaultTimeoutMs); }

   private:
    void FrameCallback(VideoFrame* frame) override {
      SleepMs(kDelayRenderCallbackMs);
      event_.Set();
    }

    rtc::Event event_;
  };

  CreateCalls(Call::Config(), Call::Config());

  test::DirectTransport sender_transport(sender_call_.get());
  test::DirectTransport receiver_transport(receiver_call_.get());
  sender_transport.SetReceiver(receiver_call_->Receiver());
  receiver_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(1, 0, &sender_transport);
  CreateMatchingReceiveConfigs(&receiver_transport);

  TestFrameCallback pre_render_callback;
  video_receive_configs_[0].pre_render_callback = &pre_render_callback;
  video_receive_configs_[0].renderer = &renderer;

  CreateVideoStreams();
  Start();

  // Create frames that are smaller than the send width/height, this is done to
  // check that the callbacks are done after processing video.
  std::unique_ptr<test::FrameGenerator> frame_generator(
      test::FrameGenerator::CreateChromaGenerator(kWidth, kHeight));
  video_send_stream_->Input()->IncomingCapturedFrame(
      *frame_generator->NextFrame());
  EXPECT_TRUE(pre_render_callback.Wait())
      << "Timed out while waiting for pre-render callback.";
  EXPECT_TRUE(renderer.Wait())
      << "Timed out while waiting for the frame to render.";

  Stop();

  sender_transport.StopSending();
  receiver_transport.StopSending();

  DestroyStreams();
}

TEST_F(EndToEndTest, TransmitsFirstFrame) {
  class Renderer : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    Renderer() : event_(false, false) {}

    void OnFrame(const VideoFrame& video_frame) override { event_.Set(); }

    bool Wait() { return event_.Wait(kDefaultTimeoutMs); }

    rtc::Event event_;
  } renderer;

  CreateCalls(Call::Config(), Call::Config());

  test::DirectTransport sender_transport(sender_call_.get());
  test::DirectTransport receiver_transport(receiver_call_.get());
  sender_transport.SetReceiver(receiver_call_->Receiver());
  receiver_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(1, 0, &sender_transport);
  CreateMatchingReceiveConfigs(&receiver_transport);
  video_receive_configs_[0].renderer = &renderer;

  CreateVideoStreams();
  Start();

  std::unique_ptr<test::FrameGenerator> frame_generator(
      test::FrameGenerator::CreateChromaGenerator(
          video_encoder_config_.streams[0].width,
          video_encoder_config_.streams[0].height));
  video_send_stream_->Input()->IncomingCapturedFrame(
      *frame_generator->NextFrame());

  EXPECT_TRUE(renderer.Wait())
      << "Timed out while waiting for the frame to render.";

  Stop();

  sender_transport.StopSending();
  receiver_transport.StopSending();

  DestroyStreams();
}

class CodecObserver : public test::EndToEndTest,
                      public rtc::VideoSinkInterface<VideoFrame> {
 public:
  CodecObserver(int no_frames_to_wait_for,
                VideoRotation rotation_to_test,
                const std::string& payload_name,
                webrtc::VideoEncoder* encoder,
                webrtc::VideoDecoder* decoder)
      : EndToEndTest(2 * webrtc::EndToEndTest::kDefaultTimeoutMs),
        no_frames_to_wait_for_(no_frames_to_wait_for),
        expected_rotation_(rotation_to_test),
        payload_name_(payload_name),
        encoder_(encoder),
        decoder_(decoder),
        frame_counter_(0) {}

  void PerformTest() override {
    EXPECT_TRUE(Wait())
        << "Timed out while waiting for enough frames to be decoded.";
  }

  void ModifyVideoConfigs(
      VideoSendStream::Config* send_config,
      std::vector<VideoReceiveStream::Config>* receive_configs,
      VideoEncoderConfig* encoder_config) override {
    send_config->encoder_settings.encoder = encoder_.get();
    send_config->encoder_settings.payload_name = payload_name_;
    send_config->encoder_settings.payload_type = 126;
    encoder_config->streams[0].min_bitrate_bps = 50000;
    encoder_config->streams[0].target_bitrate_bps =
        encoder_config->streams[0].max_bitrate_bps = 2000000;

    (*receive_configs)[0].renderer = this;
    (*receive_configs)[0].decoders.resize(1);
    (*receive_configs)[0].decoders[0].payload_type =
        send_config->encoder_settings.payload_type;
    (*receive_configs)[0].decoders[0].payload_name =
        send_config->encoder_settings.payload_name;
    (*receive_configs)[0].decoders[0].decoder = decoder_.get();
  }

  void OnFrame(const VideoFrame& video_frame) override {
    EXPECT_EQ(expected_rotation_, video_frame.rotation());
    if (++frame_counter_ == no_frames_to_wait_for_)
      observation_complete_.Set();
  }

  void OnFrameGeneratorCapturerCreated(
      test::FrameGeneratorCapturer* frame_generator_capturer) override {
    frame_generator_capturer->SetFakeRotation(expected_rotation_);
  }

 private:
  int no_frames_to_wait_for_;
  VideoRotation expected_rotation_;
  std::string payload_name_;
  std::unique_ptr<webrtc::VideoEncoder> encoder_;
  std::unique_ptr<webrtc::VideoDecoder> decoder_;
  int frame_counter_;
};

TEST_F(EndToEndTest, SendsAndReceivesVP8Rotation90) {
  CodecObserver test(5, kVideoRotation_90, "VP8",
                     VideoEncoder::Create(VideoEncoder::kVp8),
                     VP8Decoder::Create());
  RunBaseTest(&test);
}

#if !defined(RTC_DISABLE_VP9)
TEST_F(EndToEndTest, SendsAndReceivesVP9) {
  CodecObserver test(500, kVideoRotation_0, "VP9",
                     VideoEncoder::Create(VideoEncoder::kVp9),
                     VP9Decoder::Create());
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, SendsAndReceivesVP9VideoRotation90) {
  CodecObserver test(5, kVideoRotation_90, "VP9",
                     VideoEncoder::Create(VideoEncoder::kVp9),
                     VP9Decoder::Create());
  RunBaseTest(&test);
}
#endif  // !defined(RTC_DISABLE_VP9)

#if defined(WEBRTC_END_TO_END_H264_TESTS)

TEST_F(EndToEndTest, SendsAndReceivesH264) {
  CodecObserver test(500, kVideoRotation_0, "H264",
                     VideoEncoder::Create(VideoEncoder::kH264),
                     H264Decoder::Create());
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, SendsAndReceivesH264VideoRotation90) {
  CodecObserver test(5, kVideoRotation_90, "H264",
                     VideoEncoder::Create(VideoEncoder::kH264),
                     H264Decoder::Create());
  RunBaseTest(&test);
}

#endif  // defined(WEBRTC_END_TO_END_H264_TESTS)

TEST_F(EndToEndTest, ReceiverUsesLocalSsrc) {
  class SyncRtcpObserver : public test::EndToEndTest {
   public:
    SyncRtcpObserver() : EndToEndTest(kDefaultTimeoutMs) {}

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());
      uint32_t ssrc = 0;
      ssrc |= static_cast<uint32_t>(packet[4]) << 24;
      ssrc |= static_cast<uint32_t>(packet[5]) << 16;
      ssrc |= static_cast<uint32_t>(packet[6]) << 8;
      ssrc |= static_cast<uint32_t>(packet[7]) << 0;
      EXPECT_EQ(kReceiverLocalVideoSsrc, ssrc);
      observation_complete_.Set();

      return SEND_PACKET;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for a receiver RTCP packet to be sent.";
    }
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ReceivesAndRetransmitsNack) {
  static const int kNumberOfNacksToObserve = 2;
  static const int kLossBurstSize = 2;
  static const int kPacketsBetweenLossBursts = 9;
  class NackObserver : public test::EndToEndTest {
   public:
    NackObserver()
        : EndToEndTest(kLongTimeoutMs),
          sent_rtp_packets_(0),
          packets_left_to_drop_(0),
          nacks_left_(kNumberOfNacksToObserve) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      // Never drop retransmitted packets.
      if (dropped_packets_.find(header.sequenceNumber) !=
          dropped_packets_.end()) {
        retransmitted_packets_.insert(header.sequenceNumber);
        if (nacks_left_ <= 0 &&
            retransmitted_packets_.size() == dropped_packets_.size()) {
          observation_complete_.Set();
        }
        return SEND_PACKET;
      }

      ++sent_rtp_packets_;

      // Enough NACKs received, stop dropping packets.
      if (nacks_left_ <= 0)
        return SEND_PACKET;

      // Check if it's time for a new loss burst.
      if (sent_rtp_packets_ % kPacketsBetweenLossBursts == 0)
        packets_left_to_drop_ = kLossBurstSize;

      // Never drop padding packets as those won't be retransmitted.
      if (packets_left_to_drop_ > 0 && header.paddingLength == 0) {
        --packets_left_to_drop_;
        dropped_packets_.insert(header.sequenceNumber);
        return DROP_PACKET;
      }

      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
      while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
        if (packet_type == RTCPUtility::RTCPPacketTypes::kRtpfbNack) {
          --nacks_left_;
          break;
        }
        packet_type = parser.Iterate();
      }
      return SEND_PACKET;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out waiting for packets to be NACKed, retransmitted and "
             "rendered.";
    }

    rtc::CriticalSection crit_;
    std::set<uint16_t> dropped_packets_;
    std::set<uint16_t> retransmitted_packets_;
    uint64_t sent_rtp_packets_;
    int packets_left_to_drop_;
    int nacks_left_ GUARDED_BY(&crit_);
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, CanReceiveFec) {
  class FecRenderObserver : public test::EndToEndTest,
                            public rtc::VideoSinkInterface<VideoFrame> {
   public:
    FecRenderObserver()
        : EndToEndTest(kDefaultTimeoutMs), state_(kFirstPacket) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      int encapsulated_payload_type = -1;
      if (header.payloadType == kRedPayloadType) {
        encapsulated_payload_type =
            static_cast<int>(packet[header.headerLength]);
        if (encapsulated_payload_type != kFakeVideoSendPayloadType)
          EXPECT_EQ(kUlpfecPayloadType, encapsulated_payload_type);
      } else {
        EXPECT_EQ(kFakeVideoSendPayloadType, header.payloadType);
      }

      if (protected_sequence_numbers_.count(header.sequenceNumber) != 0) {
        // Retransmitted packet, should not count.
        protected_sequence_numbers_.erase(header.sequenceNumber);
        EXPECT_GT(protected_timestamps_.count(header.timestamp), 0u);
        protected_timestamps_.erase(header.timestamp);
        return SEND_PACKET;
      }

      switch (state_) {
        case kFirstPacket:
          state_ = kDropEveryOtherPacketUntilFec;
          break;
        case kDropEveryOtherPacketUntilFec:
          if (encapsulated_payload_type == kUlpfecPayloadType) {
            state_ = kDropNextMediaPacket;
            return SEND_PACKET;
          }
          if (header.sequenceNumber % 2 == 0)
            return DROP_PACKET;
          break;
        case kDropNextMediaPacket:
          if (encapsulated_payload_type == kFakeVideoSendPayloadType) {
            protected_sequence_numbers_.insert(header.sequenceNumber);
            protected_timestamps_.insert(header.timestamp);
            state_ = kDropEveryOtherPacketUntilFec;
            return DROP_PACKET;
          }
          break;
      }

      return SEND_PACKET;
    }

    void OnFrame(const VideoFrame& video_frame) override {
      rtc::CritScope lock(&crit_);
      // Rendering frame with timestamp of packet that was dropped -> FEC
      // protection worked.
      if (protected_timestamps_.count(video_frame.timestamp()) != 0)
        observation_complete_.Set();
    }

    enum {
      kFirstPacket,
      kDropEveryOtherPacketUntilFec,
      kDropNextMediaPacket,
    } state_;

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      // TODO(pbos): Run this test with combined NACK/FEC enabled as well.
      // int rtp_history_ms = 1000;
      // (*receive_configs)[0].rtp.nack.rtp_history_ms = rtp_history_ms;
      // send_config->rtp.nack.rtp_history_ms = rtp_history_ms;
      send_config->rtp.fec.red_payload_type = kRedPayloadType;
      send_config->rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;

      (*receive_configs)[0].rtp.fec.red_payload_type = kRedPayloadType;
      (*receive_configs)[0].rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;
      (*receive_configs)[0].renderer = this;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out waiting for dropped frames frames to be rendered.";
    }

    rtc::CriticalSection crit_;
    std::set<uint32_t> protected_sequence_numbers_ GUARDED_BY(crit_);
    std::set<uint32_t> protected_timestamps_ GUARDED_BY(crit_);
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ReceivedFecPacketsNotNacked) {
  class FecNackObserver : public test::EndToEndTest {
   public:
    FecNackObserver()
        : EndToEndTest(kDefaultTimeoutMs),
          state_(kFirstPacket),
          fec_sequence_number_(0),
          has_last_sequence_number_(false),
          last_sequence_number_(0),
          encoder_(VideoEncoder::Create(VideoEncoder::EncoderType::kVp8)),
          decoder_(VP8Decoder::Create()) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock_(&crit_);
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      int encapsulated_payload_type = -1;
      if (header.payloadType == kRedPayloadType) {
        encapsulated_payload_type =
            static_cast<int>(packet[header.headerLength]);
        if (encapsulated_payload_type != kFakeVideoSendPayloadType)
          EXPECT_EQ(kUlpfecPayloadType, encapsulated_payload_type);
      } else {
        EXPECT_EQ(kFakeVideoSendPayloadType, header.payloadType);
      }

      if (has_last_sequence_number_ &&
          !IsNewerSequenceNumber(header.sequenceNumber,
                                 last_sequence_number_)) {
        // Drop retransmitted packets.
        return DROP_PACKET;
      }
      last_sequence_number_ = header.sequenceNumber;
      has_last_sequence_number_ = true;

      bool fec_packet = encapsulated_payload_type == kUlpfecPayloadType;
      switch (state_) {
        case kFirstPacket:
          state_ = kDropEveryOtherPacketUntilFec;
          break;
        case kDropEveryOtherPacketUntilFec:
          if (fec_packet) {
            state_ = kDropAllMediaPacketsUntilFec;
          } else if (header.sequenceNumber % 2 == 0) {
            return DROP_PACKET;
          }
          break;
        case kDropAllMediaPacketsUntilFec:
          if (!fec_packet)
            return DROP_PACKET;
          fec_sequence_number_ = header.sequenceNumber;
          state_ = kDropOneMediaPacket;
          break;
        case kDropOneMediaPacket:
          if (fec_packet)
            return DROP_PACKET;
          state_ = kPassOneMediaPacket;
          return DROP_PACKET;
          break;
        case kPassOneMediaPacket:
          if (fec_packet)
            return DROP_PACKET;
          // Pass one media packet after dropped packet after last FEC,
          // otherwise receiver might never see a seq_no after
          // |fec_sequence_number_|
          state_ = kVerifyFecPacketNotInNackList;
          break;
        case kVerifyFecPacketNotInNackList:
          // Continue to drop packets. Make sure no frame can be decoded.
          if (fec_packet || header.sequenceNumber % 2 == 0)
            return DROP_PACKET;
          break;
      }
      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock_(&crit_);
      if (state_ == kVerifyFecPacketNotInNackList) {
        test::RtcpPacketParser rtcp_parser;
        rtcp_parser.Parse(packet, length);
        std::vector<uint16_t> nacks = rtcp_parser.nack_item()->last_nack_list();
        EXPECT_TRUE(std::find(nacks.begin(), nacks.end(),
                              fec_sequence_number_) == nacks.end())
            << "Got nack for FEC packet";
        if (!nacks.empty() &&
            IsNewerSequenceNumber(nacks.back(), fec_sequence_number_)) {
          observation_complete_.Set();
        }
      }
      return SEND_PACKET;
    }

    test::PacketTransport* CreateSendTransport(Call* sender_call) override {
      // At low RTT (< kLowRttNackMs) -> NACK only, no FEC.
      // Configure some network delay.
      const int kNetworkDelayMs = 50;
      FakeNetworkPipe::Config config;
      config.queue_delay_ms = kNetworkDelayMs;
      return new test::PacketTransport(sender_call, this,
                                       test::PacketTransport::kSender, config);
    }

    // TODO(holmer): Investigate why we don't send FEC packets when the bitrate
    // is 10 kbps.
    Call::Config GetSenderCallConfig() override {
      Call::Config config;
      const int kMinBitrateBps = 30000;
      config.bitrate_config.min_bitrate_bps = kMinBitrateBps;
      return config;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      // Configure hybrid NACK/FEC.
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      send_config->rtp.fec.red_payload_type = kRedPayloadType;
      send_config->rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;
      // Set codec to VP8, otherwise NACK/FEC hybrid will be disabled.
      send_config->encoder_settings.encoder = encoder_.get();
      send_config->encoder_settings.payload_name = "VP8";
      send_config->encoder_settings.payload_type = kFakeVideoSendPayloadType;
      encoder_config->streams[0].min_bitrate_bps = 50000;
      encoder_config->streams[0].max_bitrate_bps =
          encoder_config->streams[0].target_bitrate_bps = 2000000;

      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.fec.red_payload_type = kRedPayloadType;
      (*receive_configs)[0].rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;

      (*receive_configs)[0].decoders.resize(1);
      (*receive_configs)[0].decoders[0].payload_type =
          send_config->encoder_settings.payload_type;
      (*receive_configs)[0].decoders[0].payload_name =
          send_config->encoder_settings.payload_name;
      (*receive_configs)[0].decoders[0].decoder = decoder_.get();
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for FEC packets to be received.";
    }

    enum {
      kFirstPacket,
      kDropEveryOtherPacketUntilFec,
      kDropAllMediaPacketsUntilFec,
      kDropOneMediaPacket,
      kPassOneMediaPacket,
      kVerifyFecPacketNotInNackList,
    } state_;

    rtc::CriticalSection crit_;
    uint16_t fec_sequence_number_ GUARDED_BY(&crit_);
    bool has_last_sequence_number_;
    uint16_t last_sequence_number_;
    std::unique_ptr<webrtc::VideoEncoder> encoder_;
    std::unique_ptr<webrtc::VideoDecoder> decoder_;
  } test;

  RunBaseTest(&test);
}

// This test drops second RTP packet with a marker bit set, makes sure it's
// retransmitted and renders. Retransmission SSRCs are also checked.
void EndToEndTest::DecodesRetransmittedFrame(bool enable_rtx, bool enable_red) {
  static const int kDroppedFrameNumber = 10;
  class RetransmissionObserver : public test::EndToEndTest,
                                 public I420FrameCallback {
   public:
    RetransmissionObserver(bool enable_rtx, bool enable_red)
        : EndToEndTest(kDefaultTimeoutMs),
          payload_type_(GetPayloadType(false, enable_red)),
          retransmission_ssrc_(enable_rtx ? kSendRtxSsrcs[0]
                                          : kVideoSendSsrcs[0]),
          retransmission_payload_type_(GetPayloadType(enable_rtx, enable_red)),
          encoder_(VideoEncoder::Create(VideoEncoder::EncoderType::kVp8)),
          marker_bits_observed_(0),
          retransmitted_timestamp_(0),
          frame_retransmitted_(false) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      // Ignore padding-only packets over RTX.
      if (header.payloadType != payload_type_) {
        EXPECT_EQ(retransmission_ssrc_, header.ssrc);
        if (length == header.headerLength + header.paddingLength)
          return SEND_PACKET;
      }

      if (header.timestamp == retransmitted_timestamp_) {
        EXPECT_EQ(retransmission_ssrc_, header.ssrc);
        EXPECT_EQ(retransmission_payload_type_, header.payloadType);
        frame_retransmitted_ = true;
        return SEND_PACKET;
      }

      EXPECT_EQ(kVideoSendSsrcs[0], header.ssrc)
          << "Payload type " << static_cast<int>(header.payloadType)
          << " not expected.";
      EXPECT_EQ(payload_type_, header.payloadType);

      // Found the final packet of the frame to inflict loss to, drop this and
      // expect a retransmission.
      if (header.markerBit && ++marker_bits_observed_ == kDroppedFrameNumber) {
        retransmitted_timestamp_ = header.timestamp;
        return DROP_PACKET;
      }

      return SEND_PACKET;
    }

    void FrameCallback(VideoFrame* frame) override {
      rtc::CritScope lock(&crit_);
      if (frame->timestamp() == retransmitted_timestamp_) {
        EXPECT_TRUE(frame_retransmitted_);
        observation_complete_.Set();
      }
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].pre_render_callback = this;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;

      if (payload_type_ == kRedPayloadType) {
        send_config->rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;
        send_config->rtp.fec.red_payload_type = kRedPayloadType;
        if (retransmission_ssrc_ == kSendRtxSsrcs[0])
          send_config->rtp.fec.red_rtx_payload_type = kRtxRedPayloadType;
        (*receive_configs)[0].rtp.fec.ulpfec_payload_type =
            send_config->rtp.fec.ulpfec_payload_type;
        (*receive_configs)[0].rtp.fec.red_payload_type =
            send_config->rtp.fec.red_payload_type;
        (*receive_configs)[0].rtp.fec.red_rtx_payload_type =
            send_config->rtp.fec.red_rtx_payload_type;
      }

      if (retransmission_ssrc_ == kSendRtxSsrcs[0]) {
        send_config->rtp.rtx.ssrcs.push_back(kSendRtxSsrcs[0]);
        send_config->rtp.rtx.payload_type = kSendRtxPayloadType;
        (*receive_configs)[0].rtp.rtx[payload_type_].ssrc = kSendRtxSsrcs[0];
        (*receive_configs)[0].rtp.rtx[payload_type_].payload_type =
            kSendRtxPayloadType;
      }
      // Configure encoding and decoding with VP8, since generic packetization
      // doesn't support FEC with NACK.
      RTC_DCHECK_EQ(1u, (*receive_configs)[0].decoders.size());
      send_config->encoder_settings.encoder = encoder_.get();
      send_config->encoder_settings.payload_name = "VP8";
      (*receive_configs)[0].decoders[0].payload_name = "VP8";
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for retransmission to render.";
    }

    int GetPayloadType(bool use_rtx, bool use_red) {
      if (use_red) {
        if (use_rtx)
          return kRtxRedPayloadType;
        return kRedPayloadType;
      }
      if (use_rtx)
        return kSendRtxPayloadType;
      return kFakeVideoSendPayloadType;
    }

    rtc::CriticalSection crit_;
    const int payload_type_;
    const uint32_t retransmission_ssrc_;
    const int retransmission_payload_type_;
    std::unique_ptr<VideoEncoder> encoder_;
    const std::string payload_name_;
    int marker_bits_observed_;
    uint32_t retransmitted_timestamp_ GUARDED_BY(&crit_);
    bool frame_retransmitted_;
  } test(enable_rtx, enable_red);

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, DecodesRetransmittedFrame) {
  DecodesRetransmittedFrame(false, false);
}

TEST_F(EndToEndTest, DecodesRetransmittedFrameOverRtx) {
  DecodesRetransmittedFrame(true, false);
}

TEST_F(EndToEndTest, DecodesRetransmittedFrameByRed) {
  DecodesRetransmittedFrame(false, true);
}

TEST_F(EndToEndTest, DecodesRetransmittedFrameByRedOverRtx) {
  DecodesRetransmittedFrame(true, true);
}

void EndToEndTest::ReceivesPliAndRecovers(int rtp_history_ms) {
  static const int kPacketsToDrop = 1;

  class PliObserver : public test::EndToEndTest,
                      public rtc::VideoSinkInterface<VideoFrame> {
   public:
    explicit PliObserver(int rtp_history_ms)
        : EndToEndTest(kLongTimeoutMs),
          rtp_history_ms_(rtp_history_ms),
          nack_enabled_(rtp_history_ms > 0),
          highest_dropped_timestamp_(0),
          frames_to_drop_(0),
          received_pli_(false) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      // Drop all retransmitted packets to force a PLI.
      if (header.timestamp <= highest_dropped_timestamp_)
        return DROP_PACKET;

      if (frames_to_drop_ > 0) {
        highest_dropped_timestamp_ = header.timestamp;
        --frames_to_drop_;
        return DROP_PACKET;
      }

      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      for (RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
           packet_type != RTCPUtility::RTCPPacketTypes::kInvalid;
           packet_type = parser.Iterate()) {
        if (!nack_enabled_)
          EXPECT_NE(packet_type, RTCPUtility::RTCPPacketTypes::kRtpfbNack);

        if (packet_type == RTCPUtility::RTCPPacketTypes::kPsfbPli) {
          received_pli_ = true;
          break;
        }
      }
      return SEND_PACKET;
    }

    void OnFrame(const VideoFrame& video_frame) override {
      rtc::CritScope lock(&crit_);
      if (received_pli_ &&
          video_frame.timestamp() > highest_dropped_timestamp_) {
        observation_complete_.Set();
      }
      if (!received_pli_)
        frames_to_drop_ = kPacketsToDrop;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.nack.rtp_history_ms = rtp_history_ms_;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = rtp_history_ms_;
      (*receive_configs)[0].renderer = this;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out waiting for PLI to be "
                             "received and a frame to be "
                             "rendered afterwards.";
    }

    rtc::CriticalSection crit_;
    int rtp_history_ms_;
    bool nack_enabled_;
    uint32_t highest_dropped_timestamp_ GUARDED_BY(&crit_);
    int frames_to_drop_ GUARDED_BY(&crit_);
    bool received_pli_ GUARDED_BY(&crit_);
  } test(rtp_history_ms);

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ReceivesPliAndRecoversWithNack) {
  ReceivesPliAndRecovers(1000);
}

TEST_F(EndToEndTest, ReceivesPliAndRecoversWithoutNack) {
  ReceivesPliAndRecovers(0);
}

TEST_F(EndToEndTest, UnknownRtpPacketGivesUnknownSsrcReturnCode) {
  class PacketInputObserver : public PacketReceiver {
   public:
    explicit PacketInputObserver(PacketReceiver* receiver)
        : receiver_(receiver), delivered_packet_(false, false) {}

    bool Wait() { return delivered_packet_.Wait(kDefaultTimeoutMs); }

   private:
    DeliveryStatus DeliverPacket(MediaType media_type,
                                 const uint8_t* packet,
                                 size_t length,
                                 const PacketTime& packet_time) override {
      if (RtpHeaderParser::IsRtcp(packet, length)) {
        return receiver_->DeliverPacket(media_type, packet, length,
                                        packet_time);
      } else {
        DeliveryStatus delivery_status =
            receiver_->DeliverPacket(media_type, packet, length, packet_time);
        EXPECT_EQ(DELIVERY_UNKNOWN_SSRC, delivery_status);
        delivered_packet_.Set();
        return delivery_status;
      }
    }

    PacketReceiver* receiver_;
    rtc::Event delivered_packet_;
  };

  CreateCalls(Call::Config(), Call::Config());

  test::DirectTransport send_transport(sender_call_.get());
  test::DirectTransport receive_transport(receiver_call_.get());
  PacketInputObserver input_observer(receiver_call_->Receiver());
  send_transport.SetReceiver(&input_observer);
  receive_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(1, 0, &send_transport);
  CreateMatchingReceiveConfigs(&receive_transport);

  CreateVideoStreams();
  CreateFrameGeneratorCapturer();
  Start();

  receiver_call_->DestroyVideoReceiveStream(video_receive_streams_[0]);
  video_receive_streams_.clear();

  // Wait() waits for a received packet.
  EXPECT_TRUE(input_observer.Wait());

  Stop();

  DestroyStreams();

  send_transport.StopSending();
  receive_transport.StopSending();
}

void EndToEndTest::RespectsRtcpMode(RtcpMode rtcp_mode) {
  static const int kNumCompoundRtcpPacketsToObserve = 10;
  class RtcpModeObserver : public test::EndToEndTest {
   public:
    explicit RtcpModeObserver(RtcpMode rtcp_mode)
        : EndToEndTest(kDefaultTimeoutMs),
          rtcp_mode_(rtcp_mode),
          sent_rtp_(0),
          sent_rtcp_(0) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      if (++sent_rtp_ % 3 == 0)
        return DROP_PACKET;

      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      ++sent_rtcp_;
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
      bool has_report_block = false;
      while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
        EXPECT_NE(RTCPUtility::RTCPPacketTypes::kSr, packet_type);
        if (packet_type == RTCPUtility::RTCPPacketTypes::kRr) {
          has_report_block = true;
          break;
        }
        packet_type = parser.Iterate();
      }

      switch (rtcp_mode_) {
        case RtcpMode::kCompound:
          if (!has_report_block) {
            ADD_FAILURE() << "Received RTCP packet without receiver report for "
                             "RtcpMode::kCompound.";
            observation_complete_.Set();
          }

          if (sent_rtcp_ >= kNumCompoundRtcpPacketsToObserve)
            observation_complete_.Set();

          break;
        case RtcpMode::kReducedSize:
          if (!has_report_block)
            observation_complete_.Set();
          break;
        case RtcpMode::kOff:
          RTC_NOTREACHED();
          break;
      }

      return SEND_PACKET;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.rtcp_mode = rtcp_mode_;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << (rtcp_mode_ == RtcpMode::kCompound
                  ? "Timed out before observing enough compound packets."
                  : "Timed out before receiving a non-compound RTCP packet.");
    }

    RtcpMode rtcp_mode_;
    int sent_rtp_;
    int sent_rtcp_;
  } test(rtcp_mode);

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, UsesRtcpCompoundMode) {
  RespectsRtcpMode(RtcpMode::kCompound);
}

TEST_F(EndToEndTest, UsesRtcpReducedSizeMode) {
  RespectsRtcpMode(RtcpMode::kReducedSize);
}

// Test sets up a Call multiple senders with different resolutions and SSRCs.
// Another is set up to receive all three of these with different renderers.
class MultiStreamTest {
 public:
  static const size_t kNumStreams = 3;
  struct CodecSettings {
    uint32_t ssrc;
    int width;
    int height;
  } codec_settings[kNumStreams];

  MultiStreamTest() {
    // TODO(sprang): Cleanup when msvc supports explicit initializers for array.
    codec_settings[0] = {1, 640, 480};
    codec_settings[1] = {2, 320, 240};
    codec_settings[2] = {3, 240, 160};
  }

  virtual ~MultiStreamTest() {}

  void RunTest() {
    std::unique_ptr<Call> sender_call(Call::Create(Call::Config()));
    std::unique_ptr<Call> receiver_call(Call::Create(Call::Config()));
    std::unique_ptr<test::DirectTransport> sender_transport(
        CreateSendTransport(sender_call.get()));
    std::unique_ptr<test::DirectTransport> receiver_transport(
        CreateReceiveTransport(receiver_call.get()));
    sender_transport->SetReceiver(receiver_call->Receiver());
    receiver_transport->SetReceiver(sender_call->Receiver());

    std::unique_ptr<VideoEncoder> encoders[kNumStreams];
    for (size_t i = 0; i < kNumStreams; ++i)
      encoders[i].reset(VideoEncoder::Create(VideoEncoder::kVp8));

    VideoSendStream* send_streams[kNumStreams];
    VideoReceiveStream* receive_streams[kNumStreams];

    test::FrameGeneratorCapturer* frame_generators[kNumStreams];
    std::vector<std::unique_ptr<VideoDecoder>> allocated_decoders;
    for (size_t i = 0; i < kNumStreams; ++i) {
      uint32_t ssrc = codec_settings[i].ssrc;
      int width = codec_settings[i].width;
      int height = codec_settings[i].height;

      VideoSendStream::Config send_config(sender_transport.get());
      send_config.rtp.ssrcs.push_back(ssrc);
      send_config.encoder_settings.encoder = encoders[i].get();
      send_config.encoder_settings.payload_name = "VP8";
      send_config.encoder_settings.payload_type = 124;
      VideoEncoderConfig encoder_config;
      encoder_config.streams = test::CreateVideoStreams(1);
      VideoStream* stream = &encoder_config.streams[0];
      stream->width = width;
      stream->height = height;
      stream->max_framerate = 5;
      stream->min_bitrate_bps = stream->target_bitrate_bps =
          stream->max_bitrate_bps = 100000;

      UpdateSendConfig(i, &send_config, &encoder_config, &frame_generators[i]);

      send_streams[i] =
          sender_call->CreateVideoSendStream(send_config, encoder_config);
      send_streams[i]->Start();

      VideoReceiveStream::Config receive_config(receiver_transport.get());
      receive_config.rtp.remote_ssrc = ssrc;
      receive_config.rtp.local_ssrc = test::CallTest::kReceiverLocalVideoSsrc;
      VideoReceiveStream::Decoder decoder =
          test::CreateMatchingDecoder(send_config.encoder_settings);
      allocated_decoders.push_back(
          std::unique_ptr<VideoDecoder>(decoder.decoder));
      receive_config.decoders.push_back(decoder);

      UpdateReceiveConfig(i, &receive_config);

      receive_streams[i] =
          receiver_call->CreateVideoReceiveStream(std::move(receive_config));
      receive_streams[i]->Start();

      frame_generators[i] = test::FrameGeneratorCapturer::Create(
          send_streams[i]->Input(), width, height, 30,
          Clock::GetRealTimeClock());
      frame_generators[i]->Start();
    }

    Wait();

    for (size_t i = 0; i < kNumStreams; ++i) {
      frame_generators[i]->Stop();
      sender_call->DestroyVideoSendStream(send_streams[i]);
      receiver_call->DestroyVideoReceiveStream(receive_streams[i]);
      delete frame_generators[i];
    }

    sender_transport->StopSending();
    receiver_transport->StopSending();
  }

 protected:
  virtual void Wait() = 0;
  // Note: frame_generator is a point-to-pointer, since the actual instance
  // hasn't been created at the time of this call. Only when packets/frames
  // start flowing should this be dereferenced.
  virtual void UpdateSendConfig(
      size_t stream_index,
      VideoSendStream::Config* send_config,
      VideoEncoderConfig* encoder_config,
      test::FrameGeneratorCapturer** frame_generator) {}
  virtual void UpdateReceiveConfig(size_t stream_index,
                                   VideoReceiveStream::Config* receive_config) {
  }
  virtual test::DirectTransport* CreateSendTransport(Call* sender_call) {
    return new test::DirectTransport(sender_call);
  }
  virtual test::DirectTransport* CreateReceiveTransport(Call* receiver_call) {
    return new test::DirectTransport(receiver_call);
  }
};

// Each renderer verifies that it receives the expected resolution, and as soon
// as every renderer has received a frame, the test finishes.
TEST_F(EndToEndTest, SendsAndReceivesMultipleStreams) {
  class VideoOutputObserver : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    VideoOutputObserver(const MultiStreamTest::CodecSettings& settings,
                        uint32_t ssrc,
                        test::FrameGeneratorCapturer** frame_generator)
        : settings_(settings),
          ssrc_(ssrc),
          frame_generator_(frame_generator),
          done_(false, false) {}

    void OnFrame(const VideoFrame& video_frame) override {
      EXPECT_EQ(settings_.width, video_frame.width());
      EXPECT_EQ(settings_.height, video_frame.height());
      (*frame_generator_)->Stop();
      done_.Set();
    }

    uint32_t Ssrc() { return ssrc_; }

    bool Wait() { return done_.Wait(kDefaultTimeoutMs); }

   private:
    const MultiStreamTest::CodecSettings& settings_;
    const uint32_t ssrc_;
    test::FrameGeneratorCapturer** const frame_generator_;
    rtc::Event done_;
  };

  class Tester : public MultiStreamTest {
   public:
    Tester() {}
    virtual ~Tester() {}

   protected:
    void Wait() override {
      for (const auto& observer : observers_) {
        EXPECT_TRUE(observer->Wait()) << "Time out waiting for from on ssrc "
                                      << observer->Ssrc();
      }
    }

    void UpdateSendConfig(
        size_t stream_index,
        VideoSendStream::Config* send_config,
        VideoEncoderConfig* encoder_config,
        test::FrameGeneratorCapturer** frame_generator) override {
      observers_[stream_index].reset(new VideoOutputObserver(
          codec_settings[stream_index], send_config->rtp.ssrcs.front(),
          frame_generator));
    }

    void UpdateReceiveConfig(
        size_t stream_index,
        VideoReceiveStream::Config* receive_config) override {
      receive_config->renderer = observers_[stream_index].get();
    }

   private:
    std::unique_ptr<VideoOutputObserver> observers_[kNumStreams];
  } tester;

  tester.RunTest();
}

TEST_F(EndToEndTest, AssignsTransportSequenceNumbers) {
  static const int kExtensionId = 5;

  class RtpExtensionHeaderObserver : public test::DirectTransport {
   public:
    RtpExtensionHeaderObserver(Call* sender_call,
                               const uint32_t& first_media_ssrc,
                               const std::map<uint32_t, uint32_t>& ssrc_map)
        : DirectTransport(sender_call),
          done_(false, false),
          parser_(RtpHeaderParser::Create()),
          first_media_ssrc_(first_media_ssrc),
          rtx_to_media_ssrcs_(ssrc_map),
          padding_observed_(false),
          rtx_padding_observed_(false),
          retransmit_observed_(false),
          started_(false) {
      parser_->RegisterRtpHeaderExtension(kRtpExtensionTransportSequenceNumber,
                                          kExtensionId);
    }
    virtual ~RtpExtensionHeaderObserver() {}

    bool SendRtp(const uint8_t* data,
                 size_t length,
                 const PacketOptions& options) override {
      {
        rtc::CritScope cs(&lock_);

        if (IsDone())
          return false;

        if (started_) {
          RTPHeader header;
          EXPECT_TRUE(parser_->Parse(data, length, &header));
          bool drop_packet = false;

          EXPECT_TRUE(header.extension.hasTransportSequenceNumber);
          EXPECT_EQ(options.packet_id,
                    header.extension.transportSequenceNumber);
          if (!streams_observed_.empty()) {
            // Unwrap packet id and verify uniqueness.
            int64_t packet_id = unwrapper_.Unwrap(options.packet_id);
            EXPECT_TRUE(received_packed_ids_.insert(packet_id).second);
          }

          // Drop (up to) every 17th packet, so we get retransmits.
          // Only drop media, and not on the first stream (otherwise it will be
          // hard to distinguish from padding, which is always sent on the first
          // stream).
          if (header.payloadType != kSendRtxPayloadType &&
              header.ssrc != first_media_ssrc_ &&
              header.extension.transportSequenceNumber % 17 == 0) {
            dropped_seq_[header.ssrc].insert(header.sequenceNumber);
            drop_packet = true;
          }

          size_t payload_length =
              length - (header.headerLength + header.paddingLength);
          if (payload_length == 0) {
            padding_observed_ = true;
          } else if (header.payloadType == kSendRtxPayloadType) {
            uint16_t original_sequence_number =
                ByteReader<uint16_t>::ReadBigEndian(&data[header.headerLength]);
            uint32_t original_ssrc =
                rtx_to_media_ssrcs_.find(header.ssrc)->second;
            std::set<uint16_t>* seq_no_map = &dropped_seq_[original_ssrc];
            auto it = seq_no_map->find(original_sequence_number);
            if (it != seq_no_map->end()) {
              retransmit_observed_ = true;
              seq_no_map->erase(it);
            } else {
              rtx_padding_observed_ = true;
            }
          } else {
            streams_observed_.insert(header.ssrc);
          }

          if (IsDone())
            done_.Set();

          if (drop_packet)
            return true;
        }
      }

      return test::DirectTransport::SendRtp(data, length, options);
    }

    bool IsDone() {
      bool observed_types_ok =
          streams_observed_.size() == MultiStreamTest::kNumStreams &&
          padding_observed_ && retransmit_observed_ && rtx_padding_observed_;
      if (!observed_types_ok)
        return false;
      // We should not have any gaps in the sequence number range.
      size_t seqno_range =
          *received_packed_ids_.rbegin() - *received_packed_ids_.begin() + 1;
      return seqno_range == received_packed_ids_.size();
    }

    bool Wait() {
      {
        // Can't be sure until this point that rtx_to_media_ssrcs_ etc have
        // been initialized and are OK to read.
        rtc::CritScope cs(&lock_);
        started_ = true;
      }
      return done_.Wait(kDefaultTimeoutMs);
    }

    rtc::CriticalSection lock_;
    rtc::Event done_;
    std::unique_ptr<RtpHeaderParser> parser_;
    SequenceNumberUnwrapper unwrapper_;
    std::set<int64_t> received_packed_ids_;
    std::set<uint32_t> streams_observed_;
    std::map<uint32_t, std::set<uint16_t>> dropped_seq_;
    const uint32_t& first_media_ssrc_;
    const std::map<uint32_t, uint32_t>& rtx_to_media_ssrcs_;
    bool padding_observed_;
    bool rtx_padding_observed_;
    bool retransmit_observed_;
    bool started_;
  };

  class TransportSequenceNumberTester : public MultiStreamTest {
   public:
    TransportSequenceNumberTester()
        : first_media_ssrc_(0), observer_(nullptr) {}
    virtual ~TransportSequenceNumberTester() {}

   protected:
    void Wait() override {
      RTC_DCHECK(observer_);
      EXPECT_TRUE(observer_->Wait());
    }

    void UpdateSendConfig(
        size_t stream_index,
        VideoSendStream::Config* send_config,
        VideoEncoderConfig* encoder_config,
        test::FrameGeneratorCapturer** frame_generator) override {
      send_config->rtp.extensions.clear();
      send_config->rtp.extensions.push_back(RtpExtension(
          RtpExtension::kTransportSequenceNumberUri, kExtensionId));

      // Force some padding to be sent.
      const int kPaddingBitrateBps = 50000;
      int total_target_bitrate = 0;
      for (const VideoStream& stream : encoder_config->streams)
        total_target_bitrate += stream.target_bitrate_bps;
      encoder_config->min_transmit_bitrate_bps =
          total_target_bitrate + kPaddingBitrateBps;

      // Configure RTX for redundant payload padding.
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      send_config->rtp.rtx.ssrcs.push_back(kSendRtxSsrcs[stream_index]);
      send_config->rtp.rtx.payload_type = kSendRtxPayloadType;
      rtx_to_media_ssrcs_[kSendRtxSsrcs[stream_index]] =
          send_config->rtp.ssrcs[0];

      if (stream_index == 0)
        first_media_ssrc_ = send_config->rtp.ssrcs[0];
    }

    void UpdateReceiveConfig(
        size_t stream_index,
        VideoReceiveStream::Config* receive_config) override {
      receive_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      receive_config->rtp.extensions.clear();
      receive_config->rtp.extensions.push_back(RtpExtension(
          RtpExtension::kTransportSequenceNumberUri, kExtensionId));
    }

    test::DirectTransport* CreateSendTransport(Call* sender_call) override {
      observer_ = new RtpExtensionHeaderObserver(sender_call, first_media_ssrc_,
                                                 rtx_to_media_ssrcs_);
      return observer_;
    }

   private:
    uint32_t first_media_ssrc_;
    std::map<uint32_t, uint32_t> rtx_to_media_ssrcs_;
    RtpExtensionHeaderObserver* observer_;
  } tester;

  tester.RunTest();
}

class TransportFeedbackTester : public test::EndToEndTest {
 public:
  explicit TransportFeedbackTester(bool feedback_enabled,
                                   size_t num_video_streams,
                                   size_t num_audio_streams)
      : EndToEndTest(::webrtc::EndToEndTest::kDefaultTimeoutMs),
        feedback_enabled_(feedback_enabled),
        num_video_streams_(num_video_streams),
        num_audio_streams_(num_audio_streams) {
    // Only one stream of each supported for now.
    EXPECT_LE(num_video_streams, 1u);
    EXPECT_LE(num_audio_streams, 1u);
  }

 protected:
  Action OnSendRtcp(const uint8_t* data, size_t length) override {
    EXPECT_FALSE(HasTransportFeedback(data, length));
    return SEND_PACKET;
  }

  Action OnReceiveRtcp(const uint8_t* data, size_t length) override {
    if (HasTransportFeedback(data, length))
      observation_complete_.Set();
    return SEND_PACKET;
  }

  bool HasTransportFeedback(const uint8_t* data, size_t length) const {
    RTCPUtility::RTCPParserV2 parser(data, length, true);
    EXPECT_TRUE(parser.IsValid());

    RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
    while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
      if (packet_type == RTCPUtility::RTCPPacketTypes::kTransportFeedback)
        return true;
      packet_type = parser.Iterate();
    }

    return false;
  }

  void PerformTest() override {
    const int64_t kDisabledFeedbackTimeoutMs = 5000;
    EXPECT_EQ(feedback_enabled_,
              observation_complete_.Wait(feedback_enabled_
                                             ? test::CallTest::kDefaultTimeoutMs
                                             : kDisabledFeedbackTimeoutMs));
  }

  void OnCallsCreated(Call* sender_call, Call* receiver_call) override {
    receiver_call_ = receiver_call;
  }

  size_t GetNumVideoStreams() const override { return num_video_streams_; }
  size_t GetNumAudioStreams() const override { return num_audio_streams_; }

  void ModifyVideoConfigs(
      VideoSendStream::Config* send_config,
      std::vector<VideoReceiveStream::Config>* receive_configs,
      VideoEncoderConfig* encoder_config) override {
    send_config->rtp.extensions.clear();
    send_config->rtp.extensions.push_back(
        RtpExtension(RtpExtension::kTransportSequenceNumberUri, kExtensionId));
    (*receive_configs)[0].rtp.extensions = send_config->rtp.extensions;
    (*receive_configs)[0].rtp.transport_cc = feedback_enabled_;
  }

  void ModifyAudioConfigs(
      AudioSendStream::Config* send_config,
      std::vector<AudioReceiveStream::Config>* receive_configs) override {
    send_config->rtp.extensions.clear();
    send_config->rtp.extensions.push_back(
        RtpExtension(RtpExtension::kTransportSequenceNumberUri, kExtensionId));
    (*receive_configs)[0].rtp.extensions.clear();
    (*receive_configs)[0].rtp.extensions = send_config->rtp.extensions;
    (*receive_configs)[0].rtp.transport_cc = feedback_enabled_;
  }

 private:
  static const int kExtensionId = 5;
  const bool feedback_enabled_;
  const size_t num_video_streams_;
  const size_t num_audio_streams_;
  Call* receiver_call_;
};

TEST_F(EndToEndTest, VideoReceivesTransportFeedback) {
  TransportFeedbackTester test(true, 1, 0);
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, VideoTransportFeedbackNotConfigured) {
  TransportFeedbackTester test(false, 1, 0);
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, AudioReceivesTransportFeedback) {
  TransportFeedbackTester test(true, 0, 1);
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, AudioTransportFeedbackNotConfigured) {
  TransportFeedbackTester test(false, 0, 1);
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, AudioVideoReceivesTransportFeedback) {
  TransportFeedbackTester test(true, 1, 1);
  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ObserversEncodedFrames) {
  class EncodedFrameTestObserver : public EncodedFrameObserver {
   public:
    EncodedFrameTestObserver()
        : length_(0), frame_type_(kEmptyFrame), called_(false, false) {}
    virtual ~EncodedFrameTestObserver() {}

    virtual void EncodedFrameCallback(const EncodedFrame& encoded_frame) {
      frame_type_ = encoded_frame.frame_type_;
      length_ = encoded_frame.length_;
      buffer_.reset(new uint8_t[length_]);
      memcpy(buffer_.get(), encoded_frame.data_, length_);
      called_.Set();
    }

    bool Wait() { return called_.Wait(kDefaultTimeoutMs); }

    void ExpectEqualFrames(const EncodedFrameTestObserver& observer) {
      ASSERT_EQ(length_, observer.length_)
          << "Observed frames are of different lengths.";
      EXPECT_EQ(frame_type_, observer.frame_type_)
          << "Observed frames have different frame types.";
      EXPECT_EQ(0, memcmp(buffer_.get(), observer.buffer_.get(), length_))
          << "Observed encoded frames have different content.";
    }

   private:
    std::unique_ptr<uint8_t[]> buffer_;
    size_t length_;
    FrameType frame_type_;
    rtc::Event called_;
  };

  EncodedFrameTestObserver post_encode_observer;
  EncodedFrameTestObserver pre_decode_observer;

  CreateCalls(Call::Config(), Call::Config());

  test::DirectTransport sender_transport(sender_call_.get());
  test::DirectTransport receiver_transport(receiver_call_.get());
  sender_transport.SetReceiver(receiver_call_->Receiver());
  receiver_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(1, 0, &sender_transport);
  CreateMatchingReceiveConfigs(&receiver_transport);
  video_send_config_.post_encode_callback = &post_encode_observer;
  video_receive_configs_[0].pre_decode_callback = &pre_decode_observer;

  CreateVideoStreams();
  Start();

  std::unique_ptr<test::FrameGenerator> frame_generator(
      test::FrameGenerator::CreateChromaGenerator(
          video_encoder_config_.streams[0].width,
          video_encoder_config_.streams[0].height));
  video_send_stream_->Input()->IncomingCapturedFrame(
      *frame_generator->NextFrame());

  EXPECT_TRUE(post_encode_observer.Wait())
      << "Timed out while waiting for send-side encoded-frame callback.";

  EXPECT_TRUE(pre_decode_observer.Wait())
      << "Timed out while waiting for pre-decode encoded-frame callback.";

  post_encode_observer.ExpectEqualFrames(pre_decode_observer);

  Stop();

  sender_transport.StopSending();
  receiver_transport.StopSending();

  DestroyStreams();
}

TEST_F(EndToEndTest, ReceiveStreamSendsRemb) {
  class RembObserver : public test::EndToEndTest {
   public:
    RembObserver() : EndToEndTest(kDefaultTimeoutMs) {}

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      bool received_psfb = false;
      bool received_remb = false;
      RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
      while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
        if (packet_type == RTCPUtility::RTCPPacketTypes::kPsfbRemb) {
          const RTCPUtility::RTCPPacket& packet = parser.Packet();
          EXPECT_EQ(packet.PSFBAPP.SenderSSRC, kReceiverLocalVideoSsrc);
          received_psfb = true;
        } else if (packet_type == RTCPUtility::RTCPPacketTypes::kPsfbRembItem) {
          const RTCPUtility::RTCPPacket& packet = parser.Packet();
          EXPECT_GT(packet.REMBItem.BitRate, 0u);
          EXPECT_EQ(packet.REMBItem.NumberOfSSRCs, 1u);
          EXPECT_EQ(packet.REMBItem.SSRCs[0], kVideoSendSsrcs[0]);
          received_remb = true;
        }
        packet_type = parser.Iterate();
      }
      if (received_psfb && received_remb)
        observation_complete_.Set();
      return SEND_PACKET;
    }
    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out while waiting for a "
                             "receiver RTCP REMB packet to be "
                             "sent.";
    }
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, VerifyBandwidthStats) {
  class RtcpObserver : public test::EndToEndTest {
   public:
    RtcpObserver()
        : EndToEndTest(kDefaultTimeoutMs),
          sender_call_(nullptr),
          receiver_call_(nullptr),
          has_seen_pacer_delay_(false) {}

    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      Call::Stats sender_stats = sender_call_->GetStats();
      Call::Stats receiver_stats = receiver_call_->GetStats();
      if (!has_seen_pacer_delay_)
        has_seen_pacer_delay_ = sender_stats.pacer_delay_ms > 0;
      if (sender_stats.send_bandwidth_bps > 0 &&
          receiver_stats.recv_bandwidth_bps > 0 && has_seen_pacer_delay_) {
        observation_complete_.Set();
      }
      return SEND_PACKET;
    }

    void OnCallsCreated(Call* sender_call, Call* receiver_call) override {
      sender_call_ = sender_call;
      receiver_call_ = receiver_call;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out while waiting for "
                             "non-zero bandwidth stats.";
    }

   private:
    Call* sender_call_;
    Call* receiver_call_;
    bool has_seen_pacer_delay_;
  } test;

  RunBaseTest(&test);
}


// Verifies that it's possible to limit the send BWE by sending a REMB.
// This is verified by allowing the send BWE to ramp-up to >1000 kbps,
// then have the test generate a REMB of 500 kbps and verify that the send BWE
// is reduced to exactly 500 kbps. Then a REMB of 1000 kbps is generated and the
// test verifies that the send BWE ramps back up to exactly 1000 kbps.
TEST_F(EndToEndTest, RembWithSendSideBwe) {
  class BweObserver : public test::EndToEndTest {
   public:
    BweObserver()
        : EndToEndTest(kDefaultTimeoutMs),
          sender_call_(nullptr),
          clock_(Clock::GetRealTimeClock()),
          sender_ssrc_(0),
          remb_bitrate_bps_(1000000),
          receive_transport_(nullptr),
          event_(false, false),
          poller_thread_(&BitrateStatsPollingThread,
                         this,
                         "BitrateStatsPollingThread"),
          state_(kWaitForFirstRampUp) {}

    ~BweObserver() {}

    test::PacketTransport* CreateReceiveTransport() override {
      receive_transport_ = new test::PacketTransport(
          nullptr, this, test::PacketTransport::kReceiver,
          FakeNetworkPipe::Config());
      return receive_transport_;
    }

    Call::Config GetSenderCallConfig() override {
      Call::Config config;
      // Set a high start bitrate to reduce the test completion time.
      config.bitrate_config.start_bitrate_bps = remb_bitrate_bps_;
      return config;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      ASSERT_EQ(1u, send_config->rtp.ssrcs.size());
      send_config->rtp.extensions.clear();
      send_config->rtp.extensions.push_back(
          RtpExtension(RtpExtension::kTransportSequenceNumberUri,
                       test::kTransportSequenceNumberExtensionId));
      sender_ssrc_ = send_config->rtp.ssrcs[0];

      encoder_config->streams[0].max_bitrate_bps =
          encoder_config->streams[0].target_bitrate_bps = 2000000;

      ASSERT_EQ(1u, receive_configs->size());
      (*receive_configs)[0].rtp.remb = false;
      (*receive_configs)[0].rtp.transport_cc = true;
      (*receive_configs)[0].rtp.extensions = send_config->rtp.extensions;
      RtpRtcp::Configuration config;
      config.receiver_only = true;
      config.clock = clock_;
      config.outgoing_transport = receive_transport_;
      rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(config));
      rtp_rtcp_->SetRemoteSSRC((*receive_configs)[0].rtp.remote_ssrc);
      rtp_rtcp_->SetSSRC((*receive_configs)[0].rtp.local_ssrc);
      rtp_rtcp_->SetREMBStatus(true);
      rtp_rtcp_->SetSendingStatus(true);
      rtp_rtcp_->SetRTCPStatus(RtcpMode::kReducedSize);
    }

    void OnCallsCreated(Call* sender_call, Call* receiver_call) override {
      sender_call_ = sender_call;
    }

    static bool BitrateStatsPollingThread(void* obj) {
      return static_cast<BweObserver*>(obj)->PollStats();
    }

    bool PollStats() {
      if (sender_call_) {
        Call::Stats stats = sender_call_->GetStats();
        switch (state_) {
          case kWaitForFirstRampUp:
            if (stats.send_bandwidth_bps >= remb_bitrate_bps_) {
              state_ = kWaitForRemb;
              remb_bitrate_bps_ /= 2;
              rtp_rtcp_->SetREMBData(
                  remb_bitrate_bps_,
                  std::vector<uint32_t>(&sender_ssrc_, &sender_ssrc_ + 1));
              rtp_rtcp_->SendRTCP(kRtcpRr);
            }
            break;

          case kWaitForRemb:
            if (stats.send_bandwidth_bps == remb_bitrate_bps_) {
              state_ = kWaitForSecondRampUp;
              remb_bitrate_bps_ *= 2;
              rtp_rtcp_->SetREMBData(
                  remb_bitrate_bps_,
                  std::vector<uint32_t>(&sender_ssrc_, &sender_ssrc_ + 1));
              rtp_rtcp_->SendRTCP(kRtcpRr);
            }
            break;

          case kWaitForSecondRampUp:
            if (stats.send_bandwidth_bps == remb_bitrate_bps_) {
              observation_complete_.Set();
            }
            break;
        }
      }

      return !event_.Wait(1000);
    }

    void PerformTest() override {
      poller_thread_.Start();
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for bitrate to change according to REMB.";
      poller_thread_.Stop();
    }

   private:
    enum TestState { kWaitForFirstRampUp, kWaitForRemb, kWaitForSecondRampUp };

    Call* sender_call_;
    Clock* const clock_;
    uint32_t sender_ssrc_;
    int remb_bitrate_bps_;
    std::unique_ptr<RtpRtcp> rtp_rtcp_;
    test::PacketTransport* receive_transport_;
    rtc::Event event_;
    rtc::PlatformThread poller_thread_;
    TestState state_;
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, VerifyNackStats) {
  static const int kPacketNumberToDrop = 200;
  class NackObserver : public test::EndToEndTest {
   public:
    NackObserver()
        : EndToEndTest(kLongTimeoutMs),
          sent_rtp_packets_(0),
          dropped_rtp_packet_(0),
          dropped_rtp_packet_requested_(false),
          send_stream_(nullptr),
          start_runtime_ms_(-1) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      if (++sent_rtp_packets_ == kPacketNumberToDrop) {
        std::unique_ptr<RtpHeaderParser> parser(RtpHeaderParser::Create());
        RTPHeader header;
        EXPECT_TRUE(parser->Parse(packet, length, &header));
        dropped_rtp_packet_ = header.sequenceNumber;
        return DROP_PACKET;
      }
      VerifyStats();
      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      test::RtcpPacketParser rtcp_parser;
      rtcp_parser.Parse(packet, length);
      std::vector<uint16_t> nacks = rtcp_parser.nack_item()->last_nack_list();
      if (!nacks.empty() && std::find(
          nacks.begin(), nacks.end(), dropped_rtp_packet_) != nacks.end()) {
        dropped_rtp_packet_requested_ = true;
      }
      return SEND_PACKET;
    }

    void VerifyStats() EXCLUSIVE_LOCKS_REQUIRED(&crit_) {
      if (!dropped_rtp_packet_requested_)
        return;
      int send_stream_nack_packets = 0;
      int receive_stream_nack_packets = 0;
      VideoSendStream::Stats stats = send_stream_->GetStats();
      for (std::map<uint32_t, VideoSendStream::StreamStats>::const_iterator it =
           stats.substreams.begin(); it != stats.substreams.end(); ++it) {
        const VideoSendStream::StreamStats& stream_stats = it->second;
        send_stream_nack_packets +=
            stream_stats.rtcp_packet_type_counts.nack_packets;
      }
      for (size_t i = 0; i < receive_streams_.size(); ++i) {
        VideoReceiveStream::Stats stats = receive_streams_[i]->GetStats();
        receive_stream_nack_packets +=
            stats.rtcp_packet_type_counts.nack_packets;
      }
      if (send_stream_nack_packets >= 1 && receive_stream_nack_packets >= 1) {
        // NACK packet sent on receive stream and received on sent stream.
        if (MinMetricRunTimePassed())
          observation_complete_.Set();
      }
    }

    bool MinMetricRunTimePassed() {
      int64_t now = Clock::GetRealTimeClock()->TimeInMilliseconds();
      if (start_runtime_ms_ == -1) {
        start_runtime_ms_ = now;
        return false;
      }
      int64_t elapsed_sec = (now - start_runtime_ms_) / 1000;
      return elapsed_sec > metrics::kMinRunTimeInSeconds;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
    }

    void OnVideoStreamsCreated(
        VideoSendStream* send_stream,
        const std::vector<VideoReceiveStream*>& receive_streams) override {
      send_stream_ = send_stream;
      receive_streams_ = receive_streams;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out waiting for packet to be NACKed.";
    }

    rtc::CriticalSection crit_;
    uint64_t sent_rtp_packets_;
    uint16_t dropped_rtp_packet_ GUARDED_BY(&crit_);
    bool dropped_rtp_packet_requested_ GUARDED_BY(&crit_);
    std::vector<VideoReceiveStream*> receive_streams_;
    VideoSendStream* send_stream_;
    int64_t start_runtime_ms_;
  } test;

  metrics::Reset();
  RunBaseTest(&test);

  EXPECT_EQ(
      1, metrics::NumSamples("WebRTC.Video.UniqueNackRequestsSentInPercent"));
  EXPECT_EQ(1, metrics::NumSamples(
                   "WebRTC.Video.UniqueNackRequestsReceivedInPercent"));
  EXPECT_GT(metrics::MinSample("WebRTC.Video.NackPacketsSentPerMinute"), 0);
}

void EndToEndTest::VerifyHistogramStats(bool use_rtx,
                                        bool use_red,
                                        bool screenshare) {
  class StatsObserver : public test::EndToEndTest,
                        public rtc::VideoSinkInterface<VideoFrame> {
   public:
    StatsObserver(bool use_rtx, bool use_red, bool screenshare)
        : EndToEndTest(kLongTimeoutMs),
          use_rtx_(use_rtx),
          use_red_(use_red),
          screenshare_(screenshare),
          // This test uses NACK, so to send FEC we can't use a fake encoder.
          vp8_encoder_(
              use_red ? VideoEncoder::Create(VideoEncoder::EncoderType::kVp8)
                      : nullptr),
          sender_call_(nullptr),
          receiver_call_(nullptr),
          start_runtime_ms_(-1) {}

   private:
    void OnFrame(const VideoFrame& video_frame) override {}

    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      if (MinMetricRunTimePassed())
        observation_complete_.Set();

      return SEND_PACKET;
    }

    bool MinMetricRunTimePassed() {
      int64_t now = Clock::GetRealTimeClock()->TimeInMilliseconds();
      if (start_runtime_ms_ == -1) {
        start_runtime_ms_ = now;
        return false;
      }
      int64_t elapsed_sec = (now - start_runtime_ms_) / 1000;
      return elapsed_sec > metrics::kMinRunTimeInSeconds * 2;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      // NACK
      send_config->rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].rtp.nack.rtp_history_ms = kNackRtpHistoryMs;
      (*receive_configs)[0].renderer = this;
      // FEC
      if (use_red_) {
        send_config->rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;
        send_config->rtp.fec.red_payload_type = kRedPayloadType;
        send_config->encoder_settings.encoder = vp8_encoder_.get();
        send_config->encoder_settings.payload_name = "VP8";
        (*receive_configs)[0].decoders[0].payload_name = "VP8";
        (*receive_configs)[0].rtp.fec.red_payload_type = kRedPayloadType;
        (*receive_configs)[0].rtp.fec.ulpfec_payload_type = kUlpfecPayloadType;
      }
      // RTX
      if (use_rtx_) {
        send_config->rtp.rtx.ssrcs.push_back(kSendRtxSsrcs[0]);
        send_config->rtp.rtx.payload_type = kSendRtxPayloadType;
        (*receive_configs)[0].rtp.rtx[kFakeVideoSendPayloadType].ssrc =
            kSendRtxSsrcs[0];
        (*receive_configs)[0].rtp.rtx[kFakeVideoSendPayloadType].payload_type =
            kSendRtxPayloadType;
      }
      encoder_config->content_type =
          screenshare_ ? VideoEncoderConfig::ContentType::kScreen
                       : VideoEncoderConfig::ContentType::kRealtimeVideo;
    }

    void OnCallsCreated(Call* sender_call, Call* receiver_call) override {
      sender_call_ = sender_call;
      receiver_call_ = receiver_call;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out waiting for packet to be NACKed.";
    }

    const bool use_rtx_;
    const bool use_red_;
    const bool screenshare_;
    const std::unique_ptr<VideoEncoder> vp8_encoder_;
    Call* sender_call_;
    Call* receiver_call_;
    int64_t start_runtime_ms_;
  } test(use_rtx, use_red, screenshare);

  metrics::Reset();
  RunBaseTest(&test);

  // Delete the call for Call stats to be reported.
  sender_call_.reset();
  receiver_call_.reset();

  std::string video_prefix =
      screenshare ? "WebRTC.Video.Screenshare." : "WebRTC.Video.";

  // Verify that stats have been updated once.
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Call.VideoBitrateReceivedInKbps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Call.RtcpBitrateReceivedInBps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Call.BitrateReceivedInKbps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Call.EstimatedSendBitrateInKbps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Call.PacerBitrateInKbps"));

  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.NackPacketsSentPerMinute"));
  EXPECT_EQ(1,
            metrics::NumSamples(video_prefix + "NackPacketsReceivedPerMinute"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.FirPacketsSentPerMinute"));
  EXPECT_EQ(1,
            metrics::NumSamples(video_prefix + "FirPacketsReceivedPerMinute"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.PliPacketsSentPerMinute"));
  EXPECT_EQ(1,
            metrics::NumSamples(video_prefix + "PliPacketsReceivedPerMinute"));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "KeyFramesSentInPermille"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.KeyFramesReceivedInPermille"));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SentPacketsLostInPercent"));
  EXPECT_EQ(1,
            metrics::NumSamples("WebRTC.Video.ReceivedPacketsLostInPercent"));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "InputWidthInPixels"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "InputHeightInPixels"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SentWidthInPixels"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SentHeightInPixels"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.ReceivedWidthInPixels"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.ReceivedHeightInPixels"));

  EXPECT_EQ(1, metrics::NumEvents(
                   video_prefix + "InputWidthInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].width)));
  EXPECT_EQ(1, metrics::NumEvents(
                   video_prefix + "InputHeightInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].height)));
  EXPECT_EQ(1, metrics::NumEvents(
                   video_prefix + "SentWidthInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].width)));
  EXPECT_EQ(1, metrics::NumEvents(
                   video_prefix + "SentHeightInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].height)));
  EXPECT_EQ(1, metrics::NumEvents(
                   "WebRTC.Video.ReceivedWidthInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].width)));
  EXPECT_EQ(1, metrics::NumEvents(
                   "WebRTC.Video.ReceivedHeightInPixels",
                   static_cast<int>(video_encoder_config_.streams[0].height)));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "InputFramesPerSecond"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SentFramesPerSecond"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.DecodedFramesPerSecond"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.RenderFramesPerSecond"));

  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.JitterBufferDelayInMs"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.TargetDelayInMs"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.CurrentDelayInMs"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.OnewayDelayInMs"));

  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.RenderSqrtPixelsPerSecond"));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "EncodeTimeInMs"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.DecodeTimeInMs"));

  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "BitrateSentInKbps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.BitrateReceivedInKbps"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "MediaBitrateSentInKbps"));
  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.MediaBitrateReceivedInKbps"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "PaddingBitrateSentInKbps"));
  EXPECT_EQ(1,
            metrics::NumSamples("WebRTC.Video.PaddingBitrateReceivedInKbps"));
  EXPECT_EQ(
      1, metrics::NumSamples(video_prefix + "RetransmittedBitrateSentInKbps"));
  EXPECT_EQ(1, metrics::NumSamples(
                   "WebRTC.Video.RetransmittedBitrateReceivedInKbps"));

  EXPECT_EQ(1, metrics::NumSamples("WebRTC.Video.SendDelayInMs"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SendSideDelayInMs"));
  EXPECT_EQ(1, metrics::NumSamples(video_prefix + "SendSideDelayMaxInMs"));

  int num_rtx_samples = use_rtx ? 1 : 0;
  EXPECT_EQ(num_rtx_samples,
            metrics::NumSamples("WebRTC.Video.RtxBitrateSentInKbps"));
  EXPECT_EQ(num_rtx_samples,
            metrics::NumSamples("WebRTC.Video.RtxBitrateReceivedInKbps"));

  int num_red_samples = use_red ? 1 : 0;
  EXPECT_EQ(num_red_samples,
            metrics::NumSamples("WebRTC.Video.FecBitrateSentInKbps"));
  EXPECT_EQ(num_red_samples,
            metrics::NumSamples("WebRTC.Video.FecBitrateReceivedInKbps"));
  EXPECT_EQ(num_red_samples,
            metrics::NumSamples("WebRTC.Video.ReceivedFecPacketsInPercent"));
}

TEST_F(EndToEndTest, VerifyHistogramStatsWithRtx) {
  const bool kEnabledRtx = true;
  const bool kEnabledRed = false;
  const bool kScreenshare = false;
  VerifyHistogramStats(kEnabledRtx, kEnabledRed, kScreenshare);
}

TEST_F(EndToEndTest, VerifyHistogramStatsWithRed) {
  const bool kEnabledRtx = false;
  const bool kEnabledRed = true;
  const bool kScreenshare = false;
  VerifyHistogramStats(kEnabledRtx, kEnabledRed, kScreenshare);
}

TEST_F(EndToEndTest, VerifyHistogramStatsWithScreenshare) {
  const bool kEnabledRtx = false;
  const bool kEnabledRed = false;
  const bool kScreenshare = true;
  VerifyHistogramStats(kEnabledRtx, kEnabledRed, kScreenshare);
}

void EndToEndTest::TestXrReceiverReferenceTimeReport(bool enable_rrtr) {
  static const int kNumRtcpReportPacketsToObserve = 5;
  class RtcpXrObserver : public test::EndToEndTest {
   public:
    explicit RtcpXrObserver(bool enable_rrtr)
        : EndToEndTest(kDefaultTimeoutMs),
          enable_rrtr_(enable_rrtr),
          sent_rtcp_sr_(0),
          sent_rtcp_rr_(0),
          sent_rtcp_rrtr_(0),
          sent_rtcp_dlrr_(0) {}

   private:
    // Receive stream should send RR packets (and RRTR packets if enabled).
    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
      while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
        if (packet_type == RTCPUtility::RTCPPacketTypes::kRr) {
          ++sent_rtcp_rr_;
        } else if (packet_type ==
                   RTCPUtility::RTCPPacketTypes::kXrReceiverReferenceTime) {
          ++sent_rtcp_rrtr_;
        }
        EXPECT_NE(packet_type, RTCPUtility::RTCPPacketTypes::kSr);
        EXPECT_NE(packet_type,
                  RTCPUtility::RTCPPacketTypes::kXrDlrrReportBlockItem);
        packet_type = parser.Iterate();
      }
      return SEND_PACKET;
    }
    // Send stream should send SR packets (and DLRR packets if enabled).
    Action OnSendRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&crit_);
      RTCPUtility::RTCPParserV2 parser(packet, length, true);
      EXPECT_TRUE(parser.IsValid());

      RTCPUtility::RTCPPacketTypes packet_type = parser.Begin();
      while (packet_type != RTCPUtility::RTCPPacketTypes::kInvalid) {
        if (packet_type == RTCPUtility::RTCPPacketTypes::kSr) {
          ++sent_rtcp_sr_;
        } else if (packet_type ==
                   RTCPUtility::RTCPPacketTypes::kXrDlrrReportBlockItem) {
          ++sent_rtcp_dlrr_;
        }
        EXPECT_NE(packet_type,
                  RTCPUtility::RTCPPacketTypes::kXrReceiverReferenceTime);
        packet_type = parser.Iterate();
      }
      if (sent_rtcp_sr_ > kNumRtcpReportPacketsToObserve &&
          sent_rtcp_rr_ > kNumRtcpReportPacketsToObserve) {
        if (enable_rrtr_) {
          EXPECT_GT(sent_rtcp_rrtr_, 0);
          EXPECT_GT(sent_rtcp_dlrr_, 0);
        } else {
          EXPECT_EQ(0, sent_rtcp_rrtr_);
          EXPECT_EQ(0, sent_rtcp_dlrr_);
        }
        observation_complete_.Set();
      }
      return SEND_PACKET;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      (*receive_configs)[0].rtp.rtcp_mode = RtcpMode::kReducedSize;
      (*receive_configs)[0].rtp.rtcp_xr.receiver_reference_time_report =
          enable_rrtr_;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for RTCP SR/RR packets to be sent.";
    }

    rtc::CriticalSection crit_;
    bool enable_rrtr_;
    int sent_rtcp_sr_;
    int sent_rtcp_rr_ GUARDED_BY(&crit_);
    int sent_rtcp_rrtr_ GUARDED_BY(&crit_);
    int sent_rtcp_dlrr_;
  } test(enable_rrtr);

  RunBaseTest(&test);
}

void EndToEndTest::TestSendsSetSsrcs(size_t num_ssrcs,
                                     bool send_single_ssrc_first) {
  class SendsSetSsrcs : public test::EndToEndTest {
   public:
    SendsSetSsrcs(const uint32_t* ssrcs,
                  size_t num_ssrcs,
                  bool send_single_ssrc_first)
        : EndToEndTest(kDefaultTimeoutMs),
          num_ssrcs_(num_ssrcs),
          send_single_ssrc_first_(send_single_ssrc_first),
          ssrcs_to_observe_(num_ssrcs),
          expect_single_ssrc_(send_single_ssrc_first),
          send_stream_(nullptr) {
      for (size_t i = 0; i < num_ssrcs; ++i)
        valid_ssrcs_[ssrcs[i]] = true;
    }

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      EXPECT_TRUE(valid_ssrcs_[header.ssrc])
          << "Received unknown SSRC: " << header.ssrc;

      if (!valid_ssrcs_[header.ssrc])
        observation_complete_.Set();

      if (!is_observed_[header.ssrc]) {
        is_observed_[header.ssrc] = true;
        --ssrcs_to_observe_;
        if (expect_single_ssrc_) {
          expect_single_ssrc_ = false;
          observation_complete_.Set();
        }
      }

      if (ssrcs_to_observe_ == 0)
        observation_complete_.Set();

      return SEND_PACKET;
    }

    size_t GetNumVideoStreams() const override { return num_ssrcs_; }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      if (num_ssrcs_ > 1) {
        // Set low simulcast bitrates to not have to wait for bandwidth ramp-up.
        for (size_t i = 0; i < encoder_config->streams.size(); ++i) {
          encoder_config->streams[i].min_bitrate_bps = 10000;
          encoder_config->streams[i].target_bitrate_bps = 15000;
          encoder_config->streams[i].max_bitrate_bps = 20000;
        }
      }

      video_encoder_config_all_streams_ = *encoder_config;
      if (send_single_ssrc_first_)
        encoder_config->streams.resize(1);
    }

    void OnVideoStreamsCreated(
        VideoSendStream* send_stream,
        const std::vector<VideoReceiveStream*>& receive_streams) override {
      send_stream_ = send_stream;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out while waiting for "
                          << (send_single_ssrc_first_ ? "first SSRC."
                                                      : "SSRCs.");

      if (send_single_ssrc_first_) {
        // Set full simulcast and continue with the rest of the SSRCs.
        send_stream_->ReconfigureVideoEncoder(
            video_encoder_config_all_streams_);
        EXPECT_TRUE(Wait()) << "Timed out while waiting on additional SSRCs.";
      }
    }

   private:
    std::map<uint32_t, bool> valid_ssrcs_;
    std::map<uint32_t, bool> is_observed_;

    const size_t num_ssrcs_;
    const bool send_single_ssrc_first_;

    size_t ssrcs_to_observe_;
    bool expect_single_ssrc_;

    VideoSendStream* send_stream_;
    VideoEncoderConfig video_encoder_config_all_streams_;
  } test(kVideoSendSsrcs, num_ssrcs, send_single_ssrc_first);

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ReportsSetEncoderRates) {
  class EncoderRateStatsTest : public test::EndToEndTest,
                               public test::FakeEncoder {
   public:
    EncoderRateStatsTest()
        : EndToEndTest(kDefaultTimeoutMs),
          FakeEncoder(Clock::GetRealTimeClock()),
          send_stream_(nullptr),
          bitrate_kbps_(0) {}

    void OnVideoStreamsCreated(
        VideoSendStream* send_stream,
        const std::vector<VideoReceiveStream*>& receive_streams) override {
      send_stream_ = send_stream;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->encoder_settings.encoder = this;
      RTC_DCHECK_EQ(1u, encoder_config->streams.size());
    }

    int32_t SetRates(uint32_t new_target_bitrate, uint32_t framerate) override {
      // Make sure not to trigger on any default zero bitrates.
      if (new_target_bitrate == 0)
        return 0;
      rtc::CritScope lock(&crit_);
      bitrate_kbps_ = new_target_bitrate;
      observation_complete_.Set();
      return 0;
    }

    void PerformTest() override {
      ASSERT_TRUE(Wait())
          << "Timed out while waiting for encoder SetRates() call.";
      // Wait for GetStats to report a corresponding bitrate.
      for (int i = 0; i < kDefaultTimeoutMs; ++i) {
        VideoSendStream::Stats stats = send_stream_->GetStats();
        {
          rtc::CritScope lock(&crit_);
          if ((stats.target_media_bitrate_bps + 500) / 1000 ==
              static_cast<int>(bitrate_kbps_)) {
            return;
          }
        }
        SleepMs(1);
      }
      FAIL()
          << "Timed out waiting for stats reporting the currently set bitrate.";
    }

   private:
    rtc::CriticalSection crit_;
    VideoSendStream* send_stream_;
    uint32_t bitrate_kbps_ GUARDED_BY(crit_);
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, GetStats) {
  static const int kStartBitrateBps = 3000000;
  static const int kExpectedRenderDelayMs = 20;

  class ReceiveStreamRenderer : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    ReceiveStreamRenderer() {}

   private:
    void OnFrame(const VideoFrame& video_frame) override {}
  };

  class StatsObserver : public test::EndToEndTest,
                        public rtc::VideoSinkInterface<VideoFrame> {
   public:
    StatsObserver()
        : EndToEndTest(kLongTimeoutMs),
          encoder_(Clock::GetRealTimeClock(), 10),
          send_stream_(nullptr),
          expected_send_ssrcs_(),
          check_stats_event_(false, false) {}

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      check_stats_event_.Set();
      return SEND_PACKET;
    }

    Action OnSendRtcp(const uint8_t* packet, size_t length) override {
      check_stats_event_.Set();
      return SEND_PACKET;
    }

    Action OnReceiveRtp(const uint8_t* packet, size_t length) override {
      check_stats_event_.Set();
      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      check_stats_event_.Set();
      return SEND_PACKET;
    }

    void OnFrame(const VideoFrame& video_frame) override {
      // Ensure that we have at least 5ms send side delay.
      SleepMs(5);
    }

    bool CheckReceiveStats() {
      for (size_t i = 0; i < receive_streams_.size(); ++i) {
        VideoReceiveStream::Stats stats = receive_streams_[i]->GetStats();
        EXPECT_EQ(expected_receive_ssrcs_[i], stats.ssrc);

        // Make sure all fields have been populated.
        // TODO(pbos): Use CompoundKey if/when we ever know that all stats are
        // always filled for all receivers.
        receive_stats_filled_["IncomingRate"] |=
            stats.network_frame_rate != 0 || stats.total_bitrate_bps != 0;

        send_stats_filled_["DecoderImplementationName"] |=
            stats.decoder_implementation_name ==
            test::FakeDecoder::kImplementationName;
        receive_stats_filled_["RenderDelayAsHighAsExpected"] |=
            stats.render_delay_ms >= kExpectedRenderDelayMs;

        receive_stats_filled_["FrameCallback"] |= stats.decode_frame_rate != 0;

        receive_stats_filled_["FrameRendered"] |= stats.render_frame_rate != 0;

        receive_stats_filled_["StatisticsUpdated"] |=
            stats.rtcp_stats.cumulative_lost != 0 ||
            stats.rtcp_stats.extended_max_sequence_number != 0 ||
            stats.rtcp_stats.fraction_lost != 0 || stats.rtcp_stats.jitter != 0;

        receive_stats_filled_["DataCountersUpdated"] |=
            stats.rtp_stats.transmitted.payload_bytes != 0 ||
            stats.rtp_stats.fec.packets != 0 ||
            stats.rtp_stats.transmitted.header_bytes != 0 ||
            stats.rtp_stats.transmitted.packets != 0 ||
            stats.rtp_stats.transmitted.padding_bytes != 0 ||
            stats.rtp_stats.retransmitted.packets != 0;

        receive_stats_filled_["CodecStats"] |=
            stats.target_delay_ms != 0 || stats.discarded_packets != 0;

        receive_stats_filled_["FrameCounts"] |=
            stats.frame_counts.key_frames != 0 ||
            stats.frame_counts.delta_frames != 0;

        receive_stats_filled_["CName"] |= !stats.c_name.empty();

        receive_stats_filled_["RtcpPacketTypeCount"] |=
            stats.rtcp_packet_type_counts.fir_packets != 0 ||
            stats.rtcp_packet_type_counts.nack_packets != 0 ||
            stats.rtcp_packet_type_counts.pli_packets != 0 ||
            stats.rtcp_packet_type_counts.nack_requests != 0 ||
            stats.rtcp_packet_type_counts.unique_nack_requests != 0;

        assert(stats.current_payload_type == -1 ||
               stats.current_payload_type == kFakeVideoSendPayloadType);
        receive_stats_filled_["IncomingPayloadType"] |=
            stats.current_payload_type == kFakeVideoSendPayloadType;
      }

      return AllStatsFilled(receive_stats_filled_);
    }

    bool CheckSendStats() {
      RTC_DCHECK(send_stream_);
      VideoSendStream::Stats stats = send_stream_->GetStats();

      send_stats_filled_["NumStreams"] |=
          stats.substreams.size() == expected_send_ssrcs_.size();

      send_stats_filled_["CpuOveruseMetrics"] |=
          stats.avg_encode_time_ms != 0 && stats.encode_usage_percent != 0;

      send_stats_filled_["EncoderImplementationName"] |=
          stats.encoder_implementation_name ==
          test::FakeEncoder::kImplementationName;

      for (std::map<uint32_t, VideoSendStream::StreamStats>::const_iterator it =
               stats.substreams.begin();
           it != stats.substreams.end(); ++it) {
        EXPECT_TRUE(expected_send_ssrcs_.find(it->first) !=
                    expected_send_ssrcs_.end());

        send_stats_filled_[CompoundKey("CapturedFrameRate", it->first)] |=
            stats.input_frame_rate != 0;

        const VideoSendStream::StreamStats& stream_stats = it->second;

        send_stats_filled_[CompoundKey("StatisticsUpdated", it->first)] |=
            stream_stats.rtcp_stats.cumulative_lost != 0 ||
            stream_stats.rtcp_stats.extended_max_sequence_number != 0 ||
            stream_stats.rtcp_stats.fraction_lost != 0;

        send_stats_filled_[CompoundKey("DataCountersUpdated", it->first)] |=
            stream_stats.rtp_stats.fec.packets != 0 ||
            stream_stats.rtp_stats.transmitted.padding_bytes != 0 ||
            stream_stats.rtp_stats.retransmitted.packets != 0 ||
            stream_stats.rtp_stats.transmitted.packets != 0;

        send_stats_filled_[CompoundKey("BitrateStatisticsObserver",
                                       it->first)] |=
            stream_stats.total_bitrate_bps != 0;

        send_stats_filled_[CompoundKey("FrameCountObserver", it->first)] |=
            stream_stats.frame_counts.delta_frames != 0 ||
            stream_stats.frame_counts.key_frames != 0;

        send_stats_filled_[CompoundKey("OutgoingRate", it->first)] |=
            stats.encode_frame_rate != 0;

        send_stats_filled_[CompoundKey("Delay", it->first)] |=
            stream_stats.avg_delay_ms != 0 || stream_stats.max_delay_ms != 0;

        // TODO(pbos): Use CompoundKey when the test makes sure that all SSRCs
        // report dropped packets.
        send_stats_filled_["RtcpPacketTypeCount"] |=
            stream_stats.rtcp_packet_type_counts.fir_packets != 0 ||
            stream_stats.rtcp_packet_type_counts.nack_packets != 0 ||
            stream_stats.rtcp_packet_type_counts.pli_packets != 0 ||
            stream_stats.rtcp_packet_type_counts.nack_requests != 0 ||
            stream_stats.rtcp_packet_type_counts.unique_nack_requests != 0;
      }

      return AllStatsFilled(send_stats_filled_);
    }

    std::string CompoundKey(const char* name, uint32_t ssrc) {
      std::ostringstream oss;
      oss << name << "_" << ssrc;
      return oss.str();
    }

    bool AllStatsFilled(const std::map<std::string, bool>& stats_map) {
      for (std::map<std::string, bool>::const_iterator it = stats_map.begin();
           it != stats_map.end();
           ++it) {
        if (!it->second)
          return false;
      }
      return true;
    }

    test::PacketTransport* CreateSendTransport(Call* sender_call) override {
      FakeNetworkPipe::Config network_config;
      network_config.loss_percent = 5;
      return new test::PacketTransport(
          sender_call, this, test::PacketTransport::kSender, network_config);
    }

    Call::Config GetSenderCallConfig() override {
      Call::Config config = EndToEndTest::GetSenderCallConfig();
      config.bitrate_config.start_bitrate_bps = kStartBitrateBps;
      return config;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->pre_encode_callback = this;  // Used to inject delay.
      expected_cname_ = send_config->rtp.c_name = "SomeCName";

      const std::vector<uint32_t>& ssrcs = send_config->rtp.ssrcs;
      for (size_t i = 0; i < ssrcs.size(); ++i) {
        expected_send_ssrcs_.insert(ssrcs[i]);
        expected_receive_ssrcs_.push_back(
            (*receive_configs)[i].rtp.remote_ssrc);
        (*receive_configs)[i].render_delay_ms = kExpectedRenderDelayMs;
        (*receive_configs)[i].renderer = &receive_stream_renderer_;
      }
      // Use a delayed encoder to make sure we see CpuOveruseMetrics stats that
      // are non-zero.
      send_config->encoder_settings.encoder = &encoder_;
    }

    size_t GetNumVideoStreams() const override { return kNumSsrcs; }

    void OnVideoStreamsCreated(
        VideoSendStream* send_stream,
        const std::vector<VideoReceiveStream*>& receive_streams) override {
      send_stream_ = send_stream;
      receive_streams_ = receive_streams;
    }

    void PerformTest() override {
      Clock* clock = Clock::GetRealTimeClock();
      int64_t now = clock->TimeInMilliseconds();
      int64_t stop_time = now + test::CallTest::kLongTimeoutMs;
      bool receive_ok = false;
      bool send_ok = false;

      while (now < stop_time) {
        if (!receive_ok)
          receive_ok = CheckReceiveStats();
        if (!send_ok)
          send_ok = CheckSendStats();

        if (receive_ok && send_ok)
          return;

        int64_t time_until_timout_ = stop_time - now;
        if (time_until_timout_ > 0)
          check_stats_event_.Wait(time_until_timout_);
        now = clock->TimeInMilliseconds();
      }

      ADD_FAILURE() << "Timed out waiting for filled stats.";
      for (std::map<std::string, bool>::const_iterator it =
               receive_stats_filled_.begin();
           it != receive_stats_filled_.end();
           ++it) {
        if (!it->second) {
          ADD_FAILURE() << "Missing receive stats: " << it->first;
        }
      }

      for (std::map<std::string, bool>::const_iterator it =
               send_stats_filled_.begin();
           it != send_stats_filled_.end();
           ++it) {
        if (!it->second) {
          ADD_FAILURE() << "Missing send stats: " << it->first;
        }
      }
    }

    test::DelayedEncoder encoder_;
    std::vector<VideoReceiveStream*> receive_streams_;
    std::map<std::string, bool> receive_stats_filled_;

    VideoSendStream* send_stream_;
    std::map<std::string, bool> send_stats_filled_;

    std::vector<uint32_t> expected_receive_ssrcs_;
    std::set<uint32_t> expected_send_ssrcs_;
    std::string expected_cname_;

    rtc::Event check_stats_event_;
    ReceiveStreamRenderer receive_stream_renderer_;
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, ReceiverReferenceTimeReportEnabled) {
  TestXrReceiverReferenceTimeReport(true);
}

TEST_F(EndToEndTest, ReceiverReferenceTimeReportDisabled) {
  TestXrReceiverReferenceTimeReport(false);
}

TEST_F(EndToEndTest, TestReceivedRtpPacketStats) {
  static const size_t kNumRtpPacketsToSend = 5;
  class ReceivedRtpStatsObserver : public test::EndToEndTest {
   public:
    ReceivedRtpStatsObserver()
        : EndToEndTest(kDefaultTimeoutMs),
          receive_stream_(nullptr),
          sent_rtp_(0) {}

   private:
    void OnVideoStreamsCreated(
        VideoSendStream* send_stream,
        const std::vector<VideoReceiveStream*>& receive_streams) override {
      receive_stream_ = receive_streams[0];
    }

    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      if (sent_rtp_ >= kNumRtpPacketsToSend) {
        VideoReceiveStream::Stats stats = receive_stream_->GetStats();
        if (kNumRtpPacketsToSend == stats.rtp_stats.transmitted.packets) {
          observation_complete_.Set();
        }
        return DROP_PACKET;
      }
      ++sent_rtp_;
      return SEND_PACKET;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while verifying number of received RTP packets.";
    }

    VideoReceiveStream* receive_stream_;
    uint32_t sent_rtp_;
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, SendsSetSsrc) { TestSendsSetSsrcs(1, false); }

TEST_F(EndToEndTest, SendsSetSimulcastSsrcs) {
  TestSendsSetSsrcs(kNumSsrcs, false);
}

TEST_F(EndToEndTest, CanSwitchToUseAllSsrcs) {
  TestSendsSetSsrcs(kNumSsrcs, true);
}

TEST_F(EndToEndTest, DISABLED_RedundantPayloadsTransmittedOnAllSsrcs) {
  class ObserveRedundantPayloads: public test::EndToEndTest {
   public:
    ObserveRedundantPayloads()
        : EndToEndTest(kDefaultTimeoutMs), ssrcs_to_observe_(kNumSsrcs) {
          for (size_t i = 0; i < kNumSsrcs; ++i) {
            registered_rtx_ssrc_[kSendRtxSsrcs[i]] = true;
          }
        }

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));

      if (!registered_rtx_ssrc_[header.ssrc])
        return SEND_PACKET;

      EXPECT_LE(header.headerLength + header.paddingLength, length);
      const bool packet_is_redundant_payload =
          header.headerLength + header.paddingLength < length;

      if (!packet_is_redundant_payload)
        return SEND_PACKET;

      if (!observed_redundant_retransmission_[header.ssrc]) {
        observed_redundant_retransmission_[header.ssrc] = true;
        if (--ssrcs_to_observe_ == 0)
          observation_complete_.Set();
      }

      return SEND_PACKET;
    }

    size_t GetNumVideoStreams() const override { return kNumSsrcs; }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      // Set low simulcast bitrates to not have to wait for bandwidth ramp-up.
      for (size_t i = 0; i < encoder_config->streams.size(); ++i) {
        encoder_config->streams[i].min_bitrate_bps = 10000;
        encoder_config->streams[i].target_bitrate_bps = 15000;
        encoder_config->streams[i].max_bitrate_bps = 20000;
      }

      send_config->rtp.rtx.payload_type = kSendRtxPayloadType;

      for (size_t i = 0; i < kNumSsrcs; ++i)
        send_config->rtp.rtx.ssrcs.push_back(kSendRtxSsrcs[i]);

      // Significantly higher than max bitrates for all video streams -> forcing
      // padding to trigger redundant padding on all RTX SSRCs.
      encoder_config->min_transmit_bitrate_bps = 100000;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait())
          << "Timed out while waiting for redundant payloads on all SSRCs.";
    }

   private:
    size_t ssrcs_to_observe_;
    std::map<uint32_t, bool> observed_redundant_retransmission_;
    std::map<uint32_t, bool> registered_rtx_ssrc_;
  } test;

  RunBaseTest(&test);
}

void EndToEndTest::TestRtpStatePreservation(bool use_rtx) {
  class RtpSequenceObserver : public test::RtpRtcpObserver {
   public:
    explicit RtpSequenceObserver(bool use_rtx)
        : test::RtpRtcpObserver(kDefaultTimeoutMs),
          ssrcs_to_observe_(kNumSsrcs) {
      for (size_t i = 0; i < kNumSsrcs; ++i) {
        configured_ssrcs_[kVideoSendSsrcs[i]] = true;
        if (use_rtx)
          configured_ssrcs_[kSendRtxSsrcs[i]] = true;
      }
    }

    void ResetExpectedSsrcs(size_t num_expected_ssrcs) {
      rtc::CritScope lock(&crit_);
      ssrc_observed_.clear();
      ssrcs_to_observe_ = num_expected_ssrcs;
    }

   private:
    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));
      const uint32_t ssrc = header.ssrc;
      const int64_t sequence_number =
          seq_numbers_unwrapper_.Unwrap(header.sequenceNumber);
      const uint32_t timestamp = header.timestamp;
      const bool only_padding =
          header.headerLength + header.paddingLength == length;

      EXPECT_TRUE(configured_ssrcs_[ssrc])
          << "Received SSRC that wasn't configured: " << ssrc;

      static const int64_t kMaxSequenceNumberGap = 100;
      std::list<int64_t>* seq_numbers = &last_observed_seq_numbers_[ssrc];
      if (seq_numbers->empty()) {
        seq_numbers->push_back(sequence_number);
      } else {
        // We shouldn't get replays of previous sequence numbers.
        for (int64_t observed : *seq_numbers) {
          EXPECT_NE(observed, sequence_number)
              << "Received sequence number " << sequence_number
              << " for SSRC " << ssrc << " 2nd time.";
        }
        // Verify sequence numbers are reasonably close.
        int64_t latest_observed = seq_numbers->back();
        int64_t sequence_number_gap = sequence_number - latest_observed;
        EXPECT_LE(std::abs(sequence_number_gap), kMaxSequenceNumberGap)
            << "Gap in sequence numbers (" << latest_observed << " -> "
            << sequence_number << ") too large for SSRC: " << ssrc << ".";
        seq_numbers->push_back(sequence_number);
        if (seq_numbers->size() >= kMaxSequenceNumberGap) {
          seq_numbers->pop_front();
        }
      }

      static const int32_t kMaxTimestampGap = kDefaultTimeoutMs * 90;
      auto timestamp_it = last_observed_timestamp_.find(ssrc);
      if (timestamp_it == last_observed_timestamp_.end()) {
        EXPECT_FALSE(only_padding);
        last_observed_timestamp_[ssrc] = timestamp;
      } else {
        // Verify timestamps are reasonably close.
        uint32_t latest_observed = timestamp_it->second;
        // Wraparound handling is unnecessary here as long as an int variable
        // is used to store the result.
        int32_t timestamp_gap = timestamp - latest_observed;
        EXPECT_LE(std::abs(timestamp_gap), kMaxTimestampGap)
            << "Gap in timestamps (" << latest_observed << " -> "
            << timestamp << ") too large for SSRC: " << ssrc << ".";
        timestamp_it->second = timestamp;
      }

      rtc::CritScope lock(&crit_);
      // Wait for media packets on all ssrcs.
      if (!ssrc_observed_[ssrc] && !only_padding) {
        ssrc_observed_[ssrc] = true;
        if (--ssrcs_to_observe_ == 0)
          observation_complete_.Set();
      }

      return SEND_PACKET;
    }

    SequenceNumberUnwrapper seq_numbers_unwrapper_;
    std::map<uint32_t, std::list<int64_t>> last_observed_seq_numbers_;
    std::map<uint32_t, uint32_t> last_observed_timestamp_;
    std::map<uint32_t, bool> configured_ssrcs_;

    rtc::CriticalSection crit_;
    size_t ssrcs_to_observe_ GUARDED_BY(crit_);
    std::map<uint32_t, bool> ssrc_observed_ GUARDED_BY(crit_);
  } observer(use_rtx);

  CreateCalls(Call::Config(), Call::Config());

  test::PacketTransport send_transport(sender_call_.get(), &observer,
                                       test::PacketTransport::kSender,
                                       FakeNetworkPipe::Config());
  test::PacketTransport receive_transport(nullptr, &observer,
                                          test::PacketTransport::kReceiver,
                                          FakeNetworkPipe::Config());
  send_transport.SetReceiver(receiver_call_->Receiver());
  receive_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(kNumSsrcs, 0, &send_transport);

  if (use_rtx) {
    for (size_t i = 0; i < kNumSsrcs; ++i) {
      video_send_config_.rtp.rtx.ssrcs.push_back(kSendRtxSsrcs[i]);
    }
    video_send_config_.rtp.rtx.payload_type = kSendRtxPayloadType;
  }

  // Lower bitrates so that all streams send initially.
  for (size_t i = 0; i < video_encoder_config_.streams.size(); ++i) {
    video_encoder_config_.streams[i].min_bitrate_bps = 10000;
    video_encoder_config_.streams[i].target_bitrate_bps = 15000;
    video_encoder_config_.streams[i].max_bitrate_bps = 20000;
  }

  // Use the same total bitrates when sending a single stream to avoid lowering
  // the bitrate estimate and requiring a subsequent rampup.
  VideoEncoderConfig one_stream = video_encoder_config_;
  one_stream.streams.resize(1);
  for (size_t i = 1; i < video_encoder_config_.streams.size(); ++i) {
    one_stream.streams.front().min_bitrate_bps +=
        video_encoder_config_.streams[i].min_bitrate_bps;
    one_stream.streams.front().target_bitrate_bps +=
        video_encoder_config_.streams[i].target_bitrate_bps;
    one_stream.streams.front().max_bitrate_bps +=
        video_encoder_config_.streams[i].max_bitrate_bps;
  }

  CreateMatchingReceiveConfigs(&receive_transport);

  CreateVideoStreams();
  CreateFrameGeneratorCapturer();

  Start();
  EXPECT_TRUE(observer.Wait())
      << "Timed out waiting for all SSRCs to send packets.";

  // Test stream resetting more than once to make sure that the state doesn't
  // get set once (this could be due to using std::map::insert for instance).
  for (size_t i = 0; i < 3; ++i) {
    frame_generator_capturer_->Stop();
    sender_call_->DestroyVideoSendStream(video_send_stream_);

    // Re-create VideoSendStream with only one stream.
    video_send_stream_ =
        sender_call_->CreateVideoSendStream(video_send_config_, one_stream);
    video_send_stream_->Start();
    CreateFrameGeneratorCapturer();
    frame_generator_capturer_->Start();

    observer.ResetExpectedSsrcs(1);
    EXPECT_TRUE(observer.Wait()) << "Timed out waiting for single RTP packet.";

    // Reconfigure back to use all streams.
    video_send_stream_->ReconfigureVideoEncoder(video_encoder_config_);
    observer.ResetExpectedSsrcs(kNumSsrcs);
    EXPECT_TRUE(observer.Wait())
        << "Timed out waiting for all SSRCs to send packets.";

    // Reconfigure down to one stream.
    video_send_stream_->ReconfigureVideoEncoder(one_stream);
    observer.ResetExpectedSsrcs(1);
    EXPECT_TRUE(observer.Wait()) << "Timed out waiting for single RTP packet.";

    // Reconfigure back to use all streams.
    video_send_stream_->ReconfigureVideoEncoder(video_encoder_config_);
    observer.ResetExpectedSsrcs(kNumSsrcs);
    EXPECT_TRUE(observer.Wait())
        << "Timed out waiting for all SSRCs to send packets.";
  }

  send_transport.StopSending();
  receive_transport.StopSending();

  Stop();
  DestroyStreams();
}

TEST_F(EndToEndTest, RestartingSendStreamPreservesRtpState) {
  TestRtpStatePreservation(false);
}

#if defined(WEBRTC_MAC)
#define MAYBE_RestartingSendStreamPreservesRtpStatesWithRtx \
  DISABLED_RestartingSendStreamPreservesRtpStatesWithRtx
#else
#define MAYBE_RestartingSendStreamPreservesRtpStatesWithRtx \
  RestartingSendStreamPreservesRtpStatesWithRtx
#endif
TEST_F(EndToEndTest, MAYBE_RestartingSendStreamPreservesRtpStatesWithRtx) {
  TestRtpStatePreservation(true);
}

TEST_F(EndToEndTest, RespectsNetworkState) {
  // TODO(pbos): Remove accepted downtime packets etc. when signaling network
  // down blocks until no more packets will be sent.

  // Pacer will send from its packet list and then send required padding before
  // checking paused_ again. This should be enough for one round of pacing,
  // otherwise increase.
  static const int kNumAcceptedDowntimeRtp = 5;
  // A single RTCP may be in the pipeline.
  static const int kNumAcceptedDowntimeRtcp = 1;
  class NetworkStateTest : public test::EndToEndTest, public test::FakeEncoder {
   public:
    NetworkStateTest()
        : EndToEndTest(kDefaultTimeoutMs),
          FakeEncoder(Clock::GetRealTimeClock()),
          encoded_frames_(false, false),
          packet_event_(false, false),
          sender_call_(nullptr),
          receiver_call_(nullptr),
          sender_state_(kNetworkUp),
          sender_rtp_(0),
          sender_rtcp_(0),
          receiver_rtcp_(0),
          down_frames_(0) {}

    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&test_crit_);
      ++sender_rtp_;
      packet_event_.Set();
      return SEND_PACKET;
    }

    Action OnSendRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&test_crit_);
      ++sender_rtcp_;
      packet_event_.Set();
      return SEND_PACKET;
    }

    Action OnReceiveRtp(const uint8_t* packet, size_t length) override {
      ADD_FAILURE() << "Unexpected receiver RTP, should not be sending.";
      return SEND_PACKET;
    }

    Action OnReceiveRtcp(const uint8_t* packet, size_t length) override {
      rtc::CritScope lock(&test_crit_);
      ++receiver_rtcp_;
      packet_event_.Set();
      return SEND_PACKET;
    }

    void OnCallsCreated(Call* sender_call, Call* receiver_call) override {
      sender_call_ = sender_call;
      receiver_call_ = receiver_call;
    }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->encoder_settings.encoder = this;
    }

    void PerformTest() override {
      EXPECT_TRUE(encoded_frames_.Wait(kDefaultTimeoutMs))
          << "No frames received by the encoder.";
      // Wait for packets from both sender/receiver.
      WaitForPacketsOrSilence(false, false);

      // Sender-side network down for audio; there should be no effect on video
      sender_call_->SignalChannelNetworkState(MediaType::AUDIO, kNetworkDown);
      WaitForPacketsOrSilence(false, false);

      // Receiver-side network down for audio; no change expected
      receiver_call_->SignalChannelNetworkState(MediaType::AUDIO, kNetworkDown);
      WaitForPacketsOrSilence(false, false);

      // Sender-side network down.
      sender_call_->SignalChannelNetworkState(MediaType::VIDEO, kNetworkDown);
      {
        rtc::CritScope lock(&test_crit_);
        // After network goes down we shouldn't be encoding more frames.
        sender_state_ = kNetworkDown;
      }
      // Wait for receiver-packets and no sender packets.
      WaitForPacketsOrSilence(true, false);

      // Receiver-side network down.
      receiver_call_->SignalChannelNetworkState(MediaType::VIDEO, kNetworkDown);
      WaitForPacketsOrSilence(true, true);

      // Network up for audio for both sides; video is still not expected to
      // start
      sender_call_->SignalChannelNetworkState(MediaType::AUDIO, kNetworkUp);
      receiver_call_->SignalChannelNetworkState(MediaType::AUDIO, kNetworkUp);
      WaitForPacketsOrSilence(true, true);

      // Network back up again for both.
      {
        rtc::CritScope lock(&test_crit_);
        // It's OK to encode frames again, as we're about to bring up the
        // network.
        sender_state_ = kNetworkUp;
      }
      sender_call_->SignalChannelNetworkState(MediaType::VIDEO, kNetworkUp);
      receiver_call_->SignalChannelNetworkState(MediaType::VIDEO, kNetworkUp);
      WaitForPacketsOrSilence(false, false);

      // TODO(skvlad): add tests to verify that the audio streams are stopped
      // when the network goes down for audio once the workaround in
      // paced_sender.cc is removed.
    }

    int32_t Encode(const VideoFrame& input_image,
                   const CodecSpecificInfo* codec_specific_info,
                   const std::vector<FrameType>* frame_types) override {
      {
        rtc::CritScope lock(&test_crit_);
        if (sender_state_ == kNetworkDown) {
          ++down_frames_;
          EXPECT_LE(down_frames_, 1)
              << "Encoding more than one frame while network is down.";
          if (down_frames_ > 1)
            encoded_frames_.Set();
        } else {
          encoded_frames_.Set();
        }
      }
      return test::FakeEncoder::Encode(
          input_image, codec_specific_info, frame_types);
    }

   private:
    void WaitForPacketsOrSilence(bool sender_down, bool receiver_down) {
      int64_t initial_time_ms = clock_->TimeInMilliseconds();
      int initial_sender_rtp;
      int initial_sender_rtcp;
      int initial_receiver_rtcp;
      {
        rtc::CritScope lock(&test_crit_);
        initial_sender_rtp = sender_rtp_;
        initial_sender_rtcp = sender_rtcp_;
        initial_receiver_rtcp = receiver_rtcp_;
      }
      bool sender_done = false;
      bool receiver_done = false;
      while (!sender_done || !receiver_done) {
        packet_event_.Wait(kSilenceTimeoutMs);
        int64_t time_now_ms = clock_->TimeInMilliseconds();
        rtc::CritScope lock(&test_crit_);
        if (sender_down) {
          ASSERT_LE(sender_rtp_ - initial_sender_rtp, kNumAcceptedDowntimeRtp)
              << "RTP sent during sender-side downtime.";
          ASSERT_LE(sender_rtcp_ - initial_sender_rtcp,
                    kNumAcceptedDowntimeRtcp)
              << "RTCP sent during sender-side downtime.";
          if (time_now_ms - initial_time_ms >=
              static_cast<int64_t>(kSilenceTimeoutMs)) {
            sender_done = true;
          }
        } else {
          if (sender_rtp_ > initial_sender_rtp + kNumAcceptedDowntimeRtp)
            sender_done = true;
        }
        if (receiver_down) {
          ASSERT_LE(receiver_rtcp_ - initial_receiver_rtcp,
                    kNumAcceptedDowntimeRtcp)
              << "RTCP sent during receiver-side downtime.";
          if (time_now_ms - initial_time_ms >=
              static_cast<int64_t>(kSilenceTimeoutMs)) {
            receiver_done = true;
          }
        } else {
          if (receiver_rtcp_ > initial_receiver_rtcp + kNumAcceptedDowntimeRtcp)
            receiver_done = true;
        }
      }
    }

    rtc::CriticalSection test_crit_;
    rtc::Event encoded_frames_;
    rtc::Event packet_event_;
    Call* sender_call_;
    Call* receiver_call_;
    NetworkState sender_state_ GUARDED_BY(test_crit_);
    int sender_rtp_ GUARDED_BY(test_crit_);
    int sender_rtcp_ GUARDED_BY(test_crit_);
    int receiver_rtcp_ GUARDED_BY(test_crit_);
    int down_frames_ GUARDED_BY(test_crit_);
  } test;

  RunBaseTest(&test);
}

TEST_F(EndToEndTest, CallReportsRttForSender) {
  static const int kSendDelayMs = 30;
  static const int kReceiveDelayMs = 70;

  CreateCalls(Call::Config(), Call::Config());

  FakeNetworkPipe::Config config;
  config.queue_delay_ms = kSendDelayMs;
  test::DirectTransport sender_transport(config, sender_call_.get());
  config.queue_delay_ms = kReceiveDelayMs;
  test::DirectTransport receiver_transport(config, receiver_call_.get());
  sender_transport.SetReceiver(receiver_call_->Receiver());
  receiver_transport.SetReceiver(sender_call_->Receiver());

  CreateSendConfig(1, 0, &sender_transport);
  CreateMatchingReceiveConfigs(&receiver_transport);

  CreateVideoStreams();
  CreateFrameGeneratorCapturer();
  Start();

  int64_t start_time_ms = clock_->TimeInMilliseconds();
  while (true) {
    Call::Stats stats = sender_call_->GetStats();
    ASSERT_GE(start_time_ms + kDefaultTimeoutMs,
              clock_->TimeInMilliseconds())
        << "No RTT stats before timeout!";
    if (stats.rtt_ms != -1) {
      EXPECT_GE(stats.rtt_ms, kSendDelayMs + kReceiveDelayMs);
      break;
    }
    SleepMs(10);
  }

  Stop();
  DestroyStreams();
}

void EndToEndTest::VerifyNewVideoSendStreamsRespectNetworkState(
    MediaType network_to_bring_down,
    VideoEncoder* encoder,
    Transport* transport) {
  CreateSenderCall(Call::Config());
  sender_call_->SignalChannelNetworkState(network_to_bring_down, kNetworkDown);

  CreateSendConfig(1, 0, transport);
  video_send_config_.encoder_settings.encoder = encoder;
  CreateVideoStreams();
  CreateFrameGeneratorCapturer();

  Start();
  SleepMs(kSilenceTimeoutMs);
  Stop();

  DestroyStreams();
}

void EndToEndTest::VerifyNewVideoReceiveStreamsRespectNetworkState(
    MediaType network_to_bring_down,
    Transport* transport) {
  CreateCalls(Call::Config(), Call::Config());
  receiver_call_->SignalChannelNetworkState(network_to_bring_down,
                                            kNetworkDown);

  test::DirectTransport sender_transport(sender_call_.get());
  sender_transport.SetReceiver(receiver_call_->Receiver());
  CreateSendConfig(1, 0, &sender_transport);
  CreateMatchingReceiveConfigs(transport);
  CreateVideoStreams();
  CreateFrameGeneratorCapturer();

  Start();
  SleepMs(kSilenceTimeoutMs);
  Stop();

  sender_transport.StopSending();

  DestroyStreams();
}

TEST_F(EndToEndTest, NewVideoSendStreamsRespectVideoNetworkDown) {
  class UnusedEncoder : public test::FakeEncoder {
   public:
    UnusedEncoder() : FakeEncoder(Clock::GetRealTimeClock()) {}

    int32_t InitEncode(const VideoCodec* config,
                       int32_t number_of_cores,
                       size_t max_payload_size) override {
      EXPECT_GT(config->startBitrate, 0u);
      return 0;
    }
    int32_t Encode(const VideoFrame& input_image,
                   const CodecSpecificInfo* codec_specific_info,
                   const std::vector<FrameType>* frame_types) override {
      ADD_FAILURE() << "Unexpected frame encode.";
      return test::FakeEncoder::Encode(input_image, codec_specific_info,
                                       frame_types);
    }
  };

  UnusedEncoder unused_encoder;
  UnusedTransport unused_transport;
  VerifyNewVideoSendStreamsRespectNetworkState(
      MediaType::VIDEO, &unused_encoder, &unused_transport);
}

TEST_F(EndToEndTest, NewVideoSendStreamsIgnoreAudioNetworkDown) {
  class RequiredEncoder : public test::FakeEncoder {
   public:
    RequiredEncoder()
        : FakeEncoder(Clock::GetRealTimeClock()), encoded_frame_(false) {}
    ~RequiredEncoder() {
      if (!encoded_frame_) {
        ADD_FAILURE() << "Didn't encode an expected frame";
      }
    }
    int32_t Encode(const VideoFrame& input_image,
                   const CodecSpecificInfo* codec_specific_info,
                   const std::vector<FrameType>* frame_types) override {
      encoded_frame_ = true;
      return test::FakeEncoder::Encode(input_image, codec_specific_info,
                                       frame_types);
    }

   private:
    bool encoded_frame_;
  };

  RequiredTransport required_transport(true /*rtp*/, false /*rtcp*/);
  RequiredEncoder required_encoder;
  VerifyNewVideoSendStreamsRespectNetworkState(
      MediaType::AUDIO, &required_encoder, &required_transport);
}

TEST_F(EndToEndTest, NewVideoReceiveStreamsRespectVideoNetworkDown) {
  UnusedTransport transport;
  VerifyNewVideoReceiveStreamsRespectNetworkState(MediaType::VIDEO, &transport);
}

TEST_F(EndToEndTest, NewVideoReceiveStreamsIgnoreAudioNetworkDown) {
  RequiredTransport transport(false /*rtp*/, true /*rtcp*/);
  VerifyNewVideoReceiveStreamsRespectNetworkState(MediaType::AUDIO, &transport);
}

void VerifyEmptyNackConfig(const NackConfig& config) {
  EXPECT_EQ(0, config.rtp_history_ms)
      << "Enabling NACK requires rtcp-fb: nack negotiation.";
}

void VerifyEmptyFecConfig(const FecConfig& config) {
  EXPECT_EQ(-1, config.ulpfec_payload_type)
      << "Enabling FEC requires rtpmap: ulpfec negotiation.";
  EXPECT_EQ(-1, config.red_payload_type)
      << "Enabling FEC requires rtpmap: red negotiation.";
  EXPECT_EQ(-1, config.red_rtx_payload_type)
      << "Enabling RTX in FEC requires rtpmap: rtx negotiation.";
}

TEST_F(EndToEndTest, VerifyDefaultSendConfigParameters) {
  VideoSendStream::Config default_send_config(nullptr);
  EXPECT_EQ(0, default_send_config.rtp.nack.rtp_history_ms)
      << "Enabling NACK require rtcp-fb: nack negotiation.";
  EXPECT_TRUE(default_send_config.rtp.rtx.ssrcs.empty())
      << "Enabling RTX requires rtpmap: rtx negotiation.";
  EXPECT_TRUE(default_send_config.rtp.extensions.empty())
      << "Enabling RTP extensions require negotiation.";

  VerifyEmptyNackConfig(default_send_config.rtp.nack);
  VerifyEmptyFecConfig(default_send_config.rtp.fec);
}

TEST_F(EndToEndTest, VerifyDefaultReceiveConfigParameters) {
  VideoReceiveStream::Config default_receive_config(nullptr);
  EXPECT_EQ(RtcpMode::kCompound, default_receive_config.rtp.rtcp_mode)
      << "Reduced-size RTCP require rtcp-rsize to be negotiated.";
  EXPECT_FALSE(default_receive_config.rtp.remb)
      << "REMB require rtcp-fb: goog-remb to be negotiated.";
  EXPECT_FALSE(
      default_receive_config.rtp.rtcp_xr.receiver_reference_time_report)
      << "RTCP XR settings require rtcp-xr to be negotiated.";
  EXPECT_TRUE(default_receive_config.rtp.rtx.empty())
      << "Enabling RTX requires rtpmap: rtx negotiation.";
  EXPECT_TRUE(default_receive_config.rtp.extensions.empty())
      << "Enabling RTP extensions require negotiation.";

  VerifyEmptyNackConfig(default_receive_config.rtp.nack);
  VerifyEmptyFecConfig(default_receive_config.rtp.fec);
}

TEST_F(EndToEndTest, TransportSeqNumOnAudioAndVideo) {
  static const int kExtensionId = 8;
  class TransportSequenceNumberTest : public test::EndToEndTest {
   public:
    TransportSequenceNumberTest()
        : EndToEndTest(kDefaultTimeoutMs),
          video_observed_(false),
          audio_observed_(false) {
      parser_->RegisterRtpHeaderExtension(kRtpExtensionTransportSequenceNumber,
                                          kExtensionId);
    }

    size_t GetNumVideoStreams() const override { return 1; }
    size_t GetNumAudioStreams() const override { return 1; }

    void ModifyVideoConfigs(
        VideoSendStream::Config* send_config,
        std::vector<VideoReceiveStream::Config>* receive_configs,
        VideoEncoderConfig* encoder_config) override {
      send_config->rtp.extensions.clear();
      send_config->rtp.extensions.push_back(RtpExtension(
          RtpExtension::kTransportSequenceNumberUri, kExtensionId));
      (*receive_configs)[0].rtp.extensions = send_config->rtp.extensions;
    }

    void ModifyAudioConfigs(
        AudioSendStream::Config* send_config,
        std::vector<AudioReceiveStream::Config>* receive_configs) override {
      send_config->rtp.extensions.clear();
      send_config->rtp.extensions.push_back(RtpExtension(
          RtpExtension::kTransportSequenceNumberUri, kExtensionId));
      (*receive_configs)[0].rtp.extensions.clear();
      (*receive_configs)[0].rtp.extensions = send_config->rtp.extensions;
    }

    Action OnSendRtp(const uint8_t* packet, size_t length) override {
      RTPHeader header;
      EXPECT_TRUE(parser_->Parse(packet, length, &header));
      EXPECT_TRUE(header.extension.hasTransportSequenceNumber);
      // Unwrap packet id and verify uniqueness.
      int64_t packet_id =
          unwrapper_.Unwrap(header.extension.transportSequenceNumber);
      EXPECT_TRUE(received_packet_ids_.insert(packet_id).second);

      if (header.ssrc == kVideoSendSsrcs[0])
        video_observed_ = true;
      if (header.ssrc == kAudioSendSsrc)
        audio_observed_ = true;
      if (audio_observed_ && video_observed_ &&
          received_packet_ids_.size() == 50) {
        size_t packet_id_range =
            *received_packet_ids_.rbegin() - *received_packet_ids_.begin() + 1;
        EXPECT_EQ(received_packet_ids_.size(), packet_id_range);
        observation_complete_.Set();
      }
      return SEND_PACKET;
    }

    void PerformTest() override {
      EXPECT_TRUE(Wait()) << "Timed out while waiting for audio and video "
                             "packets with transport sequence number.";
    }

   private:
    bool video_observed_;
    bool audio_observed_;
    SequenceNumberUnwrapper unwrapper_;
    std::set<int64_t> received_packet_ids_;
  } test;

  RunBaseTest(&test);
}
}  // namespace webrtc
