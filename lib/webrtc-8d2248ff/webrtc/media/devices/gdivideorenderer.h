/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
// Definition of class GdiVideoRenderer that implements the abstract class
// cricket::VideoRenderer via GDI on Windows.

#ifndef WEBRTC_MEDIA_DEVICES_GDIVIDEORENDERER_H_
#define WEBRTC_MEDIA_DEVICES_GDIVIDEORENDERER_H_
#ifdef WIN32

#include <memory>

#include "webrtc/media/base/videosinkinterface.h"

namespace cricket {

class VideoFrame;

class GdiVideoRenderer : public rtc::VideoSinkInterface<cricket::VideoFrame> {
 public:
  GdiVideoRenderer(int x, int y);
  virtual ~GdiVideoRenderer();

  // Implementation of VideoSinkInterface
  void OnFrame(const VideoFrame& frame) override;

 private:
  class VideoWindow;  // forward declaration, defined in the .cc file
  std::unique_ptr<VideoWindow> window_;
  // The initial position of the window.
  int initial_x_;
  int initial_y_;
};

}  // namespace cricket

#endif  // WIN32
#endif  // WEBRTC_MEDIA_DEVICES_GDIVIDEORENDERER_H_
