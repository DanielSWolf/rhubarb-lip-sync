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

#import "RTCAVFoundationVideoSource+Internal.h"

#import "RTCMediaConstraints+Internal.h"
#import "RTCMediaSource+Internal.h"
#import "RTCPeerConnectionFactory+Internal.h"
#import "RTCVideoSource+Internal.h"

#include <memory>

@implementation RTCAVFoundationVideoSource {
  webrtc::AVFoundationVideoCapturer *_capturer;
}

- (instancetype)initWithFactory:(RTCPeerConnectionFactory*)factory
                    constraints:(RTCMediaConstraints*)constraints {
  NSParameterAssert(factory);
  // We pass ownership of the capturer to the source, but since we own
  // the source, it should be ok to keep a raw pointer to the
  // capturer.
  _capturer = new webrtc::AVFoundationVideoCapturer();
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source =
      factory.nativeFactory->CreateVideoSource(_capturer,
                                               constraints.constraints);
  return [super initWithMediaSource:source];
}

- (BOOL)canUseBackCamera {
  return self.capturer->CanUseBackCamera();
}

- (BOOL)useBackCamera {
  return self.capturer->GetUseBackCamera();
}

- (void)setUseBackCamera:(BOOL)useBackCamera {
  self.capturer->SetUseBackCamera(useBackCamera);
}

- (AVCaptureSession*)captureSession {
  return self.capturer->GetCaptureSession();
}

- (webrtc::AVFoundationVideoCapturer*)capturer {
  return _capturer;
}

@end
