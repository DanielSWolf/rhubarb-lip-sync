/*
 * libjingle
 * Copyright 2013 Google Inc.
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

#import "RTCI420Frame.h"

#include <memory>

#include "webrtc/media/base/videoframe.h"

@implementation RTCI420Frame {
  std::unique_ptr<cricket::VideoFrame> _videoFrame;
}

- (NSUInteger)width {
  return _videoFrame->width();
}

- (NSUInteger)height {
  return _videoFrame->height();
}

// TODO(nisse): chromaWidth and chromaHeight are used only in
// RTCOpenGLVideoRenderer.mm. Update, and then delete these
// properties.
- (NSUInteger)chromaWidth {
  return (self.width + 1) / 2;
}

- (NSUInteger)chromaHeight {
  return (self.height + 1) / 2;
}

- (const uint8_t*)yPlane {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->DataY() : nullptr;
}

- (const uint8_t*)uPlane {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->DataU() : nullptr;
}

- (const uint8_t*)vPlane {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->DataV() : nullptr;
}

- (NSInteger)yPitch {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->StrideY() : 0;
}

- (NSInteger)uPitch {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->StrideU() : 0;
}

- (NSInteger)vPitch {
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer =
      _videoFrame->video_frame_buffer();
  return buffer ? buffer->StrideV() : 0;
}

@end

@implementation RTCI420Frame (Internal)

- (instancetype)initWithVideoFrame:(cricket::VideoFrame*)videoFrame {
  if (self = [super init]) {
    // Keep a shallow copy of the video frame. The underlying frame buffer is
    // not copied.
    _videoFrame.reset(videoFrame->Copy());
  }
  return self;
}

@end
