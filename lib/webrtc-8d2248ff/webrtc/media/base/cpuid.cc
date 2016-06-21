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

#include "libyuv/cpu_id.h"

namespace cricket {

bool CpuInfo::TestCpuFlag(int flag) {
  return libyuv::TestCpuFlag(flag) ? true : false;
}

void CpuInfo::MaskCpuFlagsForTest(int enable_flags) {
  libyuv::MaskCpuFlags(enable_flags);
}

// Detect an Intel Core I5 or better such as 4th generation Macbook Air.
bool IsCoreIOrBetter() {
#if defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
  uint32_t cpu_info[4];
  libyuv::CpuId(0, 0, &cpu_info[0]);  // Function 0: Vendor ID
  if (cpu_info[1] == 0x756e6547 && cpu_info[3] == 0x49656e69 &&
      cpu_info[2] == 0x6c65746e) {  // GenuineIntel
    // Detect CPU Family and Model
    // 3:0 - Stepping
    // 7:4 - Model
    // 11:8 - Family
    // 13:12 - Processor Type
    // 19:16 - Extended Model
    // 27:20 - Extended Family
    libyuv::CpuId(1, 0, &cpu_info[0]);  // Function 1: Family and Model
    int family = ((cpu_info[0] >> 8) & 0x0f) | ((cpu_info[0] >> 16) & 0xff0);
    int model = ((cpu_info[0] >> 4) & 0x0f) | ((cpu_info[0] >> 12) & 0xf0);
    // CpuFamily | CpuModel |  Name
    //         6 |       14 |  Yonah -- Core
    //         6 |       15 |  Merom -- Core 2
    //         6 |       23 |  Penryn -- Core 2 (most common)
    //         6 |       26 |  Nehalem -- Core i*
    //         6 |       28 |  Atom
    //         6 |       30 |  Lynnfield -- Core i*
    //         6 |       37 |  Westmere -- Core i*
    const int kAtom = 28;
    const int kCore2 = 23;
    if (family < 6 || family == 15 ||
        (family == 6 && (model == kAtom || model <= kCore2))) {
      return false;
    }
    return true;
  }
#endif
  return false;
}

}  // namespace cricket
