/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_VIDEO_SEND_STREAM_H_

#include <map>
#include <memory>
#include <vector>

#include "webrtc/call/bitrate_allocator.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/call.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_coding/protection_bitrate_calculator.h"
#include "webrtc/video/encoder_state_feedback.h"
#include "webrtc/video/payload_router.h"
#include "webrtc/video/send_delay_stats.h"
#include "webrtc/video/send_statistics_proxy.h"
#include "webrtc/video/video_capture_input.h"
#include "webrtc/video/vie_encoder.h"
#include "webrtc/video_receive_stream.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {

class BitrateAllocator;
class CallStats;
class CongestionController;
class IvfFileWriter;
class ProcessThread;
class RtpRtcp;
class ViEEncoder;
class VieRemb;
class RtcEventLog;

namespace vcm {
class VideoSender;
}  // namespace vcm

namespace internal {

class VideoSendStream : public webrtc::VideoSendStream,
                        public webrtc::CpuOveruseObserver,
                        public webrtc::BitrateAllocatorObserver,
                        public webrtc::VCMProtectionCallback,
                        public EncodedImageCallback {
 public:
  VideoSendStream(int num_cpu_cores,
                  ProcessThread* module_process_thread,
                  CallStats* call_stats,
                  CongestionController* congestion_controller,
                  BitrateAllocator* bitrate_allocator,
                  SendDelayStats* send_delay_stats,
                  VieRemb* remb,
                  RtcEventLog* event_log,
                  const VideoSendStream::Config& config,
                  const VideoEncoderConfig& encoder_config,
                  const std::map<uint32_t, RtpState>& suspended_ssrcs);

  ~VideoSendStream() override;

  void SignalNetworkState(NetworkState state);
  bool DeliverRtcp(const uint8_t* packet, size_t length);

  // webrtc::VideoSendStream implementation.
  void Start() override;
  void Stop() override;
  VideoCaptureInput* Input() override;
  void ReconfigureVideoEncoder(const VideoEncoderConfig& config) override;
  Stats GetStats() override;

  // webrtc::CpuOveruseObserver implementation.
  void OveruseDetected() override;
  void NormalUsage() override;

  typedef std::map<uint32_t, RtpState> RtpStateMap;
  RtpStateMap GetRtpStates() const;

  int GetPaddingNeededBps() const;

  // Implements BitrateAllocatorObserver.
  void OnBitrateUpdated(uint32_t bitrate_bps,
                        uint8_t fraction_loss,
                        int64_t rtt) override;

 protected:
  // Implements webrtc::VCMProtectionCallback.
  int ProtectionRequest(const FecProtectionParams* delta_params,
                        const FecProtectionParams* key_params,
                        uint32_t* sent_video_rate_bps,
                        uint32_t* sent_nack_rate_bps,
                        uint32_t* sent_fec_rate_bps) override;

 private:
  struct EncoderSettings {
    VideoCodec video_codec;
    VideoEncoderConfig config;
  };

  // Implements EncodedImageCallback. The implementation routes encoded frames
  // to the |payload_router_| and |config.pre_encode_callback| if set.
  // Called on an arbitrary encoder callback thread.
  int32_t Encoded(const EncodedImage& encoded_image,
                  const CodecSpecificInfo* codec_specific_info,
                  const RTPFragmentationHeader* fragmentation) override;

  static bool EncoderThreadFunction(void* obj);
  void EncoderProcess();

  void ConfigureProtection();
  void ConfigureSsrcs();

  SendStatisticsProxy stats_proxy_;
  const VideoSendStream::Config config_;
  std::map<uint32_t, RtpState> suspended_ssrcs_;

  ProcessThread* const module_process_thread_;
  CallStats* const call_stats_;
  CongestionController* const congestion_controller_;
  BitrateAllocator* const bitrate_allocator_;
  VieRemb* const remb_;

  static const bool kEnableFrameRecording = false;
  static const int kMaxLayers = 3;
  std::unique_ptr<IvfFileWriter> file_writers_[kMaxLayers];

  rtc::PlatformThread encoder_thread_;
  rtc::Event encoder_wakeup_event_;
  volatile int stop_encoder_thread_;
  rtc::CriticalSection encoder_settings_crit_;
  std::unique_ptr<EncoderSettings> pending_encoder_settings_
      GUARDED_BY(encoder_settings_crit_);

  enum class State {
    kStopped,  // VideoSendStream::Start has not yet been called.
    kStarted,  // VideoSendStream::Start has been called.
    // VideoSendStream::Start has been called but the encoder have timed out.
    kEncoderTimedOut,
  };
  rtc::Optional<State> pending_state_change_ GUARDED_BY(encoder_settings_crit_);

  // Only used on the encoder thread.
  rtc::ThreadChecker encoder_thread_checker_;
  State state_ ACCESS_ON(&encoder_thread_checker_);
  std::unique_ptr<EncoderSettings> current_encoder_settings_
      ACCESS_ON(&encoder_thread_checker_);

  OveruseFrameDetector overuse_detector_;
  ViEEncoder vie_encoder_;
  EncoderStateFeedback encoder_feedback_;
  ProtectionBitrateCalculator protection_bitrate_calculator_;

  vcm::VideoSender* const video_sender_;

  const std::unique_ptr<RtcpBandwidthObserver> bandwidth_observer_;
  // RtpRtcp modules, declared here as they use other members on construction.
  const std::vector<RtpRtcp*> rtp_rtcp_modules_;
  PayloadRouter payload_router_;
  VideoCaptureInput input_;
};
}  // namespace internal
}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIDEO_SEND_STREAM_H_
