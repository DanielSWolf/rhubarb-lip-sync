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

#import "RTCPeerConnectionFactory+Internal.h"

#include <memory>
#include <vector>

#import "RTCAudioTrack+Internal.h"
#import "RTCICEServer+Internal.h"
#import "RTCMediaConstraints+Internal.h"
#import "RTCMediaSource+Internal.h"
#import "RTCMediaStream+Internal.h"
#import "RTCMediaStreamTrack+Internal.h"
#import "RTCPeerConnection+Internal.h"
#import "RTCPeerConnectionDelegate.h"
#import "RTCPeerConnectionInterface+Internal.h"
#import "RTCVideoCapturer+Internal.h"
#import "RTCVideoSource+Internal.h"
#import "RTCVideoTrack+Internal.h"

#include "webrtc/api/audiotrack.h"
#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/api/videotrack.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"

@implementation RTCPeerConnectionFactory {
  std::unique_ptr<rtc::Thread> _networkThread;
  std::unique_ptr<rtc::Thread> _workerThread;
  std::unique_ptr<rtc::Thread> _signalingThread;
}

@synthesize nativeFactory = _nativeFactory;

+ (void)initializeSSL {
  BOOL initialized = rtc::InitializeSSL();
  NSAssert(initialized, @"Failed to initialize SSL library");
}

+ (void)deinitializeSSL {
  BOOL deinitialized = rtc::CleanupSSL();
  NSAssert(deinitialized, @"Failed to deinitialize SSL library");
}

- (id)init {
  if ((self = [super init])) {
    _networkThread = rtc::Thread::CreateWithSocketServer();
    BOOL result = _networkThread->Start();
    NSAssert(result, @"Failed to start network thread.");

    _workerThread = rtc::Thread::Create();
    result = _workerThread->Start();
    NSAssert(result, @"Failed to start worker thread.");

    _signalingThread = rtc::Thread::Create();
    result = _signalingThread->Start();
    NSAssert(result, @"Failed to start signaling thread.");

    _nativeFactory = webrtc::CreatePeerConnectionFactory(
        _networkThread.get(), _workerThread.get(), _signalingThread.get(),
        nullptr, nullptr, nullptr);
    NSAssert(_nativeFactory, @"Failed to initialize PeerConnectionFactory!");
    // Uncomment to get sensitive logs emitted (to stderr or logcat).
    // rtc::LogMessage::LogToDebug(rtc::LS_SENSITIVE);
  }
  return self;
}

- (RTCPeerConnection *)peerConnectionWithConfiguration:(RTCConfiguration *)configuration
                                           constraints:(RTCMediaConstraints *)constraints
                                              delegate:(id<RTCPeerConnectionDelegate>)delegate {
  std::unique_ptr<webrtc::PeerConnectionInterface::RTCConfiguration> config(
      [configuration createNativeConfiguration]);
  if (!config) {
    return nil;
  }
  return [[RTCPeerConnection alloc] initWithFactory:self.nativeFactory.get()
                                             config:*config
                                        constraints:constraints.constraints
                                           delegate:delegate];
}

- (RTCPeerConnection*)
    peerConnectionWithICEServers:(NSArray*)servers
                     constraints:(RTCMediaConstraints*)constraints
                        delegate:(id<RTCPeerConnectionDelegate>)delegate {
  webrtc::PeerConnectionInterface::IceServers iceServers;
  for (RTCICEServer* server in servers) {
    iceServers.push_back(server.iceServer);
  }
  RTCPeerConnection* pc =
      [[RTCPeerConnection alloc] initWithFactory:self.nativeFactory.get()
                                      iceServers:iceServers
                                     constraints:constraints.constraints];
  pc.delegate = delegate;
  return pc;
}

- (RTCMediaStream*)mediaStreamWithLabel:(NSString*)label {
  rtc::scoped_refptr<webrtc::MediaStreamInterface> nativeMediaStream =
      self.nativeFactory->CreateLocalMediaStream([label UTF8String]);
  return [[RTCMediaStream alloc] initWithMediaStream:nativeMediaStream];
}

- (RTCVideoSource*)videoSourceWithCapturer:(RTCVideoCapturer*)capturer
                               constraints:(RTCMediaConstraints*)constraints {
  if (!capturer) {
    return nil;
  }
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source =
      self.nativeFactory->CreateVideoSource([capturer takeNativeCapturer], constraints.constraints);
  return [[RTCVideoSource alloc] initWithMediaSource:source];
}

- (RTCVideoTrack*)videoTrackWithID:(NSString*)videoId
                            source:(RTCVideoSource*)source {
  rtc::scoped_refptr<webrtc::VideoTrackInterface> track =
      self.nativeFactory->CreateVideoTrack([videoId UTF8String],
                                           source.videoSource);
  return [[RTCVideoTrack alloc] initWithMediaTrack:track];
}

- (RTCAudioTrack*)audioTrackWithID:(NSString*)audioId {
  rtc::scoped_refptr<webrtc::AudioTrackInterface> track =
      self.nativeFactory->CreateAudioTrack([audioId UTF8String], NULL);
  return [[RTCAudioTrack alloc] initWithMediaTrack:track];
}

@end
