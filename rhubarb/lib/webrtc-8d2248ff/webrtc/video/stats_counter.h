/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_STATS_COUNTER_H_
#define WEBRTC_VIDEO_STATS_COUNTER_H_

#include <memory>

#include "webrtc/base/constructormagic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AggregatedCounter;
class Clock;

// |StatsCounterObserver| is called periodically when a metric is updated.
class StatsCounterObserver {
 public:
  virtual void OnMetricUpdated(int sample) = 0;

  virtual ~StatsCounterObserver() {}
};

struct AggregatedStats {
  int64_t num_samples = 0;
  int min = -1;
  int max = -1;
  int average = -1;
  // TODO(asapersson): Consider adding median/percentiles.
};

// Classes which periodically computes a metric.
//
// During a period, |kProcessIntervalMs|, different metrics can be computed e.g:
// - |AvgCounter|: average of samples
// - |PercentCounter|: percentage of samples
// - |PermilleCounter|: permille of samples
//
// Each periodic metric can be either:
// - reported to an |observer| each period
// - aggregated during the call (e.g. min, max, average)
//
//                 periodically computed
//                    GetMetric()            GetMetric()   => AggregatedStats
//                        ^                      ^            (e.g. min/max/avg)
//                        |                      |
// |   *    *  *       *  |  **    *   * *     * | ...
// |<- process interval ->|
//
// (*) - samples
//
//
// Example usage:
//
// AvgCounter counter(&clock, nullptr);
// counter.Add(5);
// counter.Add(1);
// counter.Add(6);   // process interval passed -> GetMetric() avg:4
// counter.Add(7);
// counter.Add(3);   // process interval passed -> GetMetric() avg:5
// counter.Add(10);
// counter.Add(20);  // process interval passed -> GetMetric() avg:15
// AggregatedStats stats = counter.GetStats();
// stats: {min:4, max:15, avg:8}
//

// Note: StatsCounter takes ownership of |observer|.

class StatsCounter {
 public:
  virtual ~StatsCounter();

  virtual bool GetMetric(int* metric) const = 0;

  AggregatedStats GetStats();

 protected:
  StatsCounter(Clock* clock,
               bool include_empty_intervals,
               StatsCounterObserver* observer);

  void Add(int sample);
  void Set(int sample);

  int max_;
  int64_t sum_;
  int64_t num_samples_;
  int64_t last_sum_;

 private:
  bool TimeToProcess();
  void TryProcess();

  Clock* const clock_;
  const bool include_empty_intervals_;
  const std::unique_ptr<StatsCounterObserver> observer_;
  const std::unique_ptr<AggregatedCounter> aggregated_counter_;
  int64_t last_process_time_ms_;
};

// AvgCounter: average of samples
//
//           | *      *      *      | *           *       | ...
//           | Add(5) Add(1) Add(6) | Add(5)      Add(5)  |
// GetMetric | (5 + 1 + 6) / 3      | (5 + 5) / 2         |
//
class AvgCounter : public StatsCounter {
 public:
  AvgCounter(Clock* clock, StatsCounterObserver* observer);
  ~AvgCounter() override {}

  void Add(int sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(AvgCounter);
};

// MaxCounter: maximum of samples
//
//           | *      *      *      | *           *       | ...
//           | Add(5) Add(1) Add(6) | Add(5)      Add(5)  |
// GetMetric | max: (5, 1, 6)       | max: (5, 5)         |
//
class MaxCounter : public StatsCounter {
 public:
  MaxCounter(Clock* clock, StatsCounterObserver* observer);
  ~MaxCounter() override {}

  void Add(int sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(MaxCounter);
};

// PercentCounter: percentage of samples
//
//           | *      *      *      | *           *       | ...
//           | Add(T) Add(F) Add(T) | Add(F)      Add(T)  |
// GetMetric | 100 * 2 / 3          | 100 * 1 / 2         |
//
class PercentCounter : public StatsCounter {
 public:
  PercentCounter(Clock* clock, StatsCounterObserver* observer);
  ~PercentCounter() override {}

  void Add(bool sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(PercentCounter);
};

// PermilleCounter: permille of samples
//
//           | *      *      *      | *         *         | ...
//           | Add(T) Add(F) Add(T) | Add(F)    Add(T)    |
// GetMetric | 1000 *  2 / 3        | 1000 * 1 / 2        |
//
class PermilleCounter : public StatsCounter {
 public:
  PermilleCounter(Clock* clock, StatsCounterObserver* observer);
  ~PermilleCounter() override {}

  void Add(bool sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(PermilleCounter);
};

// RateCounter: units per second
//
//           | *      *      *      | *           *       | ...
//           | Add(5) Add(1) Add(6) | Add(5)      Add(5)  |
//           |<------ 2 sec ------->|                     |
// GetMetric | (5 + 1 + 6) / 2      | (5 + 5) / 2         |
//
class RateCounter : public StatsCounter {
 public:
  RateCounter(Clock* clock, StatsCounterObserver* observer);
  ~RateCounter() override {}

  void Add(int sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(RateCounter);
};

// RateAccCounter: units per second (used for counters)
//
//           | *      *      *      | *         *         | ...
//           | Set(5) Set(6) Set(8) | Set(11)   Set(13)   |
//           |<------ 2 sec ------->|                     |
// GetMetric | 8 / 2                | (13 - 8) / 2        |
//
class RateAccCounter : public StatsCounter {
 public:
  RateAccCounter(Clock* clock, StatsCounterObserver* observer);
  ~RateAccCounter() override {}

  void Set(int sample);

 private:
  bool GetMetric(int* metric) const override;

  RTC_DISALLOW_COPY_AND_ASSIGN(RateAccCounter);
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_STATS_COUNTER_H_
