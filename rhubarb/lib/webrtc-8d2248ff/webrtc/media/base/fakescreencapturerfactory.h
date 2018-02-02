/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_BASE_FAKESCREENCAPTURERFACTORY_H_
#define WEBRTC_MEDIA_BASE_FAKESCREENCAPTURERFACTORY_H_

#include "webrtc/media/base/fakevideocapturer.h"
#include "webrtc/media/base/videocapturerfactory.h"

namespace cricket {

class FakeScreenCapturerFactory
    : public cricket::ScreenCapturerFactory,
      public sigslot::has_slots<> {
 public:
  FakeScreenCapturerFactory()
      : window_capturer_(NULL),
        capture_state_(cricket::CS_STOPPED) {}

  virtual cricket::VideoCapturer* Create(const ScreencastId& window) {
    if (window_capturer_ != NULL) {
      return NULL;
    }
    window_capturer_ = new cricket::FakeVideoCapturer;
    window_capturer_->SignalDestroyed.connect(
        this,
        &FakeScreenCapturerFactory::OnWindowCapturerDestroyed);
    window_capturer_->SignalStateChange.connect(
        this,
        &FakeScreenCapturerFactory::OnStateChange);
    return window_capturer_;
  }

  cricket::FakeVideoCapturer* window_capturer() { return window_capturer_; }

  cricket::CaptureState capture_state() { return capture_state_; }

 private:
  void OnWindowCapturerDestroyed(cricket::FakeVideoCapturer* capturer) {
    if (capturer == window_capturer_) {
      window_capturer_ = NULL;
    }
  }
  void OnStateChange(cricket::VideoCapturer*, cricket::CaptureState state) {
    capture_state_ = state;
  }

  cricket::FakeVideoCapturer* window_capturer_;
  cricket::CaptureState capture_state_;
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_BASE_FAKESCREENCAPTURERFACTORY_H_
