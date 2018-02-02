/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/video_capture_input.h"

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/trace_event.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/modules/video_processing/include/video_processing.h"
#include "webrtc/video/overuse_frame_detector.h"
#include "webrtc/video/send_statistics_proxy.h"
#include "webrtc/video/vie_encoder.h"

namespace webrtc {

namespace internal {
VideoCaptureInput::VideoCaptureInput(
    rtc::Event* capture_event,
    rtc::VideoSinkInterface<VideoFrame>* local_renderer,
    SendStatisticsProxy* stats_proxy,
    OveruseFrameDetector* overuse_detector)
    : local_renderer_(local_renderer),
      stats_proxy_(stats_proxy),
      capture_event_(capture_event),
      // TODO(danilchap): Pass clock from outside to ensure it is same clock
      // rtcp module use to calculate offset since last frame captured
      // to estimate rtp timestamp for SenderReport.
      clock_(Clock::GetRealTimeClock()),
      last_captured_timestamp_(0),
      delta_ntp_internal_ms_(clock_->CurrentNtpInMilliseconds() -
                             clock_->TimeInMilliseconds()),
      overuse_detector_(overuse_detector) {}

VideoCaptureInput::~VideoCaptureInput() {
}

void VideoCaptureInput::IncomingCapturedFrame(const VideoFrame& video_frame) {
  // TODO(pbos): Remove local rendering, it should be handled by the client code
  // if required.
  if (local_renderer_)
    local_renderer_->OnFrame(video_frame);

  stats_proxy_->OnIncomingFrame(video_frame.width(), video_frame.height());

  VideoFrame incoming_frame = video_frame;

  // Local time in webrtc time base.
  int64_t current_time = clock_->TimeInMilliseconds();
  incoming_frame.set_render_time_ms(current_time);

  // Capture time may come from clock with an offset and drift from clock_.
  int64_t capture_ntp_time_ms;
  if (video_frame.ntp_time_ms() != 0) {
    capture_ntp_time_ms = video_frame.ntp_time_ms();
  } else if (video_frame.render_time_ms() != 0) {
    capture_ntp_time_ms = video_frame.render_time_ms() + delta_ntp_internal_ms_;
  } else {
    capture_ntp_time_ms = current_time + delta_ntp_internal_ms_;
  }
  incoming_frame.set_ntp_time_ms(capture_ntp_time_ms);

  // Convert NTP time, in ms, to RTP timestamp.
  const int kMsToRtpTimestamp = 90;
  incoming_frame.set_timestamp(
      kMsToRtpTimestamp * static_cast<uint32_t>(incoming_frame.ntp_time_ms()));

  rtc::CritScope lock(&crit_);
  if (incoming_frame.ntp_time_ms() <= last_captured_timestamp_) {
    // We don't allow the same capture time for two frames, drop this one.
    LOG(LS_WARNING) << "Same/old NTP timestamp ("
                    << incoming_frame.ntp_time_ms()
                    << " <= " << last_captured_timestamp_
                    << ") for incoming frame. Dropping.";
    return;
  }

  captured_frame_.reset(new VideoFrame);
  captured_frame_->ShallowCopy(incoming_frame);
  last_captured_timestamp_ = incoming_frame.ntp_time_ms();

  overuse_detector_->FrameCaptured(*captured_frame_);

  TRACE_EVENT_ASYNC_BEGIN1("webrtc", "Video", video_frame.render_time_ms(),
                           "render_time", video_frame.render_time_ms());

  capture_event_->Set();
}

bool VideoCaptureInput::GetVideoFrame(VideoFrame* video_frame) {
  rtc::CritScope lock(&crit_);
  if (!captured_frame_)
    return false;

  *video_frame = *captured_frame_;
  captured_frame_.reset();
  return true;
}

}  // namespace internal
}  // namespace webrtc
