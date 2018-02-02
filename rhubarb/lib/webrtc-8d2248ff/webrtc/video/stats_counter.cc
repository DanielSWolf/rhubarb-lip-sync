/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/stats_counter.h"

#include <algorithm>

#include "webrtc/system_wrappers/include/clock.h"

namespace webrtc {

namespace {
// Periodic time interval for processing samples.
const int64_t kProcessIntervalMs = 2000;
}  // namespace

// Class holding periodically computed metrics.
class AggregatedCounter {
 public:
  AggregatedCounter() : sum_(0) {}
  ~AggregatedCounter() {}

  void Add(int sample) {
    sum_ += sample;
    ++stats_.num_samples;
    if (stats_.num_samples == 1) {
      stats_.min = sample;
      stats_.max = sample;
    }
    stats_.min = std::min(sample, stats_.min);
    stats_.max = std::max(sample, stats_.max);
  }

  AggregatedStats ComputeStats() {
    Compute();
    return stats_;
  }

 private:
  void Compute() {
    if (stats_.num_samples == 0)
      return;

    stats_.average = (sum_ + stats_.num_samples / 2) / stats_.num_samples;
  }
  int64_t sum_;
  AggregatedStats stats_;
};

// StatsCounter class.
StatsCounter::StatsCounter(Clock* clock,
                           bool include_empty_intervals,
                           StatsCounterObserver* observer)
    : max_(0),
      sum_(0),
      num_samples_(0),
      last_sum_(0),
      clock_(clock),
      include_empty_intervals_(include_empty_intervals),
      observer_(observer),
      aggregated_counter_(new AggregatedCounter()),
      last_process_time_ms_(-1) {}

StatsCounter::~StatsCounter() {}

AggregatedStats StatsCounter::GetStats() {
  return aggregated_counter_->ComputeStats();
}

bool StatsCounter::TimeToProcess() {
  int64_t now = clock_->TimeInMilliseconds();
  if (last_process_time_ms_ == -1)
    last_process_time_ms_ = now;

  int64_t diff_ms = now - last_process_time_ms_;
  if (diff_ms < kProcessIntervalMs)
    return false;

  // Advance number of complete kProcessIntervalMs that have passed.
  int64_t num_intervals = diff_ms / kProcessIntervalMs;
  last_process_time_ms_ += num_intervals * kProcessIntervalMs;

  // Add zero for intervals without samples.
  if (include_empty_intervals_) {
    for (int64_t i = 0; i < num_intervals - 1; ++i) {
      aggregated_counter_->Add(0);
      if (observer_)
        observer_->OnMetricUpdated(0);
    }
  }
  return true;
}

void StatsCounter::Set(int sample) {
  TryProcess();
  ++num_samples_;
  sum_ = sample;
}

void StatsCounter::Add(int sample) {
  TryProcess();
  ++num_samples_;
  sum_ += sample;

  if (num_samples_ == 1)
    max_ = sample;
  max_ = std::max(sample, max_);
}

void StatsCounter::TryProcess() {
  if (!TimeToProcess())
    return;

  int metric;
  if (GetMetric(&metric)) {
    aggregated_counter_->Add(metric);
    if (observer_)
      observer_->OnMetricUpdated(metric);
  }
  last_sum_ = sum_;
  sum_ = 0;
  max_ = 0;
  num_samples_ = 0;
}

// StatsCounter sub-classes.
AvgCounter::AvgCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   false,  // |include_empty_intervals|
                   observer) {}

void AvgCounter::Add(int sample) {
  StatsCounter::Add(sample);
}

bool AvgCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0)
    return false;
  *metric = (sum_ + num_samples_ / 2) / num_samples_;
  return true;
}

MaxCounter::MaxCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   false,  // |include_empty_intervals|
                   observer) {}

void MaxCounter::Add(int sample) {
  StatsCounter::Add(sample);
}

bool MaxCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0)
    return false;
  *metric = max_;
  return true;
}

PercentCounter::PercentCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   false,  // |include_empty_intervals|
                   observer) {}

void PercentCounter::Add(bool sample) {
  StatsCounter::Add(sample ? 1 : 0);
}

bool PercentCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0)
    return false;
  *metric = (sum_ * 100 + num_samples_ / 2) / num_samples_;
  return true;
}

PermilleCounter::PermilleCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   false,  // |include_empty_intervals|
                   observer) {}

void PermilleCounter::Add(bool sample) {
  StatsCounter::Add(sample ? 1 : 0);
}

bool PermilleCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0)
    return false;
  *metric = (sum_ * 1000 + num_samples_ / 2) / num_samples_;
  return true;
}

RateCounter::RateCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   true,  // |include_empty_intervals|
                   observer) {}

void RateCounter::Add(int sample) {
  StatsCounter::Add(sample);
}

bool RateCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0)
    return false;
  *metric = (sum_ * 1000 + kProcessIntervalMs / 2) / kProcessIntervalMs;
  return true;
}

RateAccCounter::RateAccCounter(Clock* clock, StatsCounterObserver* observer)
    : StatsCounter(clock,
                   true,  // |include_empty_intervals|
                   observer) {}

void RateAccCounter::Set(int sample) {
  StatsCounter::Set(sample);
}

bool RateAccCounter::GetMetric(int* metric) const {
  if (num_samples_ == 0 || last_sum_ > sum_)
    return false;
  *metric =
      ((sum_ - last_sum_) * 1000 + kProcessIntervalMs / 2) / kProcessIntervalMs;
  return true;
}

}  // namespace webrtc
