/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_MEDIASTREAMPROVIDER_H_
#define WEBRTC_API_MEDIASTREAMPROVIDER_H_

#include <memory>

#include "webrtc/api/rtpsenderinterface.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/media/base/videosinkinterface.h"
#include "webrtc/media/base/videosourceinterface.h"

namespace cricket {

class AudioSource;
class VideoFrame;
struct AudioOptions;
struct VideoOptions;

}  // namespace cricket

namespace webrtc {

class AudioSinkInterface;

// TODO(deadbeef): Change the key from an ssrc to a "sender_id" or
// "receiver_id" string, which will be the MSID in the short term and MID in
// the long term.

// TODO(deadbeef): These interfaces are effectively just a way for the
// RtpSenders/Receivers to get to the BaseChannels. These interfaces should be
// refactored away eventually, as the classes converge.

// This interface is called by AudioRtpSender/Receivers to change the settings
// of an audio track connected to certain PeerConnection.
class AudioProviderInterface {
 public:
  // Enable/disable the audio playout of a remote audio track with |ssrc|.
  virtual void SetAudioPlayout(uint32_t ssrc, bool enable) = 0;
  // Enable/disable sending audio on the local audio track with |ssrc|.
  // When |enable| is true |options| should be applied to the audio track.
  virtual void SetAudioSend(uint32_t ssrc,
                            bool enable,
                            const cricket::AudioOptions& options,
                            cricket::AudioSource* source) = 0;

  // Sets the audio playout volume of a remote audio track with |ssrc|.
  // |volume| is in the range of [0, 10].
  virtual void SetAudioPlayoutVolume(uint32_t ssrc, double volume) = 0;

  // Allows for setting a direct audio sink for an incoming audio source.
  // Only one audio sink is supported per ssrc and ownership of the sink is
  // passed to the provider.
  virtual void SetRawAudioSink(
      uint32_t ssrc,
      std::unique_ptr<webrtc::AudioSinkInterface> sink) = 0;

  virtual RtpParameters GetAudioRtpSendParameters(uint32_t ssrc) const = 0;
  virtual bool SetAudioRtpSendParameters(uint32_t ssrc,
                                         const RtpParameters& parameters) = 0;

  virtual RtpParameters GetAudioRtpReceiveParameters(uint32_t ssrc) const = 0;
  virtual bool SetAudioRtpReceiveParameters(
      uint32_t ssrc,
      const RtpParameters& parameters) = 0;

  // Called when the first audio packet is received.
  sigslot::signal0<> SignalFirstAudioPacketReceived;

 protected:
  virtual ~AudioProviderInterface() {}
};

// This interface is called by VideoRtpSender/Receivers to change the settings
// of a video track connected to a certain PeerConnection.
class VideoProviderInterface {
 public:
  // Enable/disable the video playout of a remote video track with |ssrc|.
  virtual void SetVideoPlayout(
      uint32_t ssrc,
      bool enable,
      rtc::VideoSinkInterface<cricket::VideoFrame>* sink) = 0;
  // Enable/disable sending video on the local video track with |ssrc|.
  // TODO(deadbeef): Make |options| a reference parameter.
  // TODO(deadbeef): Eventually, |enable| and |options| will be contained
  // in |source|. When that happens, remove those parameters and rename
  // this to SetVideoSource.
  virtual void SetVideoSend(
      uint32_t ssrc,
      bool enable,
      const cricket::VideoOptions* options,
      rtc::VideoSourceInterface<cricket::VideoFrame>* source) = 0;

  virtual RtpParameters GetVideoRtpSendParameters(uint32_t ssrc) const = 0;
  virtual bool SetVideoRtpSendParameters(uint32_t ssrc,
                                         const RtpParameters& parameters) = 0;

  virtual RtpParameters GetVideoRtpReceiveParameters(uint32_t ssrc) const = 0;
  virtual bool SetVideoRtpReceiveParameters(
      uint32_t ssrc,
      const RtpParameters& parameters) = 0;

  // Called when the first video packet is received.
  sigslot::signal0<> SignalFirstVideoPacketReceived;

 protected:
  virtual ~VideoProviderInterface() {}
};

}  // namespace webrtc

#endif  // WEBRTC_API_MEDIASTREAMPROVIDER_H_
