/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <string.h>

#include <memory>

#include "webrtc/base/gunit.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/media/engine/webrtcvideoframe.h"
#include "webrtc/media/engine/webrtcvideoframefactory.h"

class WebRtcVideoFrameFactoryTest : public testing::Test {
 public:
  WebRtcVideoFrameFactoryTest() {}

  void InitFrame(webrtc::VideoRotation frame_rotation) {
    const int frame_width = 1920;
    const int frame_height = 1080;

    // Build the CapturedFrame.
    captured_frame_.fourcc = cricket::FOURCC_I420;
    captured_frame_.pixel_width = 1;
    captured_frame_.pixel_height = 1;
    captured_frame_.time_stamp = rtc::TimeNanos();
    captured_frame_.rotation = frame_rotation;
    captured_frame_.width = frame_width;
    captured_frame_.height = frame_height;
    captured_frame_.data_size =
        (frame_width * frame_height) +
        ((frame_width + 1) / 2) * ((frame_height + 1) / 2) * 2;
    captured_frame_buffer_.reset(new uint8_t[captured_frame_.data_size]);
    // Initialize memory to satisfy DrMemory tests.
    memset(captured_frame_buffer_.get(), 0, captured_frame_.data_size);
    captured_frame_.data = captured_frame_buffer_.get();
  }

  void VerifyFrame(cricket::VideoFrame* dest_frame,
                   webrtc::VideoRotation src_rotation,
                   int src_width,
                   int src_height,
                   bool apply_rotation) {
    if (!apply_rotation) {
      EXPECT_EQ(dest_frame->rotation(), src_rotation);
      EXPECT_EQ(dest_frame->width(), src_width);
      EXPECT_EQ(dest_frame->height(), src_height);
    } else {
      EXPECT_EQ(dest_frame->rotation(), webrtc::kVideoRotation_0);
      if (src_rotation == webrtc::kVideoRotation_90 ||
          src_rotation == webrtc::kVideoRotation_270) {
        EXPECT_EQ(dest_frame->width(), src_height);
        EXPECT_EQ(dest_frame->height(), src_width);
      } else {
        EXPECT_EQ(dest_frame->width(), src_width);
        EXPECT_EQ(dest_frame->height(), src_height);
      }
    }
  }

  void TestCreateAliasedFrame(bool apply_rotation) {
    cricket::VideoFrameFactory& factory = factory_;
    factory.SetApplyRotation(apply_rotation);
    InitFrame(webrtc::kVideoRotation_270);
    const cricket::CapturedFrame& captured_frame = get_captured_frame();
    // Create the new frame from the CapturedFrame.
    std::unique_ptr<cricket::VideoFrame> frame;
    int new_width = captured_frame.width / 2;
    int new_height = captured_frame.height / 2;
    frame.reset(factory.CreateAliasedFrame(&captured_frame, new_width,
                                           new_height, new_width, new_height));
    VerifyFrame(frame.get(), webrtc::kVideoRotation_270, new_width, new_height,
                apply_rotation);

    frame.reset(factory.CreateAliasedFrame(
        &captured_frame, new_width, new_height, new_width / 2, new_height / 2));
    VerifyFrame(frame.get(), webrtc::kVideoRotation_270, new_width / 2,
                new_height / 2, apply_rotation);

    // Reset the frame first so it's exclusive hence we could go through the
    // StretchToFrame code path in CreateAliasedFrame.
    frame.reset();
    frame.reset(factory.CreateAliasedFrame(
        &captured_frame, new_width, new_height, new_width / 2, new_height / 2));
    VerifyFrame(frame.get(), webrtc::kVideoRotation_270, new_width / 2,
                new_height / 2, apply_rotation);
  }

  const cricket::CapturedFrame& get_captured_frame() { return captured_frame_; }

 private:
  cricket::CapturedFrame captured_frame_;
  std::unique_ptr<uint8_t[]> captured_frame_buffer_;
  cricket::WebRtcVideoFrameFactory factory_;
};

TEST_F(WebRtcVideoFrameFactoryTest, NoApplyRotation) {
  TestCreateAliasedFrame(false);
}

TEST_F(WebRtcVideoFrameFactoryTest, ApplyRotation) {
  TestCreateAliasedFrame(true);
}
