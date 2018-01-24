/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/rtpsender.h"

#include "webrtc/api/localaudiosource.h"
#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/trace_event.h"

namespace webrtc {

LocalAudioSinkAdapter::LocalAudioSinkAdapter() : sink_(nullptr) {}

LocalAudioSinkAdapter::~LocalAudioSinkAdapter() {
  rtc::CritScope lock(&lock_);
  if (sink_)
    sink_->OnClose();
}

void LocalAudioSinkAdapter::OnData(const void* audio_data,
                                   int bits_per_sample,
                                   int sample_rate,
                                   size_t number_of_channels,
                                   size_t number_of_frames) {
  rtc::CritScope lock(&lock_);
  if (sink_) {
    sink_->OnData(audio_data, bits_per_sample, sample_rate, number_of_channels,
                  number_of_frames);
  }
}

void LocalAudioSinkAdapter::SetSink(cricket::AudioSource::Sink* sink) {
  rtc::CritScope lock(&lock_);
  ASSERT(!sink || !sink_);
  sink_ = sink;
}

AudioRtpSender::AudioRtpSender(AudioTrackInterface* track,
                               const std::string& stream_id,
                               AudioProviderInterface* provider,
                               StatsCollector* stats)
    : id_(track->id()),
      stream_id_(stream_id),
      provider_(provider),
      stats_(stats),
      track_(track),
      cached_track_enabled_(track->enabled()),
      sink_adapter_(new LocalAudioSinkAdapter()) {
  RTC_DCHECK(provider != nullptr);
  track_->RegisterObserver(this);
  track_->AddSink(sink_adapter_.get());
}

AudioRtpSender::AudioRtpSender(AudioTrackInterface* track,
                               AudioProviderInterface* provider,
                               StatsCollector* stats)
    : id_(track->id()),
      stream_id_(rtc::CreateRandomUuid()),
      provider_(provider),
      stats_(stats),
      track_(track),
      cached_track_enabled_(track->enabled()),
      sink_adapter_(new LocalAudioSinkAdapter()) {
  RTC_DCHECK(provider != nullptr);
  track_->RegisterObserver(this);
  track_->AddSink(sink_adapter_.get());
}

AudioRtpSender::AudioRtpSender(AudioProviderInterface* provider,
                               StatsCollector* stats)
    : id_(rtc::CreateRandomUuid()),
      stream_id_(rtc::CreateRandomUuid()),
      provider_(provider),
      stats_(stats),
      sink_adapter_(new LocalAudioSinkAdapter()) {}

AudioRtpSender::~AudioRtpSender() {
  Stop();
}

void AudioRtpSender::OnChanged() {
  TRACE_EVENT0("webrtc", "AudioRtpSender::OnChanged");
  RTC_DCHECK(!stopped_);
  if (cached_track_enabled_ != track_->enabled()) {
    cached_track_enabled_ = track_->enabled();
    if (can_send_track()) {
      SetAudioSend();
    }
  }
}

bool AudioRtpSender::SetTrack(MediaStreamTrackInterface* track) {
  TRACE_EVENT0("webrtc", "AudioRtpSender::SetTrack");
  if (stopped_) {
    LOG(LS_ERROR) << "SetTrack can't be called on a stopped RtpSender.";
    return false;
  }
  if (track && track->kind() != MediaStreamTrackInterface::kAudioKind) {
    LOG(LS_ERROR) << "SetTrack called on audio RtpSender with " << track->kind()
                  << " track.";
    return false;
  }
  AudioTrackInterface* audio_track = static_cast<AudioTrackInterface*>(track);

  // Detach from old track.
  if (track_) {
    track_->RemoveSink(sink_adapter_.get());
    track_->UnregisterObserver(this);
  }

  if (can_send_track() && stats_) {
    stats_->RemoveLocalAudioTrack(track_.get(), ssrc_);
  }

  // Attach to new track.
  bool prev_can_send_track = can_send_track();
  // Keep a reference to the old track to keep it alive until we call
  // SetAudioSend.
  rtc::scoped_refptr<AudioTrackInterface> old_track = track_;
  track_ = audio_track;
  if (track_) {
    cached_track_enabled_ = track_->enabled();
    track_->RegisterObserver(this);
    track_->AddSink(sink_adapter_.get());
  }

  // Update audio provider.
  if (can_send_track()) {
    SetAudioSend();
    if (stats_) {
      stats_->AddLocalAudioTrack(track_.get(), ssrc_);
    }
  } else if (prev_can_send_track) {
    cricket::AudioOptions options;
    provider_->SetAudioSend(ssrc_, false, options, nullptr);
  }
  return true;
}

RtpParameters AudioRtpSender::GetParameters() const {
  return provider_->GetAudioRtpSendParameters(ssrc_);
}

bool AudioRtpSender::SetParameters(const RtpParameters& parameters) {
  TRACE_EVENT0("webrtc", "AudioRtpSender::SetParameters");
  return provider_->SetAudioRtpSendParameters(ssrc_, parameters);
}

void AudioRtpSender::SetSsrc(uint32_t ssrc) {
  TRACE_EVENT0("webrtc", "AudioRtpSender::SetSsrc");
  if (stopped_ || ssrc == ssrc_) {
    return;
  }
  // If we are already sending with a particular SSRC, stop sending.
  if (can_send_track()) {
    cricket::AudioOptions options;
    provider_->SetAudioSend(ssrc_, false, options, nullptr);
    if (stats_) {
      stats_->RemoveLocalAudioTrack(track_.get(), ssrc_);
    }
  }
  ssrc_ = ssrc;
  if (can_send_track()) {
    SetAudioSend();
    if (stats_) {
      stats_->AddLocalAudioTrack(track_.get(), ssrc_);
    }
  }
}

void AudioRtpSender::Stop() {
  TRACE_EVENT0("webrtc", "AudioRtpSender::Stop");
  // TODO(deadbeef): Need to do more here to fully stop sending packets.
  if (stopped_) {
    return;
  }
  if (track_) {
    track_->RemoveSink(sink_adapter_.get());
    track_->UnregisterObserver(this);
  }
  if (can_send_track()) {
    cricket::AudioOptions options;
    provider_->SetAudioSend(ssrc_, false, options, nullptr);
    if (stats_) {
      stats_->RemoveLocalAudioTrack(track_.get(), ssrc_);
    }
  }
  stopped_ = true;
}

void AudioRtpSender::SetAudioSend() {
  RTC_DCHECK(!stopped_ && can_send_track());
  cricket::AudioOptions options;
#if !defined(WEBRTC_CHROMIUM_BUILD)
  // TODO(tommi): Remove this hack when we move CreateAudioSource out of
  // PeerConnection.  This is a bit of a strange way to apply local audio
  // options since it is also applied to all streams/channels, local or remote.
  if (track_->enabled() && track_->GetSource() &&
      !track_->GetSource()->remote()) {
    // TODO(xians): Remove this static_cast since we should be able to connect
    // a remote audio track to a peer connection.
    options = static_cast<LocalAudioSource*>(track_->GetSource())->options();
  }
#endif

  cricket::AudioSource* source = sink_adapter_.get();
  ASSERT(source != nullptr);
  provider_->SetAudioSend(ssrc_, track_->enabled(), options, source);
}

VideoRtpSender::VideoRtpSender(VideoTrackInterface* track,
                               const std::string& stream_id,
                               VideoProviderInterface* provider)
    : id_(track->id()),
      stream_id_(stream_id),
      provider_(provider),
      track_(track),
      cached_track_enabled_(track->enabled()) {
  RTC_DCHECK(provider != nullptr);
  track_->RegisterObserver(this);
}

VideoRtpSender::VideoRtpSender(VideoTrackInterface* track,
                               VideoProviderInterface* provider)
    : id_(track->id()),
      stream_id_(rtc::CreateRandomUuid()),
      provider_(provider),
      track_(track),
      cached_track_enabled_(track->enabled()) {
  RTC_DCHECK(provider != nullptr);
  track_->RegisterObserver(this);
}

VideoRtpSender::VideoRtpSender(VideoProviderInterface* provider)
    : id_(rtc::CreateRandomUuid()),
      stream_id_(rtc::CreateRandomUuid()),
      provider_(provider) {}

VideoRtpSender::~VideoRtpSender() {
  Stop();
}

void VideoRtpSender::OnChanged() {
  TRACE_EVENT0("webrtc", "VideoRtpSender::OnChanged");
  RTC_DCHECK(!stopped_);
  if (cached_track_enabled_ != track_->enabled()) {
    cached_track_enabled_ = track_->enabled();
    if (can_send_track()) {
      SetVideoSend();
    }
  }
}

bool VideoRtpSender::SetTrack(MediaStreamTrackInterface* track) {
  TRACE_EVENT0("webrtc", "VideoRtpSender::SetTrack");
  if (stopped_) {
    LOG(LS_ERROR) << "SetTrack can't be called on a stopped RtpSender.";
    return false;
  }
  if (track && track->kind() != MediaStreamTrackInterface::kVideoKind) {
    LOG(LS_ERROR) << "SetTrack called on video RtpSender with " << track->kind()
                  << " track.";
    return false;
  }
  VideoTrackInterface* video_track = static_cast<VideoTrackInterface*>(track);

  // Detach from old track.
  if (track_) {
    track_->UnregisterObserver(this);
  }

  // Attach to new track.
  bool prev_can_send_track = can_send_track();
  // Keep a reference to the old track to keep it alive until we call
  // SetVideoSend.
  rtc::scoped_refptr<VideoTrackInterface> old_track = track_;
  track_ = video_track;
  if (track_) {
    cached_track_enabled_ = track_->enabled();
    track_->RegisterObserver(this);
  }

  // Update video provider.
  if (can_send_track()) {
    SetVideoSend();
  } else if (prev_can_send_track) {
    ClearVideoSend();
  }
  return true;
}

RtpParameters VideoRtpSender::GetParameters() const {
  return provider_->GetVideoRtpSendParameters(ssrc_);
}

bool VideoRtpSender::SetParameters(const RtpParameters& parameters) {
  TRACE_EVENT0("webrtc", "VideoRtpSender::SetParameters");
  return provider_->SetVideoRtpSendParameters(ssrc_, parameters);
}

void VideoRtpSender::SetSsrc(uint32_t ssrc) {
  TRACE_EVENT0("webrtc", "VideoRtpSender::SetSsrc");
  if (stopped_ || ssrc == ssrc_) {
    return;
  }
  // If we are already sending with a particular SSRC, stop sending.
  if (can_send_track()) {
    ClearVideoSend();
  }
  ssrc_ = ssrc;
  if (can_send_track()) {
    SetVideoSend();
  }
}

void VideoRtpSender::Stop() {
  TRACE_EVENT0("webrtc", "VideoRtpSender::Stop");
  // TODO(deadbeef): Need to do more here to fully stop sending packets.
  if (stopped_) {
    return;
  }
  if (track_) {
    track_->UnregisterObserver(this);
  }
  if (can_send_track()) {
    ClearVideoSend();
  }
  stopped_ = true;
}

void VideoRtpSender::SetVideoSend() {
  RTC_DCHECK(!stopped_ && can_send_track());
  cricket::VideoOptions options;
  VideoTrackSourceInterface* source = track_->GetSource();
  if (source) {
    options.is_screencast = rtc::Optional<bool>(source->is_screencast());
    options.video_noise_reduction = source->needs_denoising();
  }
  provider_->SetVideoSend(ssrc_, track_->enabled(), &options, track_);
}

void VideoRtpSender::ClearVideoSend() {
  RTC_DCHECK(ssrc_ != 0);
  RTC_DCHECK(provider_ != nullptr);
  provider_->SetVideoSend(ssrc_, false, nullptr, nullptr);
}

}  // namespace webrtc
