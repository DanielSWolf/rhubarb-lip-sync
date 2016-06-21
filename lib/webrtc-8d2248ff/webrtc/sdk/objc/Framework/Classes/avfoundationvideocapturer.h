/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_OBJC_AVFOUNDATION_VIDEO_CAPTURER_H_
#define WEBRTC_API_OBJC_AVFOUNDATION_VIDEO_CAPTURER_H_

#import <AVFoundation/AVFoundation.h>

#include "webrtc/media/base/videocapturer.h"
#include "webrtc/video_frame.h"

@class RTCAVFoundationVideoCapturerInternal;

namespace rtc {
class Thread;
}  // namespace rtc

namespace webrtc {

class AVFoundationVideoCapturer : public cricket::VideoCapturer,
                                  public rtc::MessageHandler {
 public:
  AVFoundationVideoCapturer();
  ~AVFoundationVideoCapturer();

  cricket::CaptureState Start(const cricket::VideoFormat& format) override;
  void Stop() override;
  bool IsRunning() override;
  bool IsScreencast() const override {
    return false;
  }
  bool GetPreferredFourccs(std::vector<uint32_t> *fourccs) override {
    fourccs->push_back(cricket::FOURCC_NV12);
    return true;
  }

  // Returns the active capture session. Calls to the capture session should
  // occur on the RTCDispatcherTypeCaptureSession queue in RTCDispatcher.
  AVCaptureSession* GetCaptureSession();

  // Returns whether the rear-facing camera can be used.
  // e.g. It can't be used because it doesn't exist.
  bool CanUseBackCamera() const;

  // Switches the camera being used (either front or back).
  void SetUseBackCamera(bool useBackCamera);
  bool GetUseBackCamera() const;

  // Converts the sample buffer into a cricket::CapturedFrame and signals the
  // frame for capture.
  void CaptureSampleBuffer(CMSampleBufferRef sampleBuffer);

  // Handles messages from posts.
  void OnMessage(rtc::Message *msg) override;

 private:
  void OnFrameMessage(CVImageBufferRef image_buffer, int64_t capture_time);

  RTCAVFoundationVideoCapturerInternal *_capturer;
  rtc::Thread *_startThread;  // Set in Start(), unset in Stop().
};  // AVFoundationVideoCapturer

}  // namespace webrtc

#endif  // WEBRTC_API_OBJC_AVFOUNDATION_VIDEO_CAPTURER_H_
