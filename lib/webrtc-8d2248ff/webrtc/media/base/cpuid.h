/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_BASE_CPUID_H_
#define WEBRTC_MEDIA_BASE_CPUID_H_

#include "webrtc/base/constructormagic.h"

namespace cricket {

class CpuInfo {
 public:
  // The following flags must match libyuv/cpu_id.h values.
  // Internal flag to indicate cpuid requires initialization.
  static const int kCpuInit = 0x1;

  // These flags are only valid on ARM processors.
  static const int kCpuHasARM = 0x2;
  static const int kCpuHasNEON = 0x4;
  // 0x8 reserved for future ARM flag.

  // These flags are only valid on x86 processors.
  static const int kCpuHasX86 = 0x10;
  static const int kCpuHasSSE2 = 0x20;
  static const int kCpuHasSSSE3 = 0x40;
  static const int kCpuHasSSE41 = 0x80;
  static const int kCpuHasSSE42 = 0x100;
  static const int kCpuHasAVX = 0x200;
  static const int kCpuHasAVX2 = 0x400;
  static const int kCpuHasERMS = 0x800;

  // These flags are only valid on MIPS processors.
  static const int kCpuHasMIPS = 0x1000;
  static const int kCpuHasMIPS_DSP = 0x2000;
  static const int kCpuHasMIPS_DSPR2 = 0x4000;

  // Detect CPU has SSE2 etc.
  static bool TestCpuFlag(int flag);

  // For testing, allow CPU flags to be disabled.
  static void MaskCpuFlagsForTest(int enable_flags);

 private:
  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(CpuInfo);
};

// Detect an Intel Core I5 or better such as 4th generation Macbook Air.
bool IsCoreIOrBetter();

}  // namespace cricket

#endif  // WEBRTC_MEDIA_BASE_CPUID_H_
