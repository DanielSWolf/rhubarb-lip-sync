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

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCMediaStream+Internal.h"

#import "RTCAudioTrack+Internal.h"
#import "RTCMediaStreamTrack+Internal.h"
#import "RTCVideoTrack+Internal.h"

#include "webrtc/api/mediastreaminterface.h"

@implementation RTCMediaStream {
  NSMutableArray* _audioTracks;
  NSMutableArray* _videoTracks;
  rtc::scoped_refptr<webrtc::MediaStreamInterface> _mediaStream;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"[%@:A=%lu:V=%lu]",
                                    [self label],
                                    (unsigned long)[self.audioTracks count],
                                    (unsigned long)[self.videoTracks count]];
}

- (NSArray*)audioTracks {
  return [_audioTracks copy];
}

- (NSArray*)videoTracks {
  return [_videoTracks copy];
}

- (NSString*)label {
  return @(self.mediaStream->label().c_str());
}

- (BOOL)addAudioTrack:(RTCAudioTrack*)track {
  if (self.mediaStream->AddTrack(track.audioTrack)) {
    [_audioTracks addObject:track];
    return YES;
  }
  return NO;
}

- (BOOL)addVideoTrack:(RTCVideoTrack*)track {
  if (self.mediaStream->AddTrack(track.nativeVideoTrack)) {
    [_videoTracks addObject:track];
    return YES;
  }
  return NO;
}

- (BOOL)removeAudioTrack:(RTCAudioTrack*)track {
  NSUInteger index = [_audioTracks indexOfObjectIdenticalTo:track];
  NSAssert(index != NSNotFound,
           @"|removeAudioTrack| called on unexpected RTCAudioTrack");
  if (index != NSNotFound && self.mediaStream->RemoveTrack(track.audioTrack)) {
    [_audioTracks removeObjectAtIndex:index];
    return YES;
  }
  return NO;
}

- (BOOL)removeVideoTrack:(RTCVideoTrack*)track {
  NSUInteger index = [_videoTracks indexOfObjectIdenticalTo:track];
  NSAssert(index != NSNotFound,
           @"|removeAudioTrack| called on unexpected RTCVideoTrack");
  if (index != NSNotFound &&
      self.mediaStream->RemoveTrack(track.nativeVideoTrack)) {
    [_videoTracks removeObjectAtIndex:index];
    return YES;
  }
  return NO;
}

@end

@implementation RTCMediaStream (Internal)

- (id)initWithMediaStream:
          (rtc::scoped_refptr<webrtc::MediaStreamInterface>)mediaStream {
  if (!mediaStream) {
    NSAssert(NO, @"nil arguments not allowed");
    self = nil;
    return nil;
  }
  if ((self = [super init])) {
    webrtc::AudioTrackVector audio_tracks = mediaStream->GetAudioTracks();
    webrtc::VideoTrackVector video_tracks = mediaStream->GetVideoTracks();

    _audioTracks = [NSMutableArray arrayWithCapacity:audio_tracks.size()];
    _videoTracks = [NSMutableArray arrayWithCapacity:video_tracks.size()];
    _mediaStream = mediaStream;

    for (size_t i = 0; i < audio_tracks.size(); ++i) {
      rtc::scoped_refptr<webrtc::AudioTrackInterface> track =
          audio_tracks[i];
      RTCAudioTrack* audioTrack =
          [[RTCAudioTrack alloc] initWithMediaTrack:track];
      [_audioTracks addObject:audioTrack];
    }

    for (size_t i = 0; i < video_tracks.size(); ++i) {
      rtc::scoped_refptr<webrtc::VideoTrackInterface> track =
          video_tracks[i];
      RTCVideoTrack* videoTrack =
          [[RTCVideoTrack alloc] initWithMediaTrack:track];
      [_videoTracks addObject:videoTrack];
    }
  }
  return self;
}

- (rtc::scoped_refptr<webrtc::MediaStreamInterface>)mediaStream {
  return _mediaStream;
}

@end
