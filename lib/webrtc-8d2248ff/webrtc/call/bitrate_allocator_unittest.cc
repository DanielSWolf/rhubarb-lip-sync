/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <algorithm>
#include <memory>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/call/bitrate_allocator.h"
#include "webrtc/modules/bitrate_controller/include/bitrate_controller.h"

using testing::NiceMock;

namespace webrtc {

class MockLimitObserver : public BitrateAllocator::LimitObserver {
 public:
  MOCK_METHOD2(OnAllocationLimitsChanged,
               void(uint32_t min_send_bitrate_bps,
                    uint32_t max_padding_bitrate_bps));
};

class TestBitrateObserver : public BitrateAllocatorObserver {
 public:
  TestBitrateObserver()
      : last_bitrate_bps_(0), last_fraction_loss_(0), last_rtt_ms_(0) {}

  void OnBitrateUpdated(uint32_t bitrate_bps,
                        uint8_t fraction_loss,
                        int64_t rtt) override {
    last_bitrate_bps_ = bitrate_bps;
    last_fraction_loss_ = fraction_loss;
    last_rtt_ms_ = rtt;
  }
  uint32_t last_bitrate_bps_;
  uint8_t last_fraction_loss_;
  int64_t last_rtt_ms_;
};

class BitrateAllocatorTest : public ::testing::Test {
 protected:
  BitrateAllocatorTest() : allocator_(new BitrateAllocator(&limit_observer_)) {
    allocator_->OnNetworkChanged(300000u, 0, 0);
  }
  ~BitrateAllocatorTest() {}

  NiceMock<MockLimitObserver> limit_observer_;
  std::unique_ptr<BitrateAllocator> allocator_;
};

TEST_F(BitrateAllocatorTest, UpdatingBitrateObserver) {
  TestBitrateObserver bitrate_observer;
  const uint32_t kMinSendBitrateBps = 100000;
  const uint32_t kPadUpToBitrateBps = 50000;

  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(kMinSendBitrateBps,
                                                         kPadUpToBitrateBps));
  allocator_->AddObserver(&bitrate_observer, kMinSendBitrateBps, 1500000,
                          kPadUpToBitrateBps, true);
  EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer));
  allocator_->OnNetworkChanged(200000, 0, 0);
  EXPECT_EQ(200000, allocator_->GetStartBitrate(&bitrate_observer));

  // TODO(pbos): Expect capping to 1.5M instead of 3M when not boosting the max
  // bitrate for FEC/retransmissions (see todo in BitrateAllocator).
  allocator_->OnNetworkChanged(4000000, 0, 0);
  EXPECT_EQ(3000000, allocator_->GetStartBitrate(&bitrate_observer));

  // Expect |max_padding_bitrate_bps| to change to 0 if the observer is updated.
  EXPECT_CALL(limit_observer_,
              OnAllocationLimitsChanged(kMinSendBitrateBps, 0));
  allocator_->AddObserver(&bitrate_observer, kMinSendBitrateBps, 4000000, 0,
                          true);
  EXPECT_EQ(4000000, allocator_->GetStartBitrate(&bitrate_observer));

  EXPECT_CALL(limit_observer_,
              OnAllocationLimitsChanged(kMinSendBitrateBps, 0));
  allocator_->AddObserver(&bitrate_observer, kMinSendBitrateBps, 1500000, 0,
                          true);
  EXPECT_EQ(3000000, allocator_->GetStartBitrate(&bitrate_observer));
  EXPECT_EQ(3000000u, bitrate_observer.last_bitrate_bps_);
  allocator_->OnNetworkChanged(1500000, 0, 0);
  EXPECT_EQ(1500000u, bitrate_observer.last_bitrate_bps_);
}

TEST_F(BitrateAllocatorTest, TwoBitrateObserversOneRtcpObserver) {
  TestBitrateObserver bitrate_observer_1;
  TestBitrateObserver bitrate_observer_2;
  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(100000, 0));
      allocator_->AddObserver(&bitrate_observer_1, 100000, 300000, 0, true);
      EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_1));
      EXPECT_CALL(limit_observer_,
                  OnAllocationLimitsChanged(100000 + 200000, 0));
      allocator_->AddObserver(&bitrate_observer_2, 200000, 300000, 0, true);
      EXPECT_EQ(200000, allocator_->GetStartBitrate(&bitrate_observer_2));

      // Test too low start bitrate, hence lower than sum of min. Min bitrates
      // will
      // be allocated to all observers.
      allocator_->OnNetworkChanged(200000, 0, 50);
      EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);
      EXPECT_EQ(0, bitrate_observer_1.last_fraction_loss_);
      EXPECT_EQ(50, bitrate_observer_1.last_rtt_ms_);
      EXPECT_EQ(200000u, bitrate_observer_2.last_bitrate_bps_);
      EXPECT_EQ(0, bitrate_observer_2.last_fraction_loss_);
      EXPECT_EQ(50, bitrate_observer_2.last_rtt_ms_);

      // Test a bitrate which should be distributed equally.
      allocator_->OnNetworkChanged(500000, 0, 50);
      const uint32_t kBitrateToShare = 500000 - 200000 - 100000;
      EXPECT_EQ(100000u + kBitrateToShare / 2,
                bitrate_observer_1.last_bitrate_bps_);
      EXPECT_EQ(200000u + kBitrateToShare / 2,
                bitrate_observer_2.last_bitrate_bps_);

      // Limited by 2x max bitrates since we leave room for FEC and
      // retransmissions.
      allocator_->OnNetworkChanged(1500000, 0, 50);
      EXPECT_EQ(600000u, bitrate_observer_1.last_bitrate_bps_);
      EXPECT_EQ(600000u, bitrate_observer_2.last_bitrate_bps_);

      // Verify that if the bandwidth estimate is set to zero, the allocated
      // rate is
      // zero.
      allocator_->OnNetworkChanged(0, 0, 50);
      EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);
      EXPECT_EQ(0u, bitrate_observer_2.last_bitrate_bps_);
}

TEST_F(BitrateAllocatorTest, RemoveObserverTriggersLimitObserver) {
  TestBitrateObserver bitrate_observer;
  const uint32_t kMinSendBitrateBps = 100000;
  const uint32_t kPadUpToBitrateBps = 50000;

  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(kMinSendBitrateBps,
                                                         kPadUpToBitrateBps));
  allocator_->AddObserver(&bitrate_observer, kMinSendBitrateBps, 1500000,
                          kPadUpToBitrateBps, true);
  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(0, 0));
  allocator_->RemoveObserver(&bitrate_observer);
}

class BitrateAllocatorTestNoEnforceMin : public ::testing::Test {
 protected:
  BitrateAllocatorTestNoEnforceMin()
      : allocator_(new BitrateAllocator(&limit_observer_)) {
    allocator_->OnNetworkChanged(300000u, 0, 0);
  }
  ~BitrateAllocatorTestNoEnforceMin() {}

  NiceMock<MockLimitObserver> limit_observer_;
  std::unique_ptr<BitrateAllocator> allocator_;
};

// The following three tests verify enforcing a minimum bitrate works as
// intended.
TEST_F(BitrateAllocatorTestNoEnforceMin, OneBitrateObserver) {
  TestBitrateObserver bitrate_observer_1;
  // Expect OnAllocationLimitsChanged with |min_send_bitrate_bps| = 0 since
  // AddObserver is called with |enforce_min_bitrate| = false.
  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(0, 0));
      allocator_->AddObserver(&bitrate_observer_1, 100000, 400000, 0, false);
      EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_1));

      // High BWE.
      allocator_->OnNetworkChanged(150000, 0, 0);
      EXPECT_EQ(150000u, bitrate_observer_1.last_bitrate_bps_);

      // Low BWE.
      allocator_->OnNetworkChanged(10000, 0, 0);
      EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);

      EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(0, 0));
      allocator_->RemoveObserver(&bitrate_observer_1);
}

TEST_F(BitrateAllocatorTestNoEnforceMin, ThreeBitrateObservers) {
  TestBitrateObserver bitrate_observer_1;
  TestBitrateObserver bitrate_observer_2;
  TestBitrateObserver bitrate_observer_3;
  // Set up the observers with min bitrates at 100000, 200000, and 300000.
  allocator_->AddObserver(&bitrate_observer_1, 100000, 400000, 0, false);
  EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_1));

  allocator_->AddObserver(&bitrate_observer_2, 200000, 400000, 0, false);
  EXPECT_EQ(200000, allocator_->GetStartBitrate(&bitrate_observer_2));
  EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);

  allocator_->AddObserver(&bitrate_observer_3, 300000, 400000, 0, false);
  EXPECT_EQ(0, allocator_->GetStartBitrate(&bitrate_observer_3));
  EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(200000u, bitrate_observer_2.last_bitrate_bps_);

  // High BWE. Make sure the controllers get a fair share of the surplus (i.e.,
  // what is left after each controller gets its min rate).
  allocator_->OnNetworkChanged(690000, 0, 0);
  // Verify that each observer gets its min rate (sum of min rates is 600000),
  // and that the remaining 90000 is divided equally among the three.
  uint32_t bitrate_to_share = 690000u - 100000u - 200000u - 300000u;
  EXPECT_EQ(100000u + bitrate_to_share / 3,
            bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(200000u + bitrate_to_share / 3,
            bitrate_observer_2.last_bitrate_bps_);
  EXPECT_EQ(300000u + bitrate_to_share / 3,
            bitrate_observer_3.last_bitrate_bps_);

  // BWE below the sum of observer's min bitrate.
  allocator_->OnNetworkChanged(300000, 0, 0);
  EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);  // Min bitrate.
  EXPECT_EQ(200000u, bitrate_observer_2.last_bitrate_bps_);  // Min bitrate.
  EXPECT_EQ(0u, bitrate_observer_3.last_bitrate_bps_);  // Nothing.

  // Increased BWE, but still below the sum of configured min bitrates for all
  // observers and too little for observer 3. 1 and 2 will share the rest.
  allocator_->OnNetworkChanged(500000, 0, 0);
  EXPECT_EQ(200000u, bitrate_observer_1.last_bitrate_bps_);  // Min + split.
  EXPECT_EQ(300000u, bitrate_observer_2.last_bitrate_bps_);  // Min + split.
  EXPECT_EQ(0u, bitrate_observer_3.last_bitrate_bps_);  // Nothing.

  // Below min for all.
  allocator_->OnNetworkChanged(10000, 0, 0);
  EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(0u, bitrate_observer_2.last_bitrate_bps_);
  EXPECT_EQ(0u, bitrate_observer_3.last_bitrate_bps_);

  // Verify that zero estimated bandwidth, means that that all gets zero,
  // regardless of set min bitrate.
  allocator_->OnNetworkChanged(0, 0, 0);
  EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(0u, bitrate_observer_2.last_bitrate_bps_);
  EXPECT_EQ(0u, bitrate_observer_3.last_bitrate_bps_);

  allocator_->RemoveObserver(&bitrate_observer_1);
  allocator_->RemoveObserver(&bitrate_observer_2);
  allocator_->RemoveObserver(&bitrate_observer_3);
}

TEST_F(BitrateAllocatorTest, ThreeBitrateObserversLowBweEnforceMin) {
  TestBitrateObserver bitrate_observer_1;
  TestBitrateObserver bitrate_observer_2;
  TestBitrateObserver bitrate_observer_3;

  allocator_->AddObserver(&bitrate_observer_1, 100000, 400000, 0, true);
  EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_1));

  allocator_->AddObserver(&bitrate_observer_2, 200000, 400000, 0, true);
  EXPECT_EQ(200000, allocator_->GetStartBitrate(&bitrate_observer_2));
  EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);

      allocator_->AddObserver(&bitrate_observer_3, 300000, 400000, 0, true);
      EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_3));
      EXPECT_EQ(100000, static_cast<int>(bitrate_observer_1.last_bitrate_bps_));
      EXPECT_EQ(200000, static_cast<int>(bitrate_observer_2.last_bitrate_bps_));

      // Low BWE. Verify that all observers still get their respective min
      // bitrate.
      allocator_->OnNetworkChanged(1000, 0, 0);
      EXPECT_EQ(100000u, bitrate_observer_1.last_bitrate_bps_);  // Min cap.
      EXPECT_EQ(200000u, bitrate_observer_2.last_bitrate_bps_);  // Min cap.
      EXPECT_EQ(300000u, bitrate_observer_3.last_bitrate_bps_);  // Min cap.

      allocator_->RemoveObserver(&bitrate_observer_1);
      allocator_->RemoveObserver(&bitrate_observer_2);
      allocator_->RemoveObserver(&bitrate_observer_3);
}

TEST_F(BitrateAllocatorTest, AddObserverWhileNetworkDown) {
  TestBitrateObserver bitrate_observer_1;
  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(50000, 0));

  allocator_->AddObserver(&bitrate_observer_1, 50000, 400000, 0, true);
  EXPECT_EQ(300000, allocator_->GetStartBitrate(&bitrate_observer_1));

  // Set network down, ie, no available bitrate.
  allocator_->OnNetworkChanged(0, 0, 0);

  EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);

  TestBitrateObserver bitrate_observer_2;
  // Adding an observer while the network is down should not affect the limits.
  EXPECT_CALL(limit_observer_, OnAllocationLimitsChanged(50000 + 50000, 0));
  allocator_->AddObserver(&bitrate_observer_2, 50000, 400000, 0, true);

  // Expect the start_bitrate to be set as if the network was still up but that
  // the new observer have been notified that the network is down.
  EXPECT_EQ(300000 / 2, allocator_->GetStartBitrate(&bitrate_observer_2));
  EXPECT_EQ(0u, bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(0u, bitrate_observer_2.last_bitrate_bps_);

  // Set network back up.
  allocator_->OnNetworkChanged(1500000, 0, 50);
  EXPECT_EQ(750000u, bitrate_observer_1.last_bitrate_bps_);
  EXPECT_EQ(750000u, bitrate_observer_2.last_bitrate_bps_);
}

TEST_F(BitrateAllocatorTest, MixedEnforecedConfigs) {
  TestBitrateObserver enforced_observer;
      allocator_->AddObserver(&enforced_observer, 6000, 30000, 0, true);
      EXPECT_EQ(60000, allocator_->GetStartBitrate(&enforced_observer));

      TestBitrateObserver not_enforced_observer;
      allocator_->AddObserver(&not_enforced_observer, 30000, 2500000, 0, false);
      EXPECT_EQ(270000, allocator_->GetStartBitrate(&not_enforced_observer));
      EXPECT_EQ(30000u, enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(36000, 0, 50);
      EXPECT_EQ(6000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(30000u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(35000, 0, 50);
      EXPECT_EQ(30000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(0u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(5000, 0, 50);
      EXPECT_EQ(6000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(0u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(36000, 0, 50);
      EXPECT_EQ(30000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(0u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(55000, 0, 50);
      EXPECT_EQ(30000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(0u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(56000, 0, 50);
      EXPECT_EQ(6000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(50000u, not_enforced_observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(56000, 0, 50);
      EXPECT_EQ(16000u, enforced_observer.last_bitrate_bps_);
      EXPECT_EQ(40000u, not_enforced_observer.last_bitrate_bps_);

      allocator_->RemoveObserver(&enforced_observer);
      allocator_->RemoveObserver(&not_enforced_observer);
}

TEST_F(BitrateAllocatorTest, AvoidToggleAbsolute) {
  TestBitrateObserver observer;
      allocator_->AddObserver(&observer, 30000, 300000, 0, false);
      EXPECT_EQ(300000, allocator_->GetStartBitrate(&observer));

      allocator_->OnNetworkChanged(30000, 0, 50);
      EXPECT_EQ(30000u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(20000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(30000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(49000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(50000, 0, 50);
      EXPECT_EQ(50000u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(30000, 0, 50);
      EXPECT_EQ(30000u, observer.last_bitrate_bps_);

      allocator_->RemoveObserver(&observer);
}

TEST_F(BitrateAllocatorTest, AvoidTogglePercent) {
  TestBitrateObserver observer;
      allocator_->AddObserver(&observer, 300000, 600000, 0, false);
      EXPECT_EQ(300000, allocator_->GetStartBitrate(&observer));

      allocator_->OnNetworkChanged(300000, 0, 50);
      EXPECT_EQ(300000u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(200000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(300000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(329000, 0, 50);
      EXPECT_EQ(0u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(330000, 0, 50);
      EXPECT_EQ(330000u, observer.last_bitrate_bps_);

      allocator_->OnNetworkChanged(300000, 0, 50);
      EXPECT_EQ(300000u, observer.last_bitrate_bps_);

      allocator_->RemoveObserver(&observer);
}

}  // namespace webrtc
