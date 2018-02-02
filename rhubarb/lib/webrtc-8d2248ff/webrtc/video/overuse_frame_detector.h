/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_OVERUSE_FRAME_DETECTOR_H_
#define WEBRTC_VIDEO_OVERUSE_FRAME_DETECTOR_H_

#include <list>
#include <memory>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/optional.h"
#include "webrtc/base/exp_filter.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/modules/include/module.h"

namespace webrtc {

class Clock;
class EncodedFrameObserver;
class VideoFrame;

// CpuOveruseObserver is called when a system overuse is detected and
// VideoEngine cannot keep up the encoding frequency.
class CpuOveruseObserver {
 public:
  // Called as soon as an overuse is detected.
  virtual void OveruseDetected() = 0;
  // Called periodically when the system is not overused any longer.
  virtual void NormalUsage() = 0;

 protected:
  virtual ~CpuOveruseObserver() {}
};

struct CpuOveruseOptions {
  CpuOveruseOptions();

  int low_encode_usage_threshold_percent;  // Threshold for triggering underuse.
  int high_encode_usage_threshold_percent;  // Threshold for triggering overuse.
  // General settings.
  int frame_timeout_interval_ms;  // The maximum allowed interval between two
                                  // frames before resetting estimations.
  int min_frame_samples;  // The minimum number of frames required.
  int min_process_count;  // The number of initial process times required before
                          // triggering an overuse/underuse.
  int high_threshold_consecutive_count;  // The number of consecutive checks
                                         // above the high threshold before
                                         // triggering an overuse.
};

struct CpuOveruseMetrics {
  CpuOveruseMetrics() : encode_usage_percent(-1) {}

  int encode_usage_percent;  // Average encode time divided by the average time
                             // difference between incoming captured frames.
};

class CpuOveruseMetricsObserver {
 public:
  virtual ~CpuOveruseMetricsObserver() {}
  virtual void OnEncodedFrameTimeMeasured(int encode_duration_ms,
                                          const CpuOveruseMetrics& metrics) = 0;
};

// Use to detect system overuse based on the send-side processing time of
// incoming frames.
class OveruseFrameDetector : public Module {
 public:
  OveruseFrameDetector(Clock* clock,
                       const CpuOveruseOptions& options,
                       CpuOveruseObserver* overuse_observer,
                       EncodedFrameObserver* encoder_timing_,
                       CpuOveruseMetricsObserver* metrics_observer);
  ~OveruseFrameDetector();

  // Called for each captured frame.
  void FrameCaptured(const VideoFrame& frame);

  // Called for each sent frame.
  void FrameSent(uint32_t timestamp);

  // Implements Module.
  int64_t TimeUntilNextProcess() override;
  void Process() override;

 private:
  class SendProcessingUsage;
  struct FrameTiming {
    FrameTiming(int64_t capture_ntp_ms, uint32_t timestamp, int64_t now)
        : capture_ntp_ms(capture_ntp_ms),
          timestamp(timestamp),
          capture_ms(now),
          last_send_ms(-1) {}
    int64_t capture_ntp_ms;
    uint32_t timestamp;
    int64_t capture_ms;
    int64_t last_send_ms;
  };

  void EncodedFrameTimeMeasured(int encode_duration_ms)
      EXCLUSIVE_LOCKS_REQUIRED(crit_);

  // Only called on the processing thread.
  bool IsOverusing(const CpuOveruseMetrics& metrics);
  bool IsUnderusing(const CpuOveruseMetrics& metrics, int64_t time_now);

  bool FrameTimeoutDetected(int64_t now) const EXCLUSIVE_LOCKS_REQUIRED(crit_);
  bool FrameSizeChanged(int num_pixels) const EXCLUSIVE_LOCKS_REQUIRED(crit_);

  void ResetAll(int num_pixels) EXCLUSIVE_LOCKS_REQUIRED(crit_);

  // Protecting all members except const and those that are only accessed on the
  // processing thread.
  // TODO(asapersson): See if we can reduce locking.  As is, video frame
  // processing contends with reading stats and the processing thread.
  rtc::CriticalSection crit_;

  const CpuOveruseOptions options_;

  // Observer getting overuse reports.
  CpuOveruseObserver* const observer_;
  EncodedFrameObserver* const encoder_timing_;

  // Stats metrics.
  CpuOveruseMetricsObserver* const metrics_observer_;
  rtc::Optional<CpuOveruseMetrics> metrics_ GUARDED_BY(crit_);

  Clock* const clock_;
  int64_t num_process_times_ GUARDED_BY(crit_);

  int64_t last_capture_time_ms_ GUARDED_BY(crit_);
  int64_t last_processed_capture_time_ms_ GUARDED_BY(crit_);

  // Number of pixels of last captured frame.
  int num_pixels_ GUARDED_BY(crit_);

  // These seven members are only accessed on the processing thread.
  int64_t next_process_time_ms_;
  int64_t last_overuse_time_ms_;
  int checks_above_threshold_;
  int num_overuse_detections_;
  int64_t last_rampup_time_ms_;
  bool in_quick_rampup_;
  int current_rampup_delay_ms_;

  // TODO(asapersson): Can these be regular members (avoid separate heap
  // allocs)?
  const std::unique_ptr<SendProcessingUsage> usage_ GUARDED_BY(crit_);
  std::list<FrameTiming> frame_timing_ GUARDED_BY(crit_);

  rtc::ThreadChecker processing_thread_;

  RTC_DISALLOW_COPY_AND_ASSIGN(OveruseFrameDetector);
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_OVERUSE_FRAME_DETECTOR_H_
