/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains classes that implement RtpSenderInterface.
// An RtpSender associates a MediaStreamTrackInterface with an underlying
// transport (provided by AudioProviderInterface/VideoProviderInterface)

#ifndef WEBRTC_API_RTPSENDER_H_
#define WEBRTC_API_RTPSENDER_H_

#include <memory>
#include <string>

#include "webrtc/api/mediastreamprovider.h"
#include "webrtc/api/rtpsenderinterface.h"
#include "webrtc/api/statscollector.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/media/base/audiosource.h"

namespace webrtc {

// Internal interface used by PeerConnection.
class RtpSenderInternal : public RtpSenderInterface {
 public:
  // Used to set the SSRC of the sender, once a local description has been set.
  // If |ssrc| is 0, this indiates that the sender should disconnect from the
  // underlying transport (this occurs if the sender isn't seen in a local
  // description).
  virtual void SetSsrc(uint32_t ssrc) = 0;

  // TODO(deadbeef): Support one sender having multiple stream ids.
  virtual void set_stream_id(const std::string& stream_id) = 0;
  virtual std::string stream_id() const = 0;

  virtual void Stop() = 0;
};

// LocalAudioSinkAdapter receives data callback as a sink to the local
// AudioTrack, and passes the data to the sink of AudioSource.
class LocalAudioSinkAdapter : public AudioTrackSinkInterface,
                              public cricket::AudioSource {
 public:
  LocalAudioSinkAdapter();
  virtual ~LocalAudioSinkAdapter();

 private:
  // AudioSinkInterface implementation.
  void OnData(const void* audio_data,
              int bits_per_sample,
              int sample_rate,
              size_t number_of_channels,
              size_t number_of_frames) override;

  // cricket::AudioSource implementation.
  void SetSink(cricket::AudioSource::Sink* sink) override;

  cricket::AudioSource::Sink* sink_;
  // Critical section protecting |sink_|.
  rtc::CriticalSection lock_;
};

class AudioRtpSender : public ObserverInterface,
                       public rtc::RefCountedObject<RtpSenderInternal> {
 public:
  // StatsCollector provided so that Add/RemoveLocalAudioTrack can be called
  // at the appropriate times.
  AudioRtpSender(AudioTrackInterface* track,
                 const std::string& stream_id,
                 AudioProviderInterface* provider,
                 StatsCollector* stats);

  // Randomly generates stream_id.
  AudioRtpSender(AudioTrackInterface* track,
                 AudioProviderInterface* provider,
                 StatsCollector* stats);

  // Randomly generates id and stream_id.
  AudioRtpSender(AudioProviderInterface* provider, StatsCollector* stats);

  virtual ~AudioRtpSender();

  // ObserverInterface implementation
  void OnChanged() override;

  // RtpSenderInterface implementation
  bool SetTrack(MediaStreamTrackInterface* track) override;
  rtc::scoped_refptr<MediaStreamTrackInterface> track() const override {
    return track_;
  }

  uint32_t ssrc() const override { return ssrc_; }

  cricket::MediaType media_type() const override {
    return cricket::MEDIA_TYPE_AUDIO;
  }

  std::string id() const override { return id_; }

  std::vector<std::string> stream_ids() const override {
    std::vector<std::string> ret = {stream_id_};
    return ret;
  }

  RtpParameters GetParameters() const override;
  bool SetParameters(const RtpParameters& parameters) override;

  // RtpSenderInternal implementation.
  void SetSsrc(uint32_t ssrc) override;

  void set_stream_id(const std::string& stream_id) override {
    stream_id_ = stream_id;
  }
  std::string stream_id() const override { return stream_id_; }

  void Stop() override;

 private:
  // TODO(nisse): Since SSRC == 0 is technically valid, figure out
  // some other way to test if we have a valid SSRC.
  bool can_send_track() const { return track_ && ssrc_; }
  // Helper function to construct options for
  // AudioProviderInterface::SetAudioSend.
  void SetAudioSend();

  std::string id_;
  std::string stream_id_;
  AudioProviderInterface* provider_;
  StatsCollector* stats_;
  rtc::scoped_refptr<AudioTrackInterface> track_;
  uint32_t ssrc_ = 0;
  bool cached_track_enabled_ = false;
  bool stopped_ = false;

  // Used to pass the data callback from the |track_| to the other end of
  // cricket::AudioSource.
  std::unique_ptr<LocalAudioSinkAdapter> sink_adapter_;
};

class VideoRtpSender : public ObserverInterface,
                       public rtc::RefCountedObject<RtpSenderInternal> {
 public:
  VideoRtpSender(VideoTrackInterface* track,
                 const std::string& stream_id,
                 VideoProviderInterface* provider);

  // Randomly generates stream_id.
  VideoRtpSender(VideoTrackInterface* track, VideoProviderInterface* provider);

  // Randomly generates id and stream_id.
  explicit VideoRtpSender(VideoProviderInterface* provider);

  virtual ~VideoRtpSender();

  // ObserverInterface implementation
  void OnChanged() override;

  // RtpSenderInterface implementation
  bool SetTrack(MediaStreamTrackInterface* track) override;
  rtc::scoped_refptr<MediaStreamTrackInterface> track() const override {
    return track_;
  }

  uint32_t ssrc() const override { return ssrc_; }

  cricket::MediaType media_type() const override {
    return cricket::MEDIA_TYPE_VIDEO;
  }

  std::string id() const override { return id_; }

  std::vector<std::string> stream_ids() const override {
    std::vector<std::string> ret = {stream_id_};
    return ret;
  }

  RtpParameters GetParameters() const override;
  bool SetParameters(const RtpParameters& parameters) override;

  // RtpSenderInternal implementation.
  void SetSsrc(uint32_t ssrc) override;

  void set_stream_id(const std::string& stream_id) override {
    stream_id_ = stream_id;
  }
  std::string stream_id() const override { return stream_id_; }

  void Stop() override;

 private:
  bool can_send_track() const { return track_ && ssrc_; }
  // Helper function to construct options for
  // VideoProviderInterface::SetVideoSend.
  void SetVideoSend();
  // Helper function to call SetVideoSend with "stop sending" parameters.
  void ClearVideoSend();

  std::string id_;
  std::string stream_id_;
  VideoProviderInterface* provider_;
  rtc::scoped_refptr<VideoTrackInterface> track_;
  uint32_t ssrc_ = 0;
  bool cached_track_enabled_ = false;
  bool stopped_ = false;
};

}  // namespace webrtc

#endif  // WEBRTC_API_RTPSENDER_H_
