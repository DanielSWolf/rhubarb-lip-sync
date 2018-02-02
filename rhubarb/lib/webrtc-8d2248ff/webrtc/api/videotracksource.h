/*
 *  Copyright 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_VIDEOTRACKSOURCE_H_
#define WEBRTC_API_VIDEOTRACKSOURCE_H_

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/notifier.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/media/base/mediachannel.h"
#include "webrtc/media/base/videosinkinterface.h"

// VideoTrackSource implements VideoTrackSourceInterface.
namespace webrtc {

class VideoTrackSource : public Notifier<VideoTrackSourceInterface> {
 public:
  VideoTrackSource(rtc::VideoSourceInterface<cricket::VideoFrame>* source,
                   bool remote);
  void SetState(SourceState new_state);
  // OnSourceDestroyed clears this instance pointer to |source_|. It is useful
  // when the underlying rtc::VideoSourceInterface is destroyed before the
  // reference counted VideoTrackSource.
  void OnSourceDestroyed();

  SourceState state() const override { return state_; }
  bool remote() const override { return remote_; }

  void Stop() override{};
  void Restart() override{};

  bool is_screencast() const override { return false; }
  rtc::Optional<bool> needs_denoising() const override {
    return rtc::Optional<bool>(); }

  bool GetStats(Stats* stats) override { return false; }

  void AddOrUpdateSink(rtc::VideoSinkInterface<cricket::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<cricket::VideoFrame>* sink) override;

 private:
  rtc::ThreadChecker worker_thread_checker_;
  rtc::VideoSourceInterface<cricket::VideoFrame>* source_;
  cricket::VideoOptions options_;
  SourceState state_;
  const bool remote_;
};

}  // namespace webrtc

#endif  //  WEBRTC_API_VIDEOTRACKSOURCE_H_
