/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <string>
#include <vector>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/api/videocapturertracksource.h"
#include "webrtc/base/gunit.h"
#include "webrtc/media/base/fakemediaengine.h"
#include "webrtc/media/base/fakevideocapturer.h"
#include "webrtc/media/base/fakevideorenderer.h"
#include "webrtc/media/engine/webrtcvideoframe.h"

using webrtc::FakeConstraints;
using webrtc::VideoCapturerTrackSource;
using webrtc::MediaConstraintsInterface;
using webrtc::MediaSourceInterface;
using webrtc::ObserverInterface;
using webrtc::VideoTrackSourceInterface;

namespace {

// Max wait time for a test.
const int kMaxWaitMs = 100;

}  // anonymous namespace

// TestVideoCapturer extends cricket::FakeVideoCapturer so it can be used for
// testing without known camera formats.
// It keeps its own lists of cricket::VideoFormats for the unit tests in this
// file.
class TestVideoCapturer : public cricket::FakeVideoCapturer {
 public:
  explicit TestVideoCapturer(bool is_screencast)
      : FakeVideoCapturer(is_screencast), test_without_formats_(false) {
    std::vector<cricket::VideoFormat> formats;
    formats.push_back(
        cricket::VideoFormat(1280, 720, cricket::VideoFormat::FpsToInterval(30),
                             cricket::FOURCC_I420));
    formats.push_back(
        cricket::VideoFormat(640, 480, cricket::VideoFormat::FpsToInterval(30),
                             cricket::FOURCC_I420));
    formats.push_back(
        cricket::VideoFormat(640, 400, cricket::VideoFormat::FpsToInterval(30),
                             cricket::FOURCC_I420));
    formats.push_back(
        cricket::VideoFormat(320, 240, cricket::VideoFormat::FpsToInterval(30),
                             cricket::FOURCC_I420));
    formats.push_back(
        cricket::VideoFormat(352, 288, cricket::VideoFormat::FpsToInterval(30),
                             cricket::FOURCC_I420));
    ResetSupportedFormats(formats);
  }

  // This function is used for resetting the supported capture formats and
  // simulating a cricket::VideoCapturer implementation that don't support
  // capture format enumeration. This is used to simulate the current
  // Chrome implementation.
  void TestWithoutCameraFormats() {
    test_without_formats_ = true;
    std::vector<cricket::VideoFormat> formats;
    ResetSupportedFormats(formats);
  }

  virtual cricket::CaptureState Start(
      const cricket::VideoFormat& capture_format) {
    if (test_without_formats_) {
      std::vector<cricket::VideoFormat> formats;
      formats.push_back(capture_format);
      ResetSupportedFormats(formats);
    }
    return FakeVideoCapturer::Start(capture_format);
  }

  virtual bool GetBestCaptureFormat(const cricket::VideoFormat& desired,
                                    cricket::VideoFormat* best_format) {
    if (test_without_formats_) {
      *best_format = desired;
      return true;
    }
    return FakeVideoCapturer::GetBestCaptureFormat(desired, best_format);
  }

 private:
  bool test_without_formats_;
};

class StateObserver : public ObserverInterface {
 public:
  explicit StateObserver(VideoTrackSourceInterface* source)
      : state_(source->state()), source_(source) {}
  virtual void OnChanged() { state_ = source_->state(); }
  MediaSourceInterface::SourceState state() const { return state_; }

 private:
  MediaSourceInterface::SourceState state_;
  rtc::scoped_refptr<VideoTrackSourceInterface> source_;
};

class VideoCapturerTrackSourceTest : public testing::Test {
 protected:
  VideoCapturerTrackSourceTest() { InitCapturer(false); }
  void InitCapturer(bool is_screencast) {
    capturer_cleanup_ = std::unique_ptr<TestVideoCapturer>(
        new TestVideoCapturer(is_screencast));
    capturer_ = capturer_cleanup_.get();
  }

  void InitScreencast() { InitCapturer(true); }

  void CreateVideoCapturerSource() { CreateVideoCapturerSource(NULL); }

  void CreateVideoCapturerSource(
      const webrtc::MediaConstraintsInterface* constraints) {
    // VideoSource take ownership of |capturer_|
    source_ = VideoCapturerTrackSource::Create(rtc::Thread::Current(),
                                               capturer_cleanup_.release(),
                                               constraints, false);

    ASSERT_TRUE(source_.get() != NULL);

    state_observer_.reset(new StateObserver(source_));
    source_->RegisterObserver(state_observer_.get());
    source_->AddOrUpdateSink(&renderer_, rtc::VideoSinkWants());
  }

  std::unique_ptr<TestVideoCapturer> capturer_cleanup_;
  TestVideoCapturer* capturer_;
  cricket::FakeVideoRenderer renderer_;
  std::unique_ptr<StateObserver> state_observer_;
  rtc::scoped_refptr<VideoTrackSourceInterface> source_;
};

// Test that a VideoSource transition to kLive state when the capture
// device have started and kEnded if it is stopped.
// It also test that an output can receive video frames.
TEST_F(VideoCapturerTrackSourceTest, CapturerStartStop) {
  // Initialize without constraints.
  CreateVideoCapturerSource();
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);

  ASSERT_TRUE(capturer_->CaptureFrame());
  EXPECT_EQ(1, renderer_.num_rendered_frames());

  capturer_->Stop();
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
}

// Test that a VideoSource can be stopped and restarted.
TEST_F(VideoCapturerTrackSourceTest, StopRestart) {
  // Initialize without constraints.
  CreateVideoCapturerSource();
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);

  ASSERT_TRUE(capturer_->CaptureFrame());
  EXPECT_EQ(1, renderer_.num_rendered_frames());

  source_->Stop();
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);

  source_->Restart();
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);

  ASSERT_TRUE(capturer_->CaptureFrame());
  EXPECT_EQ(2, renderer_.num_rendered_frames());

  source_->Stop();
}

// Test that a VideoSource transition to kEnded if the capture device
// fails.
TEST_F(VideoCapturerTrackSourceTest, CameraFailed) {
  CreateVideoCapturerSource();
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);

  capturer_->SignalStateChange(capturer_, cricket::CS_FAILED);
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
}

// Test that the capture output is CIF if we set max constraints to CIF.
// and the capture device support CIF.
TEST_F(VideoCapturerTrackSourceTest, MandatoryConstraintCif5Fps) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMaxWidth, 352);
  constraints.AddMandatory(MediaConstraintsInterface::kMaxHeight, 288);
  constraints.AddMandatory(MediaConstraintsInterface::kMaxFrameRate, 5);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(352, format->width);
  EXPECT_EQ(288, format->height);
  EXPECT_EQ(30, format->framerate());
}

// Test that the capture output is 720P if the camera support it and the
// optional constraint is set to 720P.
TEST_F(VideoCapturerTrackSourceTest, MandatoryMinVgaOptional720P) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMinWidth, 640);
  constraints.AddMandatory(MediaConstraintsInterface::kMinHeight, 480);
  constraints.AddOptional(MediaConstraintsInterface::kMinWidth, 1280);
  constraints.AddOptional(MediaConstraintsInterface::kMinAspectRatio,
                          1280.0 / 720);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(1280, format->width);
  EXPECT_EQ(720, format->height);
  EXPECT_EQ(30, format->framerate());
}

// Test that the capture output have aspect ratio 4:3 if a mandatory constraint
// require it even if an optional constraint request a higher resolution
// that don't have this aspect ratio.
TEST_F(VideoCapturerTrackSourceTest, MandatoryAspectRatio4To3) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMinWidth, 640);
  constraints.AddMandatory(MediaConstraintsInterface::kMinHeight, 480);
  constraints.AddMandatory(MediaConstraintsInterface::kMaxAspectRatio,
                           640.0 / 480);
  constraints.AddOptional(MediaConstraintsInterface::kMinWidth, 1280);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(640, format->width);
  EXPECT_EQ(480, format->height);
  EXPECT_EQ(30, format->framerate());
}

// Test that the source state transition to kEnded if the mandatory aspect ratio
// is set higher than supported.
TEST_F(VideoCapturerTrackSourceTest, MandatoryAspectRatioTooHigh) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMinAspectRatio, 2);
  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
}

// Test that the source ignores an optional aspect ratio that is higher than
// supported.
TEST_F(VideoCapturerTrackSourceTest, OptionalAspectRatioTooHigh) {
  FakeConstraints constraints;
  constraints.AddOptional(MediaConstraintsInterface::kMinAspectRatio, 2);
  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  double aspect_ratio = static_cast<double>(format->width) / format->height;
  EXPECT_LT(aspect_ratio, 2);
}

// Test that the source starts video with the default resolution if the
// camera doesn't support capability enumeration and there are no constraints.
TEST_F(VideoCapturerTrackSourceTest, NoCameraCapability) {
  capturer_->TestWithoutCameraFormats();

  CreateVideoCapturerSource();
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(640, format->width);
  EXPECT_EQ(480, format->height);
  EXPECT_EQ(30, format->framerate());
}

// Test that the source can start the video and get the requested aspect ratio
// if the camera doesn't support capability enumeration and the aspect ratio is
// set.
TEST_F(VideoCapturerTrackSourceTest, NoCameraCapability16To9Ratio) {
  capturer_->TestWithoutCameraFormats();

  FakeConstraints constraints;
  double requested_aspect_ratio = 640.0 / 360;
  constraints.AddMandatory(MediaConstraintsInterface::kMinWidth, 640);
  constraints.AddMandatory(MediaConstraintsInterface::kMinAspectRatio,
                           requested_aspect_ratio);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  double aspect_ratio = static_cast<double>(format->width) / format->height;
  EXPECT_LE(requested_aspect_ratio, aspect_ratio);
}

// Test that the source state transitions to kEnded if an unknown mandatory
// constraint is found.
TEST_F(VideoCapturerTrackSourceTest, InvalidMandatoryConstraint) {
  FakeConstraints constraints;
  constraints.AddMandatory("weird key", 640);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
}

// Test that the source ignores an unknown optional constraint.
TEST_F(VideoCapturerTrackSourceTest, InvalidOptionalConstraint) {
  FakeConstraints constraints;
  constraints.AddOptional("weird key", 640);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
}

TEST_F(VideoCapturerTrackSourceTest, SetValidDenoisingConstraint) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kNoiseReduction, "false");

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ(rtc::Optional<bool>(false), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, NoiseReductionConstraintNotSet) {
  FakeConstraints constraints;
  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ(rtc::Optional<bool>(), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest,
       MandatoryDenoisingConstraintOverridesOptional) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kNoiseReduction, false);
  constraints.AddOptional(MediaConstraintsInterface::kNoiseReduction, true);

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ(rtc::Optional<bool>(false), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, NoiseReductionAndInvalidKeyOptional) {
  FakeConstraints constraints;
  constraints.AddOptional(MediaConstraintsInterface::kNoiseReduction, true);
  constraints.AddOptional("invalidKey", false);

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  EXPECT_EQ(rtc::Optional<bool>(true), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, NoiseReductionAndInvalidKeyMandatory) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kNoiseReduction, false);
  constraints.AddMandatory("invalidKey", false);

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
  EXPECT_EQ(rtc::Optional<bool>(), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, InvalidDenoisingValueOptional) {
  FakeConstraints constraints;
  constraints.AddOptional(MediaConstraintsInterface::kNoiseReduction,
                          "not a boolean");

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);

  EXPECT_EQ(rtc::Optional<bool>(), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, InvalidDenoisingValueMandatory) {
  FakeConstraints constraints;
  // Optional constraints should be ignored if the mandatory constraints fail.
  constraints.AddOptional(MediaConstraintsInterface::kNoiseReduction, "false");
  // Values are case-sensitive and must be all lower-case.
  constraints.AddMandatory(MediaConstraintsInterface::kNoiseReduction, "True");

  CreateVideoCapturerSource(&constraints);

  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
  EXPECT_EQ(rtc::Optional<bool>(), source_->needs_denoising());
}

TEST_F(VideoCapturerTrackSourceTest, MixedOptionsAndConstraints) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMaxWidth, 352);
  constraints.AddMandatory(MediaConstraintsInterface::kMaxHeight, 288);
  constraints.AddOptional(MediaConstraintsInterface::kMaxFrameRate, 5);

  constraints.AddMandatory(MediaConstraintsInterface::kNoiseReduction, false);
  constraints.AddOptional(MediaConstraintsInterface::kNoiseReduction, true);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(352, format->width);
  EXPECT_EQ(288, format->height);
  EXPECT_EQ(30, format->framerate());

  EXPECT_EQ(rtc::Optional<bool>(false), source_->needs_denoising());
}

// Tests that the source starts video with the default resolution for
// screencast if no constraint is set.
TEST_F(VideoCapturerTrackSourceTest, ScreencastResolutionNoConstraint) {
  InitScreencast();
  capturer_->TestWithoutCameraFormats();

  CreateVideoCapturerSource();
  ASSERT_TRUE(source_->is_screencast());
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(640, format->width);
  EXPECT_EQ(480, format->height);
  EXPECT_EQ(30, format->framerate());
}

// Tests that the source starts video with the max width and height set by
// constraints for screencast.
TEST_F(VideoCapturerTrackSourceTest, ScreencastResolutionWithConstraint) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMaxWidth, 480);
  constraints.AddMandatory(MediaConstraintsInterface::kMaxHeight, 270);

  InitScreencast();
  capturer_->TestWithoutCameraFormats();

  CreateVideoCapturerSource(&constraints);
  ASSERT_TRUE(source_->is_screencast());
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(480, format->width);
  EXPECT_EQ(270, format->height);
  EXPECT_EQ(30, format->framerate());
}

TEST_F(VideoCapturerTrackSourceTest, MandatorySubOneFpsConstraints) {
  FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kMaxFrameRate, 0.5);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kEnded, state_observer_->state(),
                 kMaxWaitMs);
  ASSERT_TRUE(capturer_->GetCaptureFormat() == NULL);
}

TEST_F(VideoCapturerTrackSourceTest, OptionalSubOneFpsConstraints) {
  FakeConstraints constraints;
  constraints.AddOptional(MediaConstraintsInterface::kMaxFrameRate, 0.5);

  CreateVideoCapturerSource(&constraints);
  EXPECT_EQ_WAIT(MediaSourceInterface::kLive, state_observer_->state(),
                 kMaxWaitMs);
  const cricket::VideoFormat* format = capturer_->GetCaptureFormat();
  ASSERT_TRUE(format != NULL);
  EXPECT_EQ(30, format->framerate());
}
