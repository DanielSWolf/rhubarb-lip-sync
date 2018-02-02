/*
 * libjingle
 * Copyright 2015 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "talk/app/webrtc/objc/avfoundationvideocapturer.h"

#include "webrtc/base/bind.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/thread.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "RTCDispatcher+Private.h"
#import "RTCLogging.h"

// TODO(tkchin): support other formats.
static NSString *const kDefaultPreset = AVCaptureSessionPreset640x480;
static cricket::VideoFormat const kDefaultFormat =
    cricket::VideoFormat(640,
                         480,
                         cricket::VideoFormat::FpsToInterval(30),
                         cricket::FOURCC_NV12);

// This class used to capture frames using AVFoundation APIs on iOS. It is meant
// to be owned by an instance of AVFoundationVideoCapturer. The reason for this
// because other webrtc objects own cricket::VideoCapturer, which is not
// ref counted. To prevent bad behavior we do not expose this class directly.
@interface RTCAVFoundationVideoCapturerInternal : NSObject
    <AVCaptureVideoDataOutputSampleBufferDelegate>

@property(nonatomic, readonly) AVCaptureSession *captureSession;
@property(nonatomic, readonly) BOOL isRunning;
@property(nonatomic, readonly) BOOL canUseBackCamera;
@property(nonatomic, assign) BOOL useBackCamera;  // Defaults to NO.

// We keep a pointer back to AVFoundationVideoCapturer to make callbacks on it
// when we receive frames. This is safe because this object should be owned by
// it.
- (instancetype)initWithCapturer:(webrtc::AVFoundationVideoCapturer *)capturer;

// Starts and stops the capture session asynchronously. We cannot do this
// synchronously without blocking a WebRTC thread.
- (void)start;
- (void)stop;

@end

@implementation RTCAVFoundationVideoCapturerInternal {
  // Keep pointers to inputs for convenience.
  AVCaptureDeviceInput *_frontCameraInput;
  AVCaptureDeviceInput *_backCameraInput;
  AVCaptureVideoDataOutput *_videoDataOutput;
  // The cricket::VideoCapturer that owns this class. Should never be NULL.
  webrtc::AVFoundationVideoCapturer *_capturer;
  BOOL _orientationHasChanged;
}

@synthesize captureSession = _captureSession;
@synthesize isRunning = _isRunning;
@synthesize useBackCamera = _useBackCamera;

// This is called from the thread that creates the video source, which is likely
// the main thread.
- (instancetype)initWithCapturer:(webrtc::AVFoundationVideoCapturer *)capturer {
  RTC_DCHECK(capturer);
  if (self = [super init]) {
    _capturer = capturer;
    // Create the capture session and all relevant inputs and outputs. We need
    // to do this in init because the application may want the capture session
    // before we start the capturer for e.g. AVCapturePreviewLayer. All objects
    // created here are retained until dealloc and never recreated.
    if (![self setupCaptureSession]) {
      return nil;
    }
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(deviceOrientationDidChange:)
                   name:UIDeviceOrientationDidChangeNotification
                 object:nil];
    [center addObserverForName:AVCaptureSessionRuntimeErrorNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *notification) {
      RTCLogError(@"Capture session error: %@", notification.userInfo);
    }];
  }
  return self;
}

- (void)dealloc {
  RTC_DCHECK(!_isRunning);
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _capturer = nullptr;
}

- (AVCaptureSession *)captureSession {
  return _captureSession;
}

// Called from any thread (likely main thread).
- (BOOL)canUseBackCamera {
  return _backCameraInput != nil;
}

// Called from any thread (likely main thread).
- (BOOL)useBackCamera {
  @synchronized(self) {
    return _useBackCamera;
  }
}

// Called from any thread (likely main thread).
- (void)setUseBackCamera:(BOOL)useBackCamera {
  if (!self.canUseBackCamera) {
    if (useBackCamera) {
      RTCLogWarning(@"No rear-facing camera exists or it cannot be used;"
                    "not switching.");
    }
    return;
  }
  @synchronized(self) {
    if (_useBackCamera == useBackCamera) {
      return;
    }
    _useBackCamera = useBackCamera;
    [self updateSessionInputForUseBackCamera:useBackCamera];
  }
}

// Called from WebRTC thread.
- (void)start {
  if (_isRunning) {
    return;
  }
  _isRunning = YES;
  [RTCDispatcher dispatchAsyncOnType:RTCDispatcherTypeCaptureSession
                               block:^{
    _orientationHasChanged = NO;
    [self updateOrientation];
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    AVCaptureSession *captureSession = self.captureSession;
    [captureSession startRunning];
  }];
}

// Called from same thread as start.
- (void)stop {
  if (!_isRunning) {
    return;
  }
  _isRunning = NO;
  [RTCDispatcher dispatchAsyncOnType:RTCDispatcherTypeCaptureSession
                               block:^{
    [_videoDataOutput setSampleBufferDelegate:nil queue:nullptr];
    [_captureSession stopRunning];
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
  }];
}

#pragma mark AVCaptureVideoDataOutputSampleBufferDelegate

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
  NSParameterAssert(captureOutput == _videoDataOutput);
  if (!_isRunning) {
    return;
  }
  _capturer->CaptureSampleBuffer(sampleBuffer);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
         fromConnection:(AVCaptureConnection *)connection {
  RTCLogError(@"Dropped sample buffer.");
}

#pragma mark - Private

- (BOOL)setupCaptureSession {
  AVCaptureSession *captureSession = [[AVCaptureSession alloc] init];
#if defined(__IPHONE_7_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_7_0
  NSString *version = [[UIDevice currentDevice] systemVersion];
  if ([version integerValue] >= 7) {
    captureSession.usesApplicationAudioSession = NO;
  }
#endif
  if (![captureSession canSetSessionPreset:kDefaultPreset]) {
    RTCLogError(@"Session preset unsupported.");
    return NO;
  }
  captureSession.sessionPreset = kDefaultPreset;

  // Add the output.
  AVCaptureVideoDataOutput *videoDataOutput = [self videoDataOutput];
  if (![captureSession canAddOutput:videoDataOutput]) {
    RTCLogError(@"Video data output unsupported.");
    return NO;
  }
  [captureSession addOutput:videoDataOutput];

  // Get the front and back cameras. If there isn't a front camera
  // give up.
  AVCaptureDeviceInput *frontCameraInput = [self frontCameraInput];
  AVCaptureDeviceInput *backCameraInput = [self backCameraInput];
  if (!frontCameraInput) {
    RTCLogError(@"No front camera for capture session.");
    return NO;
  }

  // Add the inputs.
  if (![captureSession canAddInput:frontCameraInput] ||
      (backCameraInput && ![captureSession canAddInput:backCameraInput])) {
    RTCLogError(@"Session does not support capture inputs.");
    return NO;
  }
  AVCaptureDeviceInput *input = self.useBackCamera ?
      backCameraInput : frontCameraInput;
  [captureSession addInput:input];
  _captureSession = captureSession;
  return YES;
}

- (AVCaptureVideoDataOutput *)videoDataOutput {
  if (!_videoDataOutput) {
    // Make the capturer output NV12. Ideally we want I420 but that's not
    // currently supported on iPhone / iPad.
    AVCaptureVideoDataOutput *videoDataOutput =
        [[AVCaptureVideoDataOutput alloc] init];
    videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    videoDataOutput.videoSettings = @{
      (NSString *)kCVPixelBufferPixelFormatTypeKey :
        @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
    };
    videoDataOutput.alwaysDiscardsLateVideoFrames = NO;
    dispatch_queue_t queue =
        [RTCDispatcher dispatchQueueForType:RTCDispatcherTypeCaptureSession];
    [videoDataOutput setSampleBufferDelegate:self queue:queue];
    _videoDataOutput = videoDataOutput;
  }
  return _videoDataOutput;
}

- (AVCaptureDevice *)videoCaptureDeviceForPosition:
    (AVCaptureDevicePosition)position {
  for (AVCaptureDevice *captureDevice in
       [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) {
    if (captureDevice.position == position) {
      return captureDevice;
    }
  }
  return nil;
}

- (AVCaptureDeviceInput *)frontCameraInput {
  if (!_frontCameraInput) {
    AVCaptureDevice *frontCameraDevice =
        [self videoCaptureDeviceForPosition:AVCaptureDevicePositionFront];
    if (!frontCameraDevice) {
      RTCLogWarning(@"Failed to find front capture device.");
      return nil;
    }
    NSError *error = nil;
    AVCaptureDeviceInput *frontCameraInput =
        [AVCaptureDeviceInput deviceInputWithDevice:frontCameraDevice
                                              error:&error];
    if (!frontCameraInput) {
      RTCLogError(@"Failed to create front camera input: %@",
                  error.localizedDescription);
      return nil;
    }
    _frontCameraInput = frontCameraInput;
  }
  return _frontCameraInput;
}

- (AVCaptureDeviceInput *)backCameraInput {
  if (!_backCameraInput) {
    AVCaptureDevice *backCameraDevice =
        [self videoCaptureDeviceForPosition:AVCaptureDevicePositionBack];
    if (!backCameraDevice) {
      RTCLogWarning(@"Failed to find front capture device.");
      return nil;
    }
    NSError *error = nil;
    AVCaptureDeviceInput *backCameraInput =
        [AVCaptureDeviceInput deviceInputWithDevice:backCameraDevice
                                              error:&error];
    if (!backCameraInput) {
      RTCLogError(@"Failed to create front camera input: %@",
                  error.localizedDescription);
      return nil;
    }
    _backCameraInput = backCameraInput;
  }
  return _backCameraInput;
}

- (void)deviceOrientationDidChange:(NSNotification *)notification {
  [RTCDispatcher dispatchAsyncOnType:RTCDispatcherTypeCaptureSession
                               block:^{
    _orientationHasChanged = YES;
    [self updateOrientation];
  }];
}

// Called from capture session queue.
- (void)updateOrientation {
  AVCaptureConnection *connection =
      [_videoDataOutput connectionWithMediaType:AVMediaTypeVideo];
  if (!connection.supportsVideoOrientation) {
    // TODO(tkchin): set rotation bit on frames.
    return;
  }
  AVCaptureVideoOrientation orientation = AVCaptureVideoOrientationPortrait;
  switch ([UIDevice currentDevice].orientation) {
    case UIDeviceOrientationPortrait:
      orientation = AVCaptureVideoOrientationPortrait;
      break;
    case UIDeviceOrientationPortraitUpsideDown:
      orientation = AVCaptureVideoOrientationPortraitUpsideDown;
      break;
    case UIDeviceOrientationLandscapeLeft:
      orientation = AVCaptureVideoOrientationLandscapeRight;
      break;
    case UIDeviceOrientationLandscapeRight:
      orientation = AVCaptureVideoOrientationLandscapeLeft;
      break;
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
    case UIDeviceOrientationUnknown:
      if (!_orientationHasChanged) {
        connection.videoOrientation = orientation;
      }
      return;
  }
  connection.videoOrientation = orientation;
}

// Update the current session input to match what's stored in _useBackCamera.
- (void)updateSessionInputForUseBackCamera:(BOOL)useBackCamera {
  [RTCDispatcher dispatchAsyncOnType:RTCDispatcherTypeCaptureSession
                               block:^{
    [_captureSession beginConfiguration];
    AVCaptureDeviceInput *oldInput = _backCameraInput;
    AVCaptureDeviceInput *newInput = _frontCameraInput;
    if (useBackCamera) {
      oldInput = _frontCameraInput;
      newInput = _backCameraInput;
    }
    if (oldInput) {
      // Ok to remove this even if it's not attached. Will be no-op.
      [_captureSession removeInput:oldInput];
    }
    if (newInput) {
      [_captureSession addInput:newInput];
    }
    [self updateOrientation];
    [_captureSession commitConfiguration];
  }];
}

@end

namespace webrtc {

enum AVFoundationVideoCapturerMessageType : uint32_t {
  kMessageTypeFrame,
};

struct AVFoundationFrame {
  AVFoundationFrame(CVImageBufferRef buffer, int64_t time)
    : image_buffer(buffer), capture_time(time) {}
  CVImageBufferRef image_buffer;
  int64_t capture_time;
};

AVFoundationVideoCapturer::AVFoundationVideoCapturer()
    : _capturer(nil), _startThread(nullptr) {
  // Set our supported formats. This matches kDefaultPreset.
  std::vector<cricket::VideoFormat> supportedFormats;
  supportedFormats.push_back(cricket::VideoFormat(kDefaultFormat));
  SetSupportedFormats(supportedFormats);
  _capturer =
      [[RTCAVFoundationVideoCapturerInternal alloc] initWithCapturer:this];
}

AVFoundationVideoCapturer::~AVFoundationVideoCapturer() {
  _capturer = nil;
}

cricket::CaptureState AVFoundationVideoCapturer::Start(
    const cricket::VideoFormat& format) {
  if (!_capturer) {
    LOG(LS_ERROR) << "Failed to create AVFoundation capturer.";
    return cricket::CaptureState::CS_FAILED;
  }
  if (_capturer.isRunning) {
    LOG(LS_ERROR) << "The capturer is already running.";
    return cricket::CaptureState::CS_FAILED;
  }
  if (format != kDefaultFormat) {
    LOG(LS_ERROR) << "Unsupported format provided.";
    return cricket::CaptureState::CS_FAILED;
  }

  // Keep track of which thread capture started on. This is the thread that
  // frames need to be sent to.
  RTC_DCHECK(!_startThread);
  _startThread = rtc::Thread::Current();

  SetCaptureFormat(&format);
  // This isn't super accurate because it takes a while for the AVCaptureSession
  // to spin up, and this call returns async.
  // TODO(tkchin): make this better.
  [_capturer start];
  SetCaptureState(cricket::CaptureState::CS_RUNNING);

  return cricket::CaptureState::CS_STARTING;
}

void AVFoundationVideoCapturer::Stop() {
  [_capturer stop];
  SetCaptureFormat(NULL);
  _startThread = nullptr;
}

bool AVFoundationVideoCapturer::IsRunning() {
  return _capturer.isRunning;
}

AVCaptureSession* AVFoundationVideoCapturer::GetCaptureSession() {
  return _capturer.captureSession;
}

bool AVFoundationVideoCapturer::CanUseBackCamera() const {
  return _capturer.canUseBackCamera;
}

void AVFoundationVideoCapturer::SetUseBackCamera(bool useBackCamera) {
  _capturer.useBackCamera = useBackCamera;
}

bool AVFoundationVideoCapturer::GetUseBackCamera() const {
  return _capturer.useBackCamera;
}

void AVFoundationVideoCapturer::CaptureSampleBuffer(
    CMSampleBufferRef sampleBuffer) {
  if (CMSampleBufferGetNumSamples(sampleBuffer) != 1 ||
      !CMSampleBufferIsValid(sampleBuffer) ||
      !CMSampleBufferDataIsReady(sampleBuffer)) {
    return;
  }

  CVImageBufferRef image_buffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (image_buffer == NULL) {
    return;
  }

  // Retain the buffer and post it to the webrtc thread. It will be released
  // after it has successfully been signaled.
  CVBufferRetain(image_buffer);
  AVFoundationFrame frame(image_buffer, rtc::TimeNanos());
  _startThread->Post(RTC_FROM_HERE, this, kMessageTypeFrame,
                     new rtc::TypedMessageData<AVFoundationFrame>(frame));
}

void AVFoundationVideoCapturer::OnMessage(rtc::Message *msg) {
  switch (msg->message_id) {
    case kMessageTypeFrame: {
      rtc::TypedMessageData<AVFoundationFrame>* data =
        static_cast<rtc::TypedMessageData<AVFoundationFrame>*>(msg->pdata);
      const AVFoundationFrame& frame = data->data();
      OnFrameMessage(frame.image_buffer, frame.capture_time);
      delete data;
      break;
    }
  }
}

void AVFoundationVideoCapturer::OnFrameMessage(CVImageBufferRef image_buffer,
                                               int64_t capture_time) {
  RTC_DCHECK(_startThread->IsCurrent());

  // Base address must be unlocked to access frame data.
  CVOptionFlags lock_flags = kCVPixelBufferLock_ReadOnly;
  CVReturn ret = CVPixelBufferLockBaseAddress(image_buffer, lock_flags);
  if (ret != kCVReturnSuccess) {
    return;
  }

  static size_t const kYPlaneIndex = 0;
  static size_t const kUVPlaneIndex = 1;
  uint8_t* y_plane_address =
      static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(image_buffer,
                                                               kYPlaneIndex));
  size_t y_plane_height =
      CVPixelBufferGetHeightOfPlane(image_buffer, kYPlaneIndex);
  size_t y_plane_width =
      CVPixelBufferGetWidthOfPlane(image_buffer, kYPlaneIndex);
  size_t y_plane_bytes_per_row =
      CVPixelBufferGetBytesPerRowOfPlane(image_buffer, kYPlaneIndex);
  size_t uv_plane_height =
      CVPixelBufferGetHeightOfPlane(image_buffer, kUVPlaneIndex);
  size_t uv_plane_bytes_per_row =
      CVPixelBufferGetBytesPerRowOfPlane(image_buffer, kUVPlaneIndex);
  size_t frame_size = y_plane_bytes_per_row * y_plane_height +
      uv_plane_bytes_per_row * uv_plane_height;

  // Sanity check assumption that planar bytes are contiguous.
  uint8_t* uv_plane_address =
      static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(image_buffer,
                                                               kUVPlaneIndex));
  RTC_DCHECK(uv_plane_address ==
             y_plane_address + y_plane_height * y_plane_bytes_per_row);

  // Stuff data into a cricket::CapturedFrame.
  cricket::CapturedFrame frame;
  frame.width = y_plane_width;
  frame.height = y_plane_height;
  frame.pixel_width = 1;
  frame.pixel_height = 1;
  frame.fourcc = static_cast<uint32_t>(cricket::FOURCC_NV12);
  frame.time_stamp = capture_time;
  frame.data = y_plane_address;
  frame.data_size = frame_size;

  // This will call a superclass method that will perform the frame conversion
  // to I420.
  SignalFrameCaptured(this, &frame);

  CVPixelBufferUnlockBaseAddress(image_buffer, lock_flags);
  CVBufferRelease(image_buffer);
}

}  // namespace webrtc
