/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_ENGINE_FAKEWEBRTCCOMMON_H_
#define WEBRTC_MEDIA_ENGINE_FAKEWEBRTCCOMMON_H_

#include "webrtc/base/common.h"

namespace cricket {

#define WEBRTC_STUB(method, args) \
  int method args override { return 0; }

#define WEBRTC_STUB_CONST(method, args) \
  int method args const override { return 0; }

#define WEBRTC_BOOL_STUB(method, args) \
  bool method args override { return true; }

#define WEBRTC_BOOL_STUB_CONST(method, args) \
  bool method args const override { return true; }

#define WEBRTC_VOID_STUB(method, args) \
  void method args override {}

#define WEBRTC_FUNC(method, args) int method args override

#define WEBRTC_FUNC_CONST(method, args) int method args const override

#define WEBRTC_BOOL_FUNC(method, args) bool method args override

#define WEBRTC_VOID_FUNC(method, args) void method args override
}  // namespace cricket

#endif  // WEBRTC_MEDIA_ENGINE_FAKEWEBRTCCOMMON_H_
