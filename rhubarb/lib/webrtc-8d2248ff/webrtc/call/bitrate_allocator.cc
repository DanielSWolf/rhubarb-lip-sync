/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "webrtc/call/bitrate_allocator.h"

#include <algorithm>
#include <utility>

#include "webrtc/base/checks.h"
#include "webrtc/modules/bitrate_controller/include/bitrate_controller.h"

namespace webrtc {

// Allow packets to be transmitted in up to 2 times max video bitrate if the
// bandwidth estimate allows it.
const int kTransmissionMaxBitrateMultiplier = 2;
const int kDefaultBitrateBps = 300000;

// Require a bitrate increase of max(10%, 20kbps) to resume paused streams.
const double kToggleFactor = 0.1;
const uint32_t kMinToggleBitrateBps = 20000;

BitrateAllocator::BitrateAllocator(LimitObserver* limit_observer)
    : limit_observer_(limit_observer),
      bitrate_observer_configs_(),
      last_bitrate_bps_(kDefaultBitrateBps),
      last_non_zero_bitrate_bps_(kDefaultBitrateBps),
      last_fraction_loss_(0),
      last_rtt_(0) {}

void BitrateAllocator::OnNetworkChanged(uint32_t target_bitrate_bps,
                                        uint8_t fraction_loss,
                                        int64_t rtt) {
  rtc::CritScope lock(&crit_sect_);
  last_bitrate_bps_ = target_bitrate_bps;
  last_non_zero_bitrate_bps_ =
      target_bitrate_bps > 0 ? target_bitrate_bps : last_non_zero_bitrate_bps_;
  last_fraction_loss_ = fraction_loss;
  last_rtt_ = rtt;

  ObserverAllocation allocation = AllocateBitrates(target_bitrate_bps);
  for (const auto& kv : allocation) {
    kv.first->OnBitrateUpdated(kv.second, last_fraction_loss_, last_rtt_);
  }
  last_allocation_ = allocation;
}

void BitrateAllocator::AddObserver(BitrateAllocatorObserver* observer,
                                   uint32_t min_bitrate_bps,
                                   uint32_t max_bitrate_bps,
                                   uint32_t pad_up_bitrate_bps,
                                   bool enforce_min_bitrate) {
  rtc::CritScope lock(&crit_sect_);
  auto it = FindObserverConfig(observer);

  // Update settings if the observer already exists, create a new one otherwise.
  if (it != bitrate_observer_configs_.end()) {
    it->min_bitrate_bps = min_bitrate_bps;
    it->max_bitrate_bps = max_bitrate_bps;
    it->pad_up_bitrate_bps = pad_up_bitrate_bps;
    it->enforce_min_bitrate = enforce_min_bitrate;
  } else {
    bitrate_observer_configs_.push_back(
        ObserverConfig(observer, min_bitrate_bps, max_bitrate_bps,
                       pad_up_bitrate_bps, enforce_min_bitrate));
  }

  ObserverAllocation allocation;
  if (last_bitrate_bps_ > 0) {
    // Calculate a new allocation and update all observers.
    allocation = AllocateBitrates(last_bitrate_bps_);
    for (const auto& kv : allocation)
      kv.first->OnBitrateUpdated(kv.second, last_fraction_loss_, last_rtt_);
  } else {
    // Currently, an encoder is not allowed to produce frames.
    // But we still have to return the initial config bitrate + let the
    // observer know that it can not produce frames.
    allocation = AllocateBitrates(last_non_zero_bitrate_bps_);
    observer->OnBitrateUpdated(0, last_fraction_loss_, last_rtt_);
  }
  UpdateAllocationLimits();

  last_allocation_ = allocation;
}

void BitrateAllocator::UpdateAllocationLimits() {
  uint32_t total_requested_padding_bitrate = 0;
  uint32_t total_requested_min_bitrate = 0;

  {
    rtc::CritScope lock(&crit_sect_);
    for (const auto& config : bitrate_observer_configs_) {
      if (config.enforce_min_bitrate) {
        total_requested_min_bitrate += config.min_bitrate_bps;
      }
      total_requested_padding_bitrate += config.pad_up_bitrate_bps;
    }
  }

  limit_observer_->OnAllocationLimitsChanged(total_requested_min_bitrate,
                                             total_requested_padding_bitrate);
}

void BitrateAllocator::RemoveObserver(BitrateAllocatorObserver* observer) {
  {
    rtc::CritScope lock(&crit_sect_);
    auto it = FindObserverConfig(observer);
    if (it != bitrate_observer_configs_.end()) {
      bitrate_observer_configs_.erase(it);
    }
  }
  UpdateAllocationLimits();
}

int BitrateAllocator::GetStartBitrate(BitrateAllocatorObserver* observer) {
  rtc::CritScope lock(&crit_sect_);
  const auto& it = last_allocation_.find(observer);
  if (it != last_allocation_.end())
    return it->second;

  // This is a new observer that has not yet been started. Assume that if it is
  // added, all observers would split the available bitrate evenly.
  return last_non_zero_bitrate_bps_ /
         static_cast<int>((bitrate_observer_configs_.size() + 1));
}

BitrateAllocator::ObserverConfigList::iterator
BitrateAllocator::FindObserverConfig(
    const BitrateAllocatorObserver* observer) {
  for (auto it = bitrate_observer_configs_.begin();
       it != bitrate_observer_configs_.end(); ++it) {
    if (it->observer == observer)
      return it;
  }
  return bitrate_observer_configs_.end();
}

BitrateAllocator::ObserverAllocation BitrateAllocator::AllocateBitrates(
    uint32_t bitrate) {
  if (bitrate_observer_configs_.empty())
    return ObserverAllocation();

  if (bitrate == 0)
    return ZeroRateAllocation();

  uint32_t sum_min_bitrates = 0;
  uint32_t sum_max_bitrates = 0;
  for (const auto& observer_config : bitrate_observer_configs_) {
    sum_min_bitrates += observer_config.min_bitrate_bps;
    sum_max_bitrates += observer_config.max_bitrate_bps;
  }

  // Not enough for all observers to get an allocation, allocate according to:
  // enforced min bitrate -> allocated bitrate previous round -> restart paused
  // streams.
  if (!EnoughBitrateForAllObservers(bitrate, sum_min_bitrates))
    return LowRateAllocation(bitrate);

  // All observers will get their min bitrate plus an even share of the rest.
  if (bitrate <= sum_max_bitrates)
    return NormalRateAllocation(bitrate, sum_min_bitrates);

  // All observers will get up to kTransmissionMaxBitrateMultiplier x max.
  return MaxRateAllocation(bitrate, sum_max_bitrates);
}

BitrateAllocator::ObserverAllocation BitrateAllocator::ZeroRateAllocation() {
  ObserverAllocation allocation;
  for (const auto& observer_config : bitrate_observer_configs_)
    allocation[observer_config.observer] = 0;
  return allocation;
}

BitrateAllocator::ObserverAllocation BitrateAllocator::LowRateAllocation(
    uint32_t bitrate) {
  ObserverAllocation allocation;

  // Start by allocating bitrate to observers enforcing a min bitrate, hence
  // remaining_bitrate might turn negative.
  int64_t remaining_bitrate = bitrate;
  for (const auto& observer_config : bitrate_observer_configs_) {
    int32_t allocated_bitrate = 0;
    if (observer_config.enforce_min_bitrate)
      allocated_bitrate = observer_config.min_bitrate_bps;

    allocation[observer_config.observer] = allocated_bitrate;
    remaining_bitrate -= allocated_bitrate;
  }

  // Allocate bitrate to all previously active streams.
  if (remaining_bitrate > 0) {
    for (const auto& observer_config : bitrate_observer_configs_) {
      if (observer_config.enforce_min_bitrate ||
          LastAllocatedBitrate(observer_config) == 0)
        continue;

      if (remaining_bitrate >= observer_config.min_bitrate_bps) {
        allocation[observer_config.observer] = observer_config.min_bitrate_bps;
        remaining_bitrate -= observer_config.min_bitrate_bps;
      }
    }
  }

  // Allocate bitrate to previously paused streams.
  if (remaining_bitrate > 0) {
    for (const auto& observer_config : bitrate_observer_configs_) {
      if (LastAllocatedBitrate(observer_config) != 0)
        continue;

      // Add a hysteresis to avoid toggling.
      uint32_t required_bitrate = MinBitrateWithHysteresis(observer_config);
      if (remaining_bitrate >= required_bitrate) {
        allocation[observer_config.observer] = required_bitrate;
        remaining_bitrate -= required_bitrate;
      }
    }
  }

  // Split a possible remainder evenly on all streams with an allocation.
  if (remaining_bitrate > 0)
    DistributeBitrateEvenly(remaining_bitrate, false, 1, &allocation);

  RTC_DCHECK_EQ(allocation.size(), bitrate_observer_configs_.size());
  return allocation;
}

BitrateAllocator::ObserverAllocation BitrateAllocator::NormalRateAllocation(
    uint32_t bitrate,
    uint32_t sum_min_bitrates) {

  ObserverAllocation allocation;
  for (const auto& observer_config : bitrate_observer_configs_)
    allocation[observer_config.observer] = observer_config.min_bitrate_bps;

  bitrate -= sum_min_bitrates;
  if (bitrate > 0)
    DistributeBitrateEvenly(bitrate, true, 1, &allocation);

  return allocation;
}

BitrateAllocator::ObserverAllocation BitrateAllocator::MaxRateAllocation(
    uint32_t bitrate, uint32_t sum_max_bitrates) {
  ObserverAllocation allocation;

  for (const auto& observer_config : bitrate_observer_configs_) {
    allocation[observer_config.observer] = observer_config.max_bitrate_bps;
    bitrate -= observer_config.max_bitrate_bps;
  }
  DistributeBitrateEvenly(bitrate, true, kTransmissionMaxBitrateMultiplier,
                          &allocation);
  return allocation;
}

uint32_t BitrateAllocator::LastAllocatedBitrate(
    const ObserverConfig& observer_config) {

  const auto& it = last_allocation_.find(observer_config.observer);
  if (it != last_allocation_.end())
    return it->second;

  // Return the configured minimum bitrate for newly added observers, to avoid
  // requiring an extra high bitrate for the observer to get an allocated
  // bitrate.
  return observer_config.min_bitrate_bps;
}

uint32_t BitrateAllocator::MinBitrateWithHysteresis(
    const ObserverConfig& observer_config) {
  uint32_t min_bitrate = observer_config.min_bitrate_bps;
  if (LastAllocatedBitrate(observer_config) == 0) {
    min_bitrate += std::max(static_cast<uint32_t>(kToggleFactor * min_bitrate),
                            kMinToggleBitrateBps);
  }
  return min_bitrate;
}

void BitrateAllocator::DistributeBitrateEvenly(uint32_t bitrate,
                                               bool include_zero_allocations,
                                               int max_multiplier,
                                               ObserverAllocation* allocation) {
  RTC_DCHECK_EQ(allocation->size(), bitrate_observer_configs_.size());

  ObserverSortingMap list_max_bitrates;
  for (const auto& observer_config : bitrate_observer_configs_) {
    if (include_zero_allocations ||
        allocation->at(observer_config.observer) != 0) {
      list_max_bitrates.insert(std::pair<uint32_t, const ObserverConfig*>(
          observer_config.max_bitrate_bps, &observer_config));
    }
  }
  auto it = list_max_bitrates.begin();
  while (it != list_max_bitrates.end()) {
    RTC_DCHECK_GT(bitrate, 0u);
    uint32_t extra_allocation =
        bitrate / static_cast<uint32_t>(list_max_bitrates.size());
    uint32_t total_allocation =
        extra_allocation + allocation->at(it->second->observer);
    bitrate -= extra_allocation;
    if (total_allocation > max_multiplier * it->first) {
      // There is more than we can fit for this observer, carry over to the
      // remaining observers.
      bitrate += total_allocation - max_multiplier * it->first;
      total_allocation = max_multiplier * it->first;
    }
    // Finally, update the allocation for this observer.
    allocation->at(it->second->observer) = total_allocation;
    it = list_max_bitrates.erase(it);
  }
}

bool BitrateAllocator::EnoughBitrateForAllObservers(uint32_t bitrate,
                                                    uint32_t sum_min_bitrates) {
  if (bitrate < sum_min_bitrates)
    return false;

  uint32_t extra_bitrate_per_observer = (bitrate - sum_min_bitrates) /
      static_cast<uint32_t>(bitrate_observer_configs_.size());
  for (const auto& observer_config : bitrate_observer_configs_) {
    if (observer_config.min_bitrate_bps + extra_bitrate_per_observer <
        MinBitrateWithHysteresis(observer_config))
      return false;
  }
  return true;
}
}  // namespace webrtc
