/*
 *  Copyright (c) 2010 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Declaration of abstract class VideoCapturer

#ifndef WEBRTC_MEDIA_BASE_VIDEOCAPTURER_H_
#define WEBRTC_MEDIA_BASE_VIDEOCAPTURER_H_

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/media/base/videosourceinterface.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/media/base/videoadapter.h"
#include "webrtc/media/base/videobroadcaster.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/media/base/videoframefactory.h"


namespace cricket {

// Current state of the capturer.
enum CaptureState {
  CS_STOPPED,    // The capturer has been stopped or hasn't started yet.
  CS_STARTING,   // The capturer is in the process of starting. Note, it may
                 // still fail to start.
  CS_RUNNING,    // The capturer has been started successfully and is now
                 // capturing.
  CS_FAILED,     // The capturer failed to start.
};

class VideoFrame;

struct CapturedFrame {
  static const uint32_t kFrameHeaderSize = 40;  // Size from width to data_size.
  static const uint32_t kUnknownDataSize = 0xFFFFFFFF;

  CapturedFrame();

  // Get the number of bytes of the frame data. If data_size is known, return
  // it directly. Otherwise, calculate the size based on width, height, and
  // fourcc. Return true if succeeded.
  bool GetDataSize(uint32_t* size) const;

  // The width and height of the captured frame could be different from those
  // of VideoFormat. Once the first frame is captured, the width, height,
  // fourcc, pixel_width, and pixel_height should keep the same over frames.
  int width;              // in number of pixels
  int height;             // in number of pixels
  uint32_t fourcc;        // compression
  uint32_t pixel_width;   // width of a pixel, default is 1
  uint32_t pixel_height;  // height of a pixel, default is 1
  int64_t time_stamp;  // timestamp of when the frame was captured, in unix
                       // time with nanosecond units.
  uint32_t data_size;  // number of bytes of the frame data

  webrtc::VideoRotation rotation;  // rotation in degrees of the frame.

  void*  data;          // pointer to the frame data. This object allocates the
                        // memory or points to an existing memory.

 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(CapturedFrame);
};

// VideoCapturer is an abstract class that defines the interfaces for video
// capturing. The subclasses implement the video capturer for various types of
// capturers and various platforms.
//
// The captured frames may need to be adapted (for example, cropping).
// Video adaptation is built into and enabled by default. After a frame has
// been captured from the device, it is sent to the video adapter, then out to
// the sinks.
//
// Programming model:
//   Create an object of a subclass of VideoCapturer
//   Initialize
//   SignalStateChange.connect()
//   AddOrUpdateSink()
//   Find the capture format for Start() by either calling GetSupportedFormats()
//   and selecting one of the supported or calling GetBestCaptureFormat().
//   video_adapter()->OnOutputFormatRequest(desired_encoding_format)
//   Start()
//   GetCaptureFormat() optionally
//   Stop()
//
// Assumption:
//   The Start() and Stop() methods are called by a single thread (E.g., the
//   media engine thread). Hence, the VideoCapture subclasses dont need to be
//   thread safe.
//
class VideoCapturer : public sigslot::has_slots<>,
                      public rtc::VideoSourceInterface<cricket::VideoFrame> {
 public:
  VideoCapturer();

  virtual ~VideoCapturer() {}

  // Gets the id of the underlying device, which is available after the capturer
  // is initialized. Can be used to determine if two capturers reference the
  // same device.
  const std::string& GetId() const { return id_; }

  // Get the capture formats supported by the video capturer. The supported
  // formats are non empty after the device has been opened successfully.
  const std::vector<VideoFormat>* GetSupportedFormats() const;

  // Get the best capture format for the desired format. The best format is the
  // same as one of the supported formats except that the frame interval may be
  // different. If the application asks for 16x9 and the camera does not support
  // 16x9 HD or the application asks for 16x10, we find the closest 4x3 and then
  // crop; Otherwise, we find what the application asks for. Note that we assume
  // that for HD, the desired format is always 16x9. The subclasses can override
  // the default implementation.
  // Parameters
  //   desired: the input desired format. If desired.fourcc is not kAnyFourcc,
  //            the best capture format has the exactly same fourcc. Otherwise,
  //            the best capture format uses a fourcc in GetPreferredFourccs().
  //   best_format: the output of the best capture format.
  // Return false if there is no such a best format, that is, the desired format
  // is not supported.
  virtual bool GetBestCaptureFormat(const VideoFormat& desired,
                                    VideoFormat* best_format);

  // TODO(hellner): deprecate (make private) the Start API in favor of this one.
  //                Also remove CS_STARTING as it is implied by the return
  //                value of StartCapturing().
  bool StartCapturing(const VideoFormat& capture_format);
  // Start the video capturer with the specified capture format.
  // Parameter
  //   capture_format: The caller got this parameter by either calling
  //                   GetSupportedFormats() and selecting one of the supported
  //                   or calling GetBestCaptureFormat().
  // Return
  //   CS_STARTING:  The capturer is trying to start. Success or failure will
  //                 be notified via the |SignalStateChange| callback.
  //   CS_RUNNING:   if the capturer is started and capturing.
  //   CS_FAILED:    if the capturer failes to start..
  //   CS_NO_DEVICE: if the capturer has no device and fails to start.
  virtual CaptureState Start(const VideoFormat& capture_format) = 0;

  // Get the current capture format, which is set by the Start() call.
  // Note that the width and height of the captured frames may differ from the
  // capture format. For example, the capture format is HD but the captured
  // frames may be smaller than HD.
  const VideoFormat* GetCaptureFormat() const {
    return capture_format_.get();
  }

  // Stop the video capturer.
  virtual void Stop() = 0;
  // Check if the video capturer is running.
  virtual bool IsRunning() = 0;
  CaptureState capture_state() const {
    return capture_state_;
  }

  virtual bool apply_rotation() { return apply_rotation_; }

  // Returns true if the capturer is screencasting. This can be used to
  // implement screencast specific behavior.
  virtual bool IsScreencast() const = 0;

  // Caps the VideoCapturer's format according to max_format. It can e.g. be
  // used to prevent cameras from capturing at a resolution or framerate that
  // the capturer is capable of but not performing satisfactorily at.
  // The capping is an upper bound for each component of the capturing format.
  // The fourcc component is ignored.
  void ConstrainSupportedFormats(const VideoFormat& max_format);

  void set_enable_camera_list(bool enable_camera_list) {
    enable_camera_list_ = enable_camera_list;
  }
  bool enable_camera_list() {
    return enable_camera_list_;
  }

  // Signal all capture state changes that are not a direct result of calling
  // Start().
  sigslot::signal2<VideoCapturer*, CaptureState> SignalStateChange;
  // Frame callbacks are multithreaded to allow disconnect and connect to be
  // called concurrently. It also ensures that it is safe to call disconnect
  // at any time which is needed since the signal may be called from an
  // unmarshalled thread owned by the VideoCapturer.
  // Signal the captured frame to downstream.
  sigslot::signal2<VideoCapturer*, const CapturedFrame*,
                   sigslot::multi_threaded_local> SignalFrameCaptured;

  // If true, run video adaptation. By default, video adaptation is enabled
  // and users must call video_adapter()->OnOutputFormatRequest()
  // to receive frames.
  bool enable_video_adapter() const { return enable_video_adapter_; }
  void set_enable_video_adapter(bool enable_video_adapter) {
    enable_video_adapter_ = enable_video_adapter;
  }

  // Takes ownership.
  void set_frame_factory(VideoFrameFactory* frame_factory);

  bool GetInputSize(int* width, int* height);

  // Implements VideoSourceInterface
  void AddOrUpdateSink(rtc::VideoSinkInterface<cricket::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<cricket::VideoFrame>* sink) override;

 protected:
  // OnSinkWantsChanged can be overridden to change the default behavior
  // when a sink changes its VideoSinkWants by calling AddOrUpdateSink.
  virtual void OnSinkWantsChanged(const rtc::VideoSinkWants& wants);

  // Reports the appropriate frame size after adaptation. Returns true
  // if a frame is wanted. Returns false if there are no interested
  // sinks, or if the VideoAdapter decides to drop the frame.
  bool AdaptFrame(int width,
                  int height,
                  int64_t capture_time_ns,
                  int* out_width,
                  int* out_height,
                  int* crop_width,
                  int* crop_height,
                  int* crop_x,
                  int* crop_y);

  // Callback attached to SignalFrameCaptured where SignalVideoFrames is called.
  void OnFrameCaptured(VideoCapturer* video_capturer,
                       const CapturedFrame* captured_frame);

  // Called when a frame has been captured and converted to a
  // VideoFrame. OnFrame can be called directly by an implementation
  // that does not use SignalFrameCaptured or OnFrameCaptured. The
  // orig_width and orig_height are used only to produce stats.
  void OnFrame(const VideoFrame& frame, int orig_width, int orig_height);

  VideoAdapter* video_adapter() { return &video_adapter_; }

  void SetCaptureState(CaptureState state);

  // subclasses override this virtual method to provide a vector of fourccs, in
  // order of preference, that are expected by the media engine.
  virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) = 0;

  // mutators to set private attributes
  void SetId(const std::string& id) {
    id_ = id;
  }

  void SetCaptureFormat(const VideoFormat* format) {
    capture_format_.reset(format ? new VideoFormat(*format) : NULL);
  }

  void SetSupportedFormats(const std::vector<VideoFormat>& formats);
  VideoFrameFactory* frame_factory() { return frame_factory_.get(); }

 private:
  void Construct();
  // Get the distance between the desired format and the supported format.
  // Return the max distance if they mismatch. See the implementation for
  // details.
  int64_t GetFormatDistance(const VideoFormat& desired,
                            const VideoFormat& supported);

  // Convert captured frame to readable string for LOG messages.
  std::string ToString(const CapturedFrame* frame) const;

  // Updates filtered_supported_formats_ so that it contains the formats in
  // supported_formats_ that fulfill all applied restrictions.
  void UpdateFilteredSupportedFormats();
  // Returns true if format doesn't fulfill all applied restrictions.
  bool ShouldFilterFormat(const VideoFormat& format) const;

  void UpdateInputSize(int width, int height);

  rtc::ThreadChecker thread_checker_;
  std::string id_;
  CaptureState capture_state_;
  std::unique_ptr<VideoFrameFactory> frame_factory_;
  std::unique_ptr<VideoFormat> capture_format_;
  std::vector<VideoFormat> supported_formats_;
  std::unique_ptr<VideoFormat> max_format_;
  std::vector<VideoFormat> filtered_supported_formats_;

  bool enable_camera_list_;
  int scaled_width_;  // Current output size from ComputeScale.
  int scaled_height_;

  rtc::VideoBroadcaster broadcaster_;
  bool enable_video_adapter_;
  VideoAdapter video_adapter_;

  rtc::CriticalSection frame_stats_crit_;
  // The captured frame size before potential adapation.
  bool input_size_valid_ GUARDED_BY(frame_stats_crit_) = false;
  int input_width_ GUARDED_BY(frame_stats_crit_);
  int input_height_ GUARDED_BY(frame_stats_crit_);

  // Whether capturer should apply rotation to the frame before signaling it.
  bool apply_rotation_;

  RTC_DISALLOW_COPY_AND_ASSIGN(VideoCapturer);
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_BASE_VIDEOCAPTURER_H_
