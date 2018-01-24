/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_AUDIO_SEND_STREAM_H_
#define WEBRTC_AUDIO_AUDIO_SEND_STREAM_H_

#include <memory>

#include "webrtc/audio_send_stream.h"
#include "webrtc/audio_state.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/thread_checker.h"

namespace webrtc {
class CongestionController;
class VoiceEngine;

namespace voe {
class ChannelProxy;
}  // namespace voe

namespace internal {
class AudioSendStream final : public webrtc::AudioSendStream {
 public:
  AudioSendStream(const webrtc::AudioSendStream::Config& config,
                  const rtc::scoped_refptr<webrtc::AudioState>& audio_state,
                  CongestionController* congestion_controller);
  ~AudioSendStream() override;

  // webrtc::AudioSendStream implementation.
  void Start() override;
  void Stop() override;
  bool SendTelephoneEvent(int payload_type, int event,
                          int duration_ms) override;
  void SetMuted(bool muted) override;
  webrtc::AudioSendStream::Stats GetStats() const override;

  void SignalNetworkState(NetworkState state);
  bool DeliverRtcp(const uint8_t* packet, size_t length);
  const webrtc::AudioSendStream::Config& config() const;

 private:
  VoiceEngine* voice_engine() const;

  rtc::ThreadChecker thread_checker_;
  const webrtc::AudioSendStream::Config config_;
  rtc::scoped_refptr<webrtc::AudioState> audio_state_;
  std::unique_ptr<voe::ChannelProxy> channel_proxy_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(AudioSendStream);
};
}  // namespace internal
}  // namespace webrtc

#endif  // WEBRTC_AUDIO_AUDIO_SEND_STREAM_H_
