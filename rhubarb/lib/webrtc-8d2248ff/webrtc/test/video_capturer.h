/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef WEBRTC_TEST_VIDEO_CAPTURER_H_
#define WEBRTC_TEST_VIDEO_CAPTURER_H_

#include <stddef.h>

namespace webrtc {

class Clock;

class VideoCaptureInput;

namespace test {

class VideoCapturer {
 public:
  static VideoCapturer* Create(VideoCaptureInput* input,
                               size_t width,
                               size_t height,
                               int fps,
                               Clock* clock);
  virtual ~VideoCapturer() {}

  virtual void Start() = 0;
  virtual void Stop() = 0;

 protected:
  explicit VideoCapturer(VideoCaptureInput* input);
  VideoCaptureInput* input_;
};
}  // test
}  // webrtc

#endif  // WEBRTC_TEST_VIDEO_CAPTURER_H_
