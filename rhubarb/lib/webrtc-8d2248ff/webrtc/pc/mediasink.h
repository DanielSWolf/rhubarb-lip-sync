/*
 *  Copyright 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_PC_MEDIASINK_H_
#define WEBRTC_PC_MEDIASINK_H_

namespace cricket {

// MediaSinkInterface is a sink to handle RTP and RTCP packets that are sent or
// received by a channel.
class MediaSinkInterface {
 public:
  virtual ~MediaSinkInterface() {}

  virtual void SetMaxSize(size_t size) = 0;
  virtual bool Enable(bool enable) = 0;
  virtual bool IsEnabled() const = 0;
  virtual void OnPacket(const void* data, size_t size, bool rtcp) = 0;
  virtual void set_packet_filter(int filter) = 0;
};

}  // namespace cricket

#endif  // WEBRTC_PC_MEDIASINK_H_
