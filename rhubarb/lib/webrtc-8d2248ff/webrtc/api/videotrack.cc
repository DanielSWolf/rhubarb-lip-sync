/*
 *  Copyright 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/videotrack.h"

#include <string>

namespace webrtc {

const char MediaStreamTrackInterface::kVideoKind[] = "video";

VideoTrack::VideoTrack(const std::string& label,
                       VideoTrackSourceInterface* video_source)
    : MediaStreamTrack<VideoTrackInterface>(label),
      video_source_(video_source) {
  worker_thread_checker_.DetachFromThread();
  video_source_->RegisterObserver(this);
}

VideoTrack::~VideoTrack() {
  video_source_->UnregisterObserver(this);
}

std::string VideoTrack::kind() const {
  return kVideoKind;
}

// AddOrUpdateSink and RemoveSink should be called on the worker
// thread.
void VideoTrack::AddOrUpdateSink(
    rtc::VideoSinkInterface<cricket::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  VideoSourceBase::AddOrUpdateSink(sink, wants);
  rtc::VideoSinkWants modified_wants = wants;
  modified_wants.black_frames = !enabled();
  video_source_->AddOrUpdateSink(sink, modified_wants);
}

void VideoTrack::RemoveSink(
    rtc::VideoSinkInterface<cricket::VideoFrame>* sink) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  VideoSourceBase::RemoveSink(sink);
  video_source_->RemoveSink(sink);
}

bool VideoTrack::set_enabled(bool enable) {
  RTC_DCHECK(signaling_thread_checker_.CalledOnValidThread());
  for (auto& sink_pair : sink_pairs()) {
    rtc::VideoSinkWants modified_wants = sink_pair.wants;
    modified_wants.black_frames = !enable;
    // video_source_ is a proxy object, marshalling the call to the
    // worker thread.
    video_source_->AddOrUpdateSink(sink_pair.sink, modified_wants);
  }
  return MediaStreamTrack<VideoTrackInterface>::set_enabled(enable);
}

void VideoTrack::OnChanged() {
  RTC_DCHECK(signaling_thread_checker_.CalledOnValidThread());
  if (video_source_->state() == MediaSourceInterface::kEnded) {
    set_state(kEnded);
  } else {
    set_state(kLive);
  }
}

rtc::scoped_refptr<VideoTrack> VideoTrack::Create(
    const std::string& id,
    VideoTrackSourceInterface* source) {
  rtc::RefCountedObject<VideoTrack>* track =
      new rtc::RefCountedObject<VideoTrack>(id, source);
  return track;
}

}  // namespace webrtc
