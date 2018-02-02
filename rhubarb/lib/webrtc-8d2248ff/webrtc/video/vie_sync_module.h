/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// ViESyncModule is responsible for synchronization audio and video for a given
// VoE and ViE channel couple.

#ifndef WEBRTC_VIDEO_VIE_SYNC_MODULE_H_
#define WEBRTC_VIDEO_VIE_SYNC_MODULE_H_

#include <memory>

#include "webrtc/base/criticalsection.h"
#include "webrtc/modules/include/module.h"
#include "webrtc/video/stream_synchronization.h"
#include "webrtc/voice_engine/include/voe_video_sync.h"

namespace webrtc {

class Clock;
class RtpRtcp;
class VideoFrame;
class ViEChannel;
class VoEVideoSync;

namespace vcm {
class VideoReceiver;
}  // namespace vcm

class ViESyncModule : public Module {
 public:
  explicit ViESyncModule(vcm::VideoReceiver* vcm);
  ~ViESyncModule();

  void ConfigureSync(int voe_channel_id,
                     VoEVideoSync* voe_sync_interface,
                     RtpRtcp* video_rtcp_module,
                     RtpReceiver* rtp_receiver);

  // Implements Module.
  int64_t TimeUntilNextProcess() override;
  void Process() override;

  // Gets the sync offset between the current played out audio frame and the
  // video |frame|. Returns true on success, false otherwise.
  bool GetStreamSyncOffsetInMs(const VideoFrame& frame,
                               int64_t* stream_offset_ms) const;

 private:
  rtc::CriticalSection data_cs_;
  vcm::VideoReceiver* const video_receiver_;
  Clock* const clock_;
  RtpReceiver* rtp_receiver_;
  RtpRtcp* video_rtp_rtcp_;
  int voe_channel_id_;
  VoEVideoSync* voe_sync_interface_;
  int64_t last_sync_time_;
  std::unique_ptr<StreamSynchronization> sync_;
  StreamSynchronization::Measurements audio_measurement_;
  StreamSynchronization::Measurements video_measurement_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIE_SYNC_MODULE_H_
