/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/vie_encoder.h"

#include <algorithm>
#include <limits>

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/trace_event.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/modules/pacing/paced_sender.h"
#include "webrtc/modules/video_coding/include/video_coding.h"
#include "webrtc/modules/video_coding/include/video_coding_defines.h"
#include "webrtc/system_wrappers/include/metrics.h"
#include "webrtc/video/overuse_frame_detector.h"
#include "webrtc/video/send_statistics_proxy.h"
#include "webrtc/video_frame.h"

namespace webrtc {

ViEEncoder::ViEEncoder(uint32_t number_of_cores,
                       ProcessThread* module_process_thread,
                       SendStatisticsProxy* stats_proxy,
                       OveruseFrameDetector* overuse_detector,
                       EncodedImageCallback* sink)
    : number_of_cores_(number_of_cores),
      sink_(sink),
      vp_(VideoProcessing::Create()),
      video_sender_(Clock::GetRealTimeClock(), this, this, this),
      stats_proxy_(stats_proxy),
      overuse_detector_(overuse_detector),
      time_of_last_frame_activity_ms_(std::numeric_limits<int64_t>::max()),
      encoder_config_(),
      last_observed_bitrate_bps_(0),
      encoder_paused_and_dropped_frame_(false),
      module_process_thread_(module_process_thread),
      has_received_sli_(false),
      picture_id_sli_(0),
      has_received_rpsi_(false),
      picture_id_rpsi_(0),
      video_suspended_(false) {
  module_process_thread_->RegisterModule(&video_sender_);
  vp_->EnableTemporalDecimation(true);
}

vcm::VideoSender* ViEEncoder::video_sender() {
  return &video_sender_;
}

ViEEncoder::~ViEEncoder() {
  module_process_thread_->DeRegisterModule(&video_sender_);
}

int32_t ViEEncoder::RegisterExternalEncoder(webrtc::VideoEncoder* encoder,
                                            uint8_t pl_type,
                                            bool internal_source) {
  video_sender_.RegisterExternalEncoder(encoder, pl_type, internal_source);
  return 0;
}

int32_t ViEEncoder::DeRegisterExternalEncoder(uint8_t pl_type) {
  video_sender_.RegisterExternalEncoder(nullptr, pl_type, false);
  return 0;
}

void ViEEncoder::SetEncoder(const webrtc::VideoCodec& video_codec,
                            size_t max_data_payload_length) {
  // Setting target width and height for VPM.
  RTC_CHECK_EQ(VPM_OK,
               vp_->SetTargetResolution(video_codec.width, video_codec.height,
                                        video_codec.maxFramerate));
  {
    rtc::CritScope lock(&data_cs_);
    encoder_config_ = video_codec;
  }

  bool success = video_sender_.RegisterSendCodec(
                     &video_codec, number_of_cores_,
                     static_cast<uint32_t>(max_data_payload_length)) == VCM_OK;

  if (!success) {
    LOG(LS_ERROR) << "Failed to configure encoder.";
    RTC_DCHECK(success);
  }

  if (stats_proxy_) {
    VideoEncoderConfig::ContentType content_type =
        VideoEncoderConfig::ContentType::kRealtimeVideo;
    switch (video_codec.mode) {
      case kRealtimeVideo:
        content_type = VideoEncoderConfig::ContentType::kRealtimeVideo;
        break;
      case kScreensharing:
        content_type = VideoEncoderConfig::ContentType::kScreen;
        break;
      default:
        RTC_NOTREACHED();
        break;
    }
    stats_proxy_->SetContentType(content_type);
  }
}

bool ViEEncoder::EncoderPaused() const {
  // Pause video if paused by caller or as long as the network is down or the
  // pacer queue has grown too large in buffered mode.
  // If the pacer queue has grown too large or the network is down,
  // last_observed_bitrate_bps_ will be 0.
  return video_suspended_ || last_observed_bitrate_bps_ == 0;
}

void ViEEncoder::TraceFrameDropStart() {
  // Start trace event only on the first frame after encoder is paused.
  if (!encoder_paused_and_dropped_frame_) {
    TRACE_EVENT_ASYNC_BEGIN0("webrtc", "EncoderPaused", this);
  }
  encoder_paused_and_dropped_frame_ = true;
  return;
}

void ViEEncoder::TraceFrameDropEnd() {
  // End trace event on first frame after encoder resumes, if frame was dropped.
  if (encoder_paused_and_dropped_frame_) {
    TRACE_EVENT_ASYNC_END0("webrtc", "EncoderPaused", this);
  }
  encoder_paused_and_dropped_frame_ = false;
}

void ViEEncoder::EncodeVideoFrame(const VideoFrame& video_frame) {
  VideoCodecType codec_type;
  {
    rtc::CritScope lock(&data_cs_);
    time_of_last_frame_activity_ms_ = rtc::TimeMillis();
    if (EncoderPaused()) {
      TraceFrameDropStart();
      return;
    }
    TraceFrameDropEnd();
    codec_type = encoder_config_.codecType;
  }

  TRACE_EVENT_ASYNC_STEP0("webrtc", "Video", video_frame.render_time_ms(),
                          "Encode");
  const VideoFrame* frame_to_send = &video_frame;
  // TODO(wuchengli): support texture frames.
  if (!video_frame.video_frame_buffer()->native_handle()) {
    // Pass frame via preprocessor.
    frame_to_send = vp_->PreprocessFrame(video_frame);
    if (!frame_to_send) {
      // Drop this frame, or there was an error processing it.
      return;
    }
  }

  if (codec_type == webrtc::kVideoCodecVP8) {
    webrtc::CodecSpecificInfo codec_specific_info;
    codec_specific_info.codecType = webrtc::kVideoCodecVP8;
    {
      rtc::CritScope lock(&data_cs_);
      codec_specific_info.codecSpecific.VP8.hasReceivedRPSI =
          has_received_rpsi_;
      codec_specific_info.codecSpecific.VP8.hasReceivedSLI =
          has_received_sli_;
      codec_specific_info.codecSpecific.VP8.pictureIdRPSI =
          picture_id_rpsi_;
      codec_specific_info.codecSpecific.VP8.pictureIdSLI  =
          picture_id_sli_;
      has_received_sli_ = false;
      has_received_rpsi_ = false;
    }

    video_sender_.AddVideoFrame(*frame_to_send, &codec_specific_info);
    return;
  }
  video_sender_.AddVideoFrame(*frame_to_send, nullptr);
}

void ViEEncoder::SendKeyFrame() {
  video_sender_.IntraFrameRequest(0);
}

int64_t ViEEncoder::time_of_last_frame_activity_ms() {
  rtc::CritScope lock(&data_cs_);
  return time_of_last_frame_activity_ms_;
}

void ViEEncoder::OnSetRates(uint32_t bitrate_bps, int framerate) {
  if (stats_proxy_)
    stats_proxy_->OnSetRates(bitrate_bps, framerate);
}

int32_t ViEEncoder::Encoded(const EncodedImage& encoded_image,
                            const CodecSpecificInfo* codec_specific_info,
                            const RTPFragmentationHeader* fragmentation) {
  {
    rtc::CritScope lock(&data_cs_);
    time_of_last_frame_activity_ms_ = rtc::TimeMillis();
  }
  if (stats_proxy_) {
    stats_proxy_->OnSendEncodedImage(encoded_image, codec_specific_info);
  }

  int success =
      sink_->Encoded(encoded_image, codec_specific_info, fragmentation);

  overuse_detector_->FrameSent(encoded_image._timeStamp);
  return success;
}

void ViEEncoder::SendStatistics(uint32_t bit_rate,
                                uint32_t frame_rate,
                                const std::string& encoder_name) {
  if (stats_proxy_)
    stats_proxy_->OnEncoderStatsUpdate(frame_rate, bit_rate, encoder_name);
}

void ViEEncoder::OnReceivedSLI(uint8_t picture_id) {
  rtc::CritScope lock(&data_cs_);
  picture_id_sli_ = picture_id;
  has_received_sli_ = true;
}

void ViEEncoder::OnReceivedRPSI(uint64_t picture_id) {
  rtc::CritScope lock(&data_cs_);
  picture_id_rpsi_ = picture_id;
  has_received_rpsi_ = true;
}

void ViEEncoder::OnReceivedIntraFrameRequest(size_t stream_index) {
  // Key frame request from remote side, signal to VCM.
  TRACE_EVENT0("webrtc", "OnKeyFrameRequest");
  video_sender_.IntraFrameRequest(stream_index);
}

void ViEEncoder::OnBitrateUpdated(uint32_t bitrate_bps,
                                  uint8_t fraction_lost,
                                  int64_t round_trip_time_ms) {
  LOG(LS_VERBOSE) << "OnBitrateUpdated, bitrate " << bitrate_bps
                  << " packet loss " << static_cast<int>(fraction_lost)
                  << " rtt " << round_trip_time_ms;
  video_sender_.SetChannelParameters(bitrate_bps, fraction_lost,
                                     round_trip_time_ms);
  bool video_suspension_changed;
  bool video_is_suspended = bitrate_bps == 0;
  {
    rtc::CritScope lock(&data_cs_);
    last_observed_bitrate_bps_ = bitrate_bps;
    video_suspension_changed = video_suspended_ != video_is_suspended;
    video_suspended_ = video_is_suspended;
    // Set |time_of_last_frame_activity_ms_| to now if this is the first time
    // the encoder is supposed to produce encoded frames.
    // TODO(perkj): Remove this hack. It is here to avoid a race that the
    // encoder report that it has timed out before it has processed the first
    // frame.
    if (last_observed_bitrate_bps_ != 0 &&
        time_of_last_frame_activity_ms_ ==
            std::numeric_limits<int64_t>::max()) {
      time_of_last_frame_activity_ms_ = rtc::TimeMillis();
    }
  }

  if (stats_proxy_ && video_suspension_changed) {
    LOG(LS_INFO) << "Video suspend state changed " << video_is_suspended;
    stats_proxy_->OnSuspendChange(video_is_suspended);
  }
}

}  // namespace webrtc
