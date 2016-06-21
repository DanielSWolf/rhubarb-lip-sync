/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_BASE_MEDIACOMMON_H_
#define WEBRTC_MEDIA_BASE_MEDIACOMMON_H_

#include "webrtc/base/stringencode.h"

namespace cricket {

enum MediaCapabilities {
  AUDIO_RECV = 1 << 0,
  AUDIO_SEND = 1 << 1,
  VIDEO_RECV = 1 << 2,
  VIDEO_SEND = 1 << 3,
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_BASE_MEDIACOMMON_H_
