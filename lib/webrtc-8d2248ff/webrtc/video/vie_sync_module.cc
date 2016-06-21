/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/vie_sync_module.h"

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/trace_event.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_receiver.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/video_coding/video_coding_impl.h"
#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/video/stream_synchronization.h"
#include "webrtc/video_frame.h"
#include "webrtc/voice_engine/include/voe_video_sync.h"

namespace webrtc {
namespace {
int UpdateMeasurements(StreamSynchronization::Measurements* stream,
                       const RtpRtcp& rtp_rtcp, const RtpReceiver& receiver) {
  if (!receiver.Timestamp(&stream->latest_timestamp))
    return -1;
  if (!receiver.LastReceivedTimeMs(&stream->latest_receive_time_ms))
    return -1;

  uint32_t ntp_secs = 0;
  uint32_t ntp_frac = 0;
  uint32_t rtp_timestamp = 0;
  if (rtp_rtcp.RemoteNTP(&ntp_secs, &ntp_frac, nullptr, nullptr,
                         &rtp_timestamp) != 0) {
    return -1;
  }

  bool new_rtcp_sr = false;
  if (!UpdateRtcpList(
      ntp_secs, ntp_frac, rtp_timestamp, &stream->rtcp, &new_rtcp_sr)) {
    return -1;
  }

  return 0;
}
}  // namespace

ViESyncModule::ViESyncModule(vcm::VideoReceiver* video_receiver)
    : video_receiver_(video_receiver),
      clock_(Clock::GetRealTimeClock()),
      rtp_receiver_(nullptr),
      video_rtp_rtcp_(nullptr),
      voe_channel_id_(-1),
      voe_sync_interface_(nullptr),
      last_sync_time_(rtc::TimeNanos()),
      sync_() {}

ViESyncModule::~ViESyncModule() {
}

void ViESyncModule::ConfigureSync(int voe_channel_id,
                                  VoEVideoSync* voe_sync_interface,
                                  RtpRtcp* video_rtcp_module,
                                  RtpReceiver* rtp_receiver) {
  if (voe_channel_id != -1)
    RTC_DCHECK(voe_sync_interface);
  rtc::CritScope lock(&data_cs_);
  // Prevent expensive no-ops.
  if (voe_channel_id_ == voe_channel_id &&
      voe_sync_interface_ == voe_sync_interface &&
      rtp_receiver_ == rtp_receiver && video_rtp_rtcp_ == video_rtcp_module) {
    return;
  }
  voe_channel_id_ = voe_channel_id;
  voe_sync_interface_ = voe_sync_interface;
  rtp_receiver_ = rtp_receiver;
  video_rtp_rtcp_ = video_rtcp_module;
  sync_.reset(
      new StreamSynchronization(video_rtp_rtcp_->SSRC(), voe_channel_id));
}

int64_t ViESyncModule::TimeUntilNextProcess() {
  const int64_t kSyncIntervalMs = 1000;
  return kSyncIntervalMs -
      (rtc::TimeNanos() - last_sync_time_) / rtc::kNumNanosecsPerMillisec;
}

void ViESyncModule::Process() {
  rtc::CritScope lock(&data_cs_);
  last_sync_time_ = rtc::TimeNanos();

  const int current_video_delay_ms = video_receiver_->Delay();

  if (voe_channel_id_ == -1) {
    return;
  }
  assert(video_rtp_rtcp_ && voe_sync_interface_);
  assert(sync_.get());

  int audio_jitter_buffer_delay_ms = 0;
  int playout_buffer_delay_ms = 0;
  if (voe_sync_interface_->GetDelayEstimate(voe_channel_id_,
                                            &audio_jitter_buffer_delay_ms,
                                            &playout_buffer_delay_ms) != 0) {
    return;
  }
  const int current_audio_delay_ms = audio_jitter_buffer_delay_ms +
      playout_buffer_delay_ms;

  RtpRtcp* voice_rtp_rtcp = nullptr;
  RtpReceiver* voice_receiver = nullptr;
  if (voe_sync_interface_->GetRtpRtcp(voe_channel_id_, &voice_rtp_rtcp,
                                      &voice_receiver) != 0) {
    return;
  }
  assert(voice_rtp_rtcp);
  assert(voice_receiver);

  if (UpdateMeasurements(&video_measurement_, *video_rtp_rtcp_,
                         *rtp_receiver_) != 0) {
    return;
  }

  if (UpdateMeasurements(&audio_measurement_, *voice_rtp_rtcp,
                         *voice_receiver) != 0) {
    return;
  }

  int relative_delay_ms;
  // Calculate how much later or earlier the audio stream is compared to video.
  if (!sync_->ComputeRelativeDelay(audio_measurement_, video_measurement_,
                                   &relative_delay_ms)) {
    return;
  }

  TRACE_COUNTER1("webrtc", "SyncCurrentVideoDelay", current_video_delay_ms);
  TRACE_COUNTER1("webrtc", "SyncCurrentAudioDelay", current_audio_delay_ms);
  TRACE_COUNTER1("webrtc", "SyncRelativeDelay", relative_delay_ms);
  int target_audio_delay_ms = 0;
  int target_video_delay_ms = current_video_delay_ms;
  // Calculate the necessary extra audio delay and desired total video
  // delay to get the streams in sync.
  if (!sync_->ComputeDelays(relative_delay_ms,
                            current_audio_delay_ms,
                            &target_audio_delay_ms,
                            &target_video_delay_ms)) {
    return;
  }

  if (voe_sync_interface_->SetMinimumPlayoutDelay(
      voe_channel_id_, target_audio_delay_ms) == -1) {
    LOG(LS_ERROR) << "Error setting voice delay.";
  }
  video_receiver_->SetMinimumPlayoutDelay(target_video_delay_ms);
}

bool ViESyncModule::GetStreamSyncOffsetInMs(const VideoFrame& frame,
                                            int64_t* stream_offset_ms) const {
  rtc::CritScope lock(&data_cs_);
  if (voe_channel_id_ == -1)
    return false;

  uint32_t playout_timestamp = 0;
  if (voe_sync_interface_->GetPlayoutTimestamp(voe_channel_id_,
                                               playout_timestamp) != 0) {
    return false;
  }

  int64_t latest_audio_ntp;
  if (!RtpToNtpMs(playout_timestamp, audio_measurement_.rtcp,
                  &latest_audio_ntp)) {
    return false;
  }

  int64_t latest_video_ntp;
  if (!RtpToNtpMs(frame.timestamp(), video_measurement_.rtcp,
                  &latest_video_ntp)) {
    return false;
  }

  int64_t time_to_render_ms =
      frame.render_time_ms() - clock_->TimeInMilliseconds();
  if (time_to_render_ms > 0)
    latest_video_ntp += time_to_render_ms;

  *stream_offset_ms = latest_audio_ntp - latest_video_ntp;
  return true;
}

}  // namespace webrtc
