/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "RTCVideoSource+Private.h"

@implementation RTCVideoSource {
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _nativeVideoSource;
}

- (RTCSourceState)state {
  return [[self class] sourceStateForNativeState:_nativeVideoSource->state()];
}

- (NSString *)description {
  return [NSString stringWithFormat:@"RTCVideoSource:\n%@",
                                    [[self class] stringForState:self.state]];
}

#pragma mark - Private

- (rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>)nativeVideoSource {
  return _nativeVideoSource;
}

- (instancetype)initWithNativeVideoSource:
    (rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>)nativeVideoSource {
  NSParameterAssert(nativeVideoSource);
  if (self = [super init]) {
    _nativeVideoSource = nativeVideoSource;
  }
  return self;
}

+ (webrtc::MediaSourceInterface::SourceState)nativeSourceStateForState:
    (RTCSourceState)state {
  switch (state) {
    case RTCSourceStateInitializing:
      return webrtc::MediaSourceInterface::kInitializing;
    case RTCSourceStateLive:
      return webrtc::MediaSourceInterface::kLive;
    case RTCSourceStateEnded:
      return webrtc::MediaSourceInterface::kEnded;
    case RTCSourceStateMuted:
      return webrtc::MediaSourceInterface::kMuted;
  }
}

+ (RTCSourceState)sourceStateForNativeState:
    (webrtc::MediaSourceInterface::SourceState)nativeState {
  switch (nativeState) {
    case webrtc::MediaSourceInterface::kInitializing:
      return RTCSourceStateInitializing;
    case webrtc::MediaSourceInterface::kLive:
      return RTCSourceStateLive;
    case webrtc::MediaSourceInterface::kEnded:
      return RTCSourceStateEnded;
    case webrtc::MediaSourceInterface::kMuted:
      return RTCSourceStateMuted;
  }
}

+ (NSString *)stringForState:(RTCSourceState)state {
  switch (state) {
    case RTCSourceStateInitializing:
      return @"Initializing";
    case RTCSourceStateLive:
      return @"Live";
    case RTCSourceStateEnded:
      return @"Ended";
    case RTCSourceStateMuted:
      return @"Muted";
  }
}

@end
