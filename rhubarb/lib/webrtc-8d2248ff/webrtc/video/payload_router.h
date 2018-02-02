/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_PAYLOAD_ROUTER_H_
#define WEBRTC_VIDEO_PAYLOAD_ROUTER_H_

#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/config.h"
#include "webrtc/video_encoder.h"
#include "webrtc/system_wrappers/include/atomic32.h"

namespace webrtc {

class RTPFragmentationHeader;
class RtpRtcp;
struct RTPVideoHeader;

// PayloadRouter routes outgoing data to the correct sending RTP module, based
// on the simulcast layer in RTPVideoHeader.
class PayloadRouter : public EncodedImageCallback {
 public:
  // Rtp modules are assumed to be sorted in simulcast index order.
  explicit PayloadRouter(const std::vector<RtpRtcp*>& rtp_modules,
                         int payload_type);
  ~PayloadRouter();

  static size_t DefaultMaxPayloadLength();
  void SetSendStreams(const std::vector<VideoStream>& streams);

  // PayloadRouter will only route packets if being active, all packets will be
  // dropped otherwise.
  void set_active(bool active);
  bool active();

  // Implements EncodedImageCallback.
  // Returns 0 if the packet was routed / sent, -1 otherwise.
  int32_t Encoded(const EncodedImage& encoded_image,
                  const CodecSpecificInfo* codec_specific_info,
                  const RTPFragmentationHeader* fragmentation) override;

  // Configures current target bitrate.
  void SetTargetSendBitrate(uint32_t bitrate_bps);

  // Returns the maximum allowed data payload length, given the configured MTU
  // and RTP headers.
  size_t MaxPayloadLength() const;

 private:
  void UpdateModuleSendingState() EXCLUSIVE_LOCKS_REQUIRED(crit_);

  rtc::CriticalSection crit_;
  bool active_ GUARDED_BY(crit_);
  std::vector<VideoStream> streams_ GUARDED_BY(crit_);
  size_t num_sending_modules_ GUARDED_BY(crit_);

  // Rtp modules are assumed to be sorted in simulcast index order. Not owned.
  const std::vector<RtpRtcp*> rtp_modules_;
  const int payload_type_;

  RTC_DISALLOW_COPY_AND_ASSIGN(PayloadRouter);
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_PAYLOAD_ROUTER_H_
