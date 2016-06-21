/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Implementation of CarbonVideoRenderer

#include "webrtc/media/devices/carbonvideorenderer.h"

#include "webrtc/base/logging.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/media/base/videoframe.h"

namespace cricket {

CarbonVideoRenderer::CarbonVideoRenderer(int x, int y)
    : image_width_(0),
      image_height_(0),
      x_(x),
      y_(y),
      window_ref_(NULL) {
}

CarbonVideoRenderer::~CarbonVideoRenderer() {
  if (window_ref_) {
    DisposeWindow(window_ref_);
  }
}

// Called from the main event loop. All renderering needs to happen on
// the main thread.
OSStatus CarbonVideoRenderer::DrawEventHandler(EventHandlerCallRef handler,
                                               EventRef event,
                                               void* data) {
  OSStatus status = noErr;
  CarbonVideoRenderer* renderer = static_cast<CarbonVideoRenderer*>(data);
  if (renderer != NULL) {
    if (!renderer->DrawFrame()) {
      LOG(LS_ERROR) << "Failed to draw frame.";
    }
  }
  return status;
}

bool CarbonVideoRenderer::DrawFrame() {
  // Grab the image lock to make sure it is not changed why we'll draw it.
  rtc::CritScope cs(&image_crit_);

  if (image_.get() == NULL) {
    // Nothing to draw, just return.
    return true;
  }
  int width = image_width_;
  int height = image_height_;
  CGDataProviderRef provider =
      CGDataProviderCreateWithData(NULL, image_.get(), width * height * 4,
                                   NULL);
  CGColorSpaceRef color_space_ref = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmap_info = kCGBitmapByteOrderDefault;
  CGColorRenderingIntent rendering_intent = kCGRenderingIntentDefault;
  CGImageRef image_ref = CGImageCreate(width, height, 8, 32, width * 4,
                                       color_space_ref, bitmap_info, provider,
                                       NULL, false, rendering_intent);
  CGDataProviderRelease(provider);

  if (image_ref == NULL) {
    return false;
  }
  CGContextRef context;
  SetPortWindowPort(window_ref_);
  if (QDBeginCGContext(GetWindowPort(window_ref_), &context) != noErr) {
    CGImageRelease(image_ref);
    return false;
  }
  Rect window_bounds;
  GetWindowPortBounds(window_ref_, &window_bounds);

  // Anchor the image to the top left corner.
  int x = 0;
  int y = window_bounds.bottom - CGImageGetHeight(image_ref);
  CGRect dst_rect = CGRectMake(x, y, CGImageGetWidth(image_ref),
                               CGImageGetHeight(image_ref));
  CGContextDrawImage(context, dst_rect, image_ref);
  CGContextFlush(context);
  QDEndCGContext(GetWindowPort(window_ref_), &context);
  CGImageRelease(image_ref);
  return true;
}

bool CarbonVideoRenderer::SetSize(int width, int height) {
  if (width != image_width_ || height != image_height_) {
    // Grab the image lock while changing its size.
    rtc::CritScope cs(&image_crit_);
    image_width_ = width;
    image_height_ = height;
    image_.reset(new uint8_t[width * height * 4]);
    memset(image_.get(), 255, width * height * 4);
  }
  return true;
}

void CarbonVideoRenderer::OnFrame(const VideoFrame& video_frame) {
  {
    const VideoFrame* frame = video_frame->GetCopyWithRotationApplied();

    if (!SetSize(frame->width(), frame->height())) {
      return false;
    }

    // Grab the image lock so we are not trashing up the image being drawn.
    rtc::CritScope cs(&image_crit_);
    frame->ConvertToRgbBuffer(cricket::FOURCC_ABGR,
                              image_.get(),
                              static_cast<size_t>(frame->width()) *
                                frame->height() * 4,
                              frame->width() * 4);
  }

  // Trigger a repaint event for the whole window.
  Rect bounds;
  InvalWindowRect(window_ref_, GetWindowPortBounds(window_ref_, &bounds));
  return true;
}

bool CarbonVideoRenderer::Initialize() {
  OSStatus err;
  WindowAttributes attributes =
      kWindowStandardDocumentAttributes |
      kWindowLiveResizeAttribute |
      kWindowFrameworkScaledAttribute |
      kWindowStandardHandlerAttribute;

  struct Rect bounds;
  bounds.top = y_;
  bounds.bottom = 480;
  bounds.left = x_;
  bounds.right = 640;
  err = CreateNewWindow(kDocumentWindowClass, attributes,
                        &bounds, &window_ref_);
  if (!window_ref_ || err != noErr) {
    LOG(LS_ERROR) << "CreateNewWindow failed, error code: " << err;
    return false;
  }
  static const EventTypeSpec event_spec = {
    kEventClassWindow,
    kEventWindowDrawContent
  };

  err = InstallWindowEventHandler(
      window_ref_,
      NewEventHandlerUPP(CarbonVideoRenderer::DrawEventHandler),
      GetEventTypeCount(event_spec),
      &event_spec,
      this,
      NULL);
  if (err != noErr) {
    LOG(LS_ERROR) << "Failed to install event handler, error code: " << err;
    return false;
  }
  SelectWindow(window_ref_);
  ShowWindow(window_ref_);
  return true;
}

}  // namespace cricket
