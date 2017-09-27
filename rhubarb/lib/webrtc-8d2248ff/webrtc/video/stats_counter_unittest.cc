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

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/system_wrappers/include/clock.h"

namespace webrtc {
namespace {
const int kProcessIntervalMs = 2000;

class StatsCounterObserverImpl : public StatsCounterObserver {
 public:
  StatsCounterObserverImpl() : num_calls_(0), last_sample_(-1) {}
  void OnMetricUpdated(int sample) override {
    ++num_calls_;
    last_sample_ = sample;
  }
  int num_calls_;
  int last_sample_;
};
}  // namespace

class StatsCounterTest : public ::testing::Test {
 protected:
  StatsCounterTest()
      : clock_(1234) {}

  void AddSampleAndAdvance(int sample, int interval_ms, AvgCounter* counter) {
    counter->Add(sample);
    clock_.AdvanceTimeMilliseconds(interval_ms);
  }

  void SetSampleAndAdvance(int sample,
                           int interval_ms,
                           RateAccCounter* counter) {
    counter->Set(sample);
    clock_.AdvanceTimeMilliseconds(interval_ms);
  }

  void VerifyStatsIsNotSet(const AggregatedStats& stats) {
    EXPECT_EQ(0, stats.num_samples);
    EXPECT_EQ(-1, stats.min);
    EXPECT_EQ(-1, stats.max);
    EXPECT_EQ(-1, stats.average);
  }

  SimulatedClock clock_;
};

TEST_F(StatsCounterTest, NoSamples) {
  AvgCounter counter(&clock_, nullptr);
  VerifyStatsIsNotSet(counter.GetStats());
}

TEST_F(StatsCounterTest, TestRegisterObserver) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  const int kSample = 22;
  AvgCounter counter(&clock_, observer);
  AddSampleAndAdvance(kSample, kProcessIntervalMs, &counter);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  EXPECT_EQ(1, observer->num_calls_);
}

TEST_F(StatsCounterTest, VerifyProcessInterval) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  AvgCounter counter(&clock_, observer);
  counter.Add(4);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs - 1);
  // Try trigger process (interval has not passed).
  counter.Add(8);
  EXPECT_EQ(0, observer->num_calls_);
  VerifyStatsIsNotSet(counter.GetStats());
  // Make process interval pass.
  clock_.AdvanceTimeMilliseconds(1);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(6, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
}

TEST_F(StatsCounterTest, TestMetric_AvgCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  AvgCounter counter(&clock_, observer);
  counter.Add(4);
  counter.Add(8);
  counter.Add(9);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  // Average per interval.
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(7, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(7, stats.min);
  EXPECT_EQ(7, stats.max);
  EXPECT_EQ(7, stats.average);
}

TEST_F(StatsCounterTest, TestMetric_MaxCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  MaxCounter counter(&clock_, observer);
  counter.Add(4);
  counter.Add(9);
  counter.Add(8);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  // Average per interval.
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(9, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(9, stats.min);
  EXPECT_EQ(9, stats.max);
  EXPECT_EQ(9, stats.average);
}

TEST_F(StatsCounterTest, TestMetric_PercentCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  PercentCounter counter(&clock_, observer);
  counter.Add(true);
  counter.Add(false);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Add(false);
  // Percentage per interval.
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(50, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(50, stats.min);
  EXPECT_EQ(50, stats.max);
}

TEST_F(StatsCounterTest, TestMetric_PermilleCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  PermilleCounter counter(&clock_, observer);
  counter.Add(true);
  counter.Add(false);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Add(false);
  // Permille per interval.
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(500, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(500, stats.min);
  EXPECT_EQ(500, stats.max);
}

TEST_F(StatsCounterTest, TestMetric_RateCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  RateCounter counter(&clock_, observer);
  counter.Add(186);
  counter.Add(350);
  counter.Add(22);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  // Rate per interval, (186 + 350 + 22) / 2 sec = 279 samples/sec
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(279, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(279, stats.min);
  EXPECT_EQ(279, stats.max);
}

TEST_F(StatsCounterTest, TestMetric_RateAccCounter) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  RateAccCounter counter(&clock_, observer);
  counter.Set(175);
  counter.Set(188);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  // Trigger process (sample included in next interval).
  counter.Set(192);
  // Rate per interval: (188 - 0) / 2 sec = 94 samples/sec
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(94, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(94, stats.min);
  EXPECT_EQ(94, stats.max);
}

TEST_F(StatsCounterTest, TestGetStats_MultipleIntervals) {
  AvgCounter counter(&clock_, nullptr);
  const int kSample1 = 1;
  const int kSample2 = 5;
  const int kSample3 = 8;
  const int kSample4 = 11;
  const int kSample5 = 50;
  AddSampleAndAdvance(kSample1, kProcessIntervalMs, &counter);
  AddSampleAndAdvance(kSample2, kProcessIntervalMs, &counter);
  AddSampleAndAdvance(kSample3, kProcessIntervalMs, &counter);
  AddSampleAndAdvance(kSample4, kProcessIntervalMs, &counter);
  AddSampleAndAdvance(kSample5, kProcessIntervalMs, &counter);
  // Trigger process (sample included in next interval).
  counter.Add(111);
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(5, stats.num_samples);
  EXPECT_EQ(kSample1, stats.min);
  EXPECT_EQ(kSample5, stats.max);
  EXPECT_EQ(15, stats.average);
}

TEST_F(StatsCounterTest, TestGetStatsTwice) {
  const int kSample1 = 4;
  const int kSample2 = 7;
  AvgCounter counter(&clock_, nullptr);
  AddSampleAndAdvance(kSample1, kProcessIntervalMs, &counter);
  // Trigger process (sample included in next interval).
  counter.Add(kSample2);
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(1, stats.num_samples);
  EXPECT_EQ(kSample1, stats.min);
  EXPECT_EQ(kSample1, stats.max);
  // Trigger process (sample included in next interval).
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs);
  counter.Add(111);
  stats = counter.GetStats();
  EXPECT_EQ(2, stats.num_samples);
  EXPECT_EQ(kSample1, stats.min);
  EXPECT_EQ(kSample2, stats.max);
  EXPECT_EQ(6, stats.average);
}

TEST_F(StatsCounterTest, TestRateAccCounter_NegativeRateIgnored) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  const int kSample1 = 200;  //  200 / 2 sec
  const int kSample2 = 100;  // -100 / 2 sec - negative ignored
  const int kSample3 = 700;  //  600 / 2 sec
  RateAccCounter counter(&clock_, observer);
  SetSampleAndAdvance(kSample1, kProcessIntervalMs, &counter);
  SetSampleAndAdvance(kSample2, kProcessIntervalMs, &counter);
  SetSampleAndAdvance(kSample3, kProcessIntervalMs, &counter);
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(100, observer->last_sample_);
  // Trigger process (sample included in next interval).
  counter.Set(2000);
  EXPECT_EQ(2, observer->num_calls_);
  EXPECT_EQ(300, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(2, stats.num_samples);
  EXPECT_EQ(100, stats.min);
  EXPECT_EQ(300, stats.max);
  EXPECT_EQ(200, stats.average);
}

TEST_F(StatsCounterTest, TestAvgCounter_IntervalsWithoutSamplesIgnored) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  AvgCounter counter(&clock_, observer);
  AddSampleAndAdvance(6, kProcessIntervalMs * 4 - 1, &counter);
  // Trigger process (sample included in next interval).
  counter.Add(8);
  // [6:1],  two intervals without samples passed.
  EXPECT_EQ(1, observer->num_calls_);
  EXPECT_EQ(6, observer->last_sample_);
  // Make last interval pass.
  clock_.AdvanceTimeMilliseconds(1);
  counter.Add(111);  // Trigger process (sample included in next interval).
  // [6:1],[8:1]
  EXPECT_EQ(2, observer->num_calls_);
  EXPECT_EQ(8, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(2, stats.num_samples);
  EXPECT_EQ(6, stats.min);
  EXPECT_EQ(8, stats.max);
}

TEST_F(StatsCounterTest, TestRateCounter_IntervalsWithoutSamplesIncluded) {
  StatsCounterObserverImpl* observer = new StatsCounterObserverImpl();
  const int kSample1 = 50;  //  50 / 2 sec
  const int kSample2 = 20;  //  20 / 2 sec
  RateCounter counter(&clock_, observer);
  counter.Add(kSample1);
  clock_.AdvanceTimeMilliseconds(kProcessIntervalMs * 3 - 1);
  // Trigger process (sample included in next interval).
  counter.Add(kSample2);
  // [0:1],[25:1],  one interval without samples passed.
  EXPECT_EQ(2, observer->num_calls_);
  EXPECT_EQ(25, observer->last_sample_);
  // Make last interval pass.
  clock_.AdvanceTimeMilliseconds(1);
  counter.Add(111);  // Trigger process (sample included in next interval).
  // [0:1],[10:1],[25:1]
  EXPECT_EQ(3, observer->num_calls_);
  EXPECT_EQ(10, observer->last_sample_);
  // Aggregated stats.
  AggregatedStats stats = counter.GetStats();
  EXPECT_EQ(3, stats.num_samples);
  EXPECT_EQ(0, stats.min);
  EXPECT_EQ(25, stats.max);
}

}  // namespace webrtc
