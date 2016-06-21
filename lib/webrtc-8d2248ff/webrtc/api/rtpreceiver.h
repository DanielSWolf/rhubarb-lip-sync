/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains classes that implement RtpReceiverInterface.
// An RtpReceiver associates a MediaStreamTrackInterface with an underlying
// transport (provided by AudioProviderInterface/VideoProviderInterface)

#ifndef WEBRTC_API_RTPRECEIVER_H_
#define WEBRTC_API_RTPRECEIVER_H_

#include <string>

#include "webrtc/api/mediastreamprovider.h"
#include "webrtc/api/rtpreceiverinterface.h"
#include "webrtc/api/remoteaudiosource.h"
#include "webrtc/api/videotracksource.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/media/base/videobroadcaster.h"

namespace webrtc {

// Internal class used by PeerConnection.
class RtpReceiverInternal : public RtpReceiverInterface {
 public:
  virtual void Stop() = 0;
};

class AudioRtpReceiver : public ObserverInterface,
                         public AudioSourceInterface::AudioObserver,
                         public rtc::RefCountedObject<RtpReceiverInternal>,
                         public sigslot::has_slots<> {
 public:
  AudioRtpReceiver(MediaStreamInterface* stream,
                   const std::string& track_id,
                   uint32_t ssrc,
                   AudioProviderInterface* provider);

  virtual ~AudioRtpReceiver();

  // ObserverInterface implementation
  void OnChanged() override;

  // AudioSourceInterface::AudioObserver implementation
  void OnSetVolume(double volume) override;

  rtc::scoped_refptr<AudioTrackInterface> audio_track() const {
    return track_.get();
  }

  // RtpReceiverInterface implementation
  rtc::scoped_refptr<MediaStreamTrackInterface> track() const override {
    return track_.get();
  }

  std::string id() const override { return id_; }

  RtpParameters GetParameters() const override;
  bool SetParameters(const RtpParameters& parameters) override;

  // RtpReceiverInternal implementation.
  void Stop() override;

  void SetObserver(RtpReceiverObserverInterface* observer) override;

  cricket::MediaType media_type() override { return cricket::MEDIA_TYPE_AUDIO; }

 private:
  void Reconfigure();
  void OnFirstAudioPacketReceived();

  const std::string id_;
  const uint32_t ssrc_;
  AudioProviderInterface* provider_;  // Set to null in Stop().
  const rtc::scoped_refptr<AudioTrackInterface> track_;
  bool cached_track_enabled_;
  RtpReceiverObserverInterface* observer_ = nullptr;
  bool received_first_packet_ = false;
};

class VideoRtpReceiver : public rtc::RefCountedObject<RtpReceiverInternal>,
                         public sigslot::has_slots<> {
 public:
  VideoRtpReceiver(MediaStreamInterface* stream,
                   const std::string& track_id,
                   rtc::Thread* worker_thread,
                   uint32_t ssrc,
                   VideoProviderInterface* provider);

  virtual ~VideoRtpReceiver();

  rtc::scoped_refptr<VideoTrackInterface> video_track() const {
    return track_.get();
  }

  // RtpReceiverInterface implementation
  rtc::scoped_refptr<MediaStreamTrackInterface> track() const override {
    return track_.get();
  }

  std::string id() const override { return id_; }

  RtpParameters GetParameters() const override;
  bool SetParameters(const RtpParameters& parameters) override;

  // RtpReceiverInternal implementation.
  void Stop() override;

  void SetObserver(RtpReceiverObserverInterface* observer) override;

  cricket::MediaType media_type() override { return cricket::MEDIA_TYPE_VIDEO; }

 private:
  void OnFirstVideoPacketReceived();

  std::string id_;
  uint32_t ssrc_;
  VideoProviderInterface* provider_;
  // |broadcaster_| is needed since the decoder can only handle one sink.
  // It might be better if the decoder can handle multiple sinks and consider
  // the VideoSinkWants.
  rtc::VideoBroadcaster broadcaster_;
  // |source_| is held here to be able to change the state of the source when
  // the VideoRtpReceiver is stopped.
  rtc::scoped_refptr<VideoTrackSource> source_;
  rtc::scoped_refptr<VideoTrackInterface> track_;
  RtpReceiverObserverInterface* observer_ = nullptr;
  bool received_first_packet_ = false;
};

}  // namespace webrtc

#endif  // WEBRTC_API_RTPRECEIVER_H_
