/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/base/cpuid.h"

#include <iostream>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/systeminfo.h"

TEST(CpuInfoTest, CpuId) {
  LOG(LS_INFO) << "ARM: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasARM);
  LOG(LS_INFO) << "NEON: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasNEON);
  LOG(LS_INFO) << "X86: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasX86);
  LOG(LS_INFO) << "SSE2: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSE2);
  LOG(LS_INFO) << "SSSE3: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSSE3);
  LOG(LS_INFO) << "SSE41: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSE41);
  LOG(LS_INFO) << "SSE42: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSE42);
  LOG(LS_INFO) << "AVX: "
      << cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasAVX);
  bool has_arm = cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasARM);
  bool has_x86 = cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasX86);
  EXPECT_FALSE(has_arm && has_x86);
}

TEST(CpuInfoTest, IsCoreIOrBetter) {
  bool core_i_or_better = cricket::IsCoreIOrBetter();
  // Tests the function is callable.  Run on known hardware to confirm.
  LOG(LS_INFO) << "IsCoreIOrBetter: " << core_i_or_better;

  // All Core I CPUs have SSE 4.1.
  if (core_i_or_better) {
    EXPECT_TRUE(cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSE41));
    EXPECT_TRUE(cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSSE3));
  }

  // All CPUs that lack SSE 4.1 are not Core I CPUs.
  if (!cricket::CpuInfo::TestCpuFlag(cricket::CpuInfo::kCpuHasSSE41)) {
    EXPECT_FALSE(core_i_or_better);
  }
}

