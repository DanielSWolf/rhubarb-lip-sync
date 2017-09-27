/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_ENGINE_WEBRTCVIDEOCHANNELFACTORY_H_
#define WEBRTC_MEDIA_ENGINE_WEBRTCVIDEOCHANNELFACTORY_H_

namespace cricket {
class VoiceMediaChannel;
class WebRtcVideoEngine2;
class WebRtcVideoChannel2;

class WebRtcVideoChannelFactory {
 public:
  virtual ~WebRtcVideoChannelFactory() {}
  virtual WebRtcVideoChannel2* Create(WebRtcVideoEngine2* engine,
                                      VoiceMediaChannel* voice_channel) = 0;
};
}  // namespace cricket

#endif  // WEBRTC_MEDIA_ENGINE_WEBRTCVIDEOCHANNELFACTORY_H_
