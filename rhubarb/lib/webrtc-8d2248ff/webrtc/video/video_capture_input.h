/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_VIDEO_CAPTURE_INPUT_H_
#define WEBRTC_VIDEO_VIDEO_CAPTURE_INPUT_H_

#include <memory>
#include <vector>

#include "webrtc/base/criticalsection.h"
#include "webrtc/base/event.h"
#include "webrtc/base/platform_thread.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/video_capture/video_capture.h"
#include "webrtc/modules/video_coding/include/video_codec_interface.h"
#include "webrtc/modules/video_coding/include/video_coding.h"
#include "webrtc/modules/video_processing/include/video_processing.h"
#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {

class Config;
class OveruseFrameDetector;
class SendStatisticsProxy;

namespace internal {
class VideoCaptureInput : public webrtc::VideoCaptureInput {
 public:
  VideoCaptureInput(rtc::Event* capture_event,
                    rtc::VideoSinkInterface<VideoFrame>* local_renderer,
                    SendStatisticsProxy* send_stats_proxy,
                    OveruseFrameDetector* overuse_detector);
  ~VideoCaptureInput();

  void IncomingCapturedFrame(const VideoFrame& video_frame) override;

  bool GetVideoFrame(VideoFrame* frame);

 private:
  rtc::CriticalSection crit_;

  rtc::VideoSinkInterface<VideoFrame>* const local_renderer_;
  SendStatisticsProxy* const stats_proxy_;
  rtc::Event* const capture_event_;

  std::unique_ptr<VideoFrame> captured_frame_ GUARDED_BY(crit_);
  Clock* const clock_;
  // Used to make sure incoming time stamp is increasing for every frame.
  int64_t last_captured_timestamp_;
  // Delta used for translating between NTP and internal timestamps.
  const int64_t delta_ntp_internal_ms_;

  OveruseFrameDetector* const overuse_detector_;
};

}  // namespace internal
}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIDEO_CAPTURE_INPUT_H_
