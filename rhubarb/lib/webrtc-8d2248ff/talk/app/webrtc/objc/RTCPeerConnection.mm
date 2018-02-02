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

#import "RTCPeerConnection+Internal.h"

#import "RTCDataChannel+Internal.h"
#import "RTCEnumConverter.h"
#import "RTCICECandidate+Internal.h"
#import "RTCICEServer+Internal.h"
#import "RTCMediaConstraints+Internal.h"
#import "RTCMediaStream+Internal.h"
#import "RTCMediaStreamTrack+Internal.h"
#import "RTCPeerConnectionObserver.h"
#import "RTCSessionDescription+Internal.h"
#import "RTCSessionDescription.h"
#import "RTCSessionDescriptionDelegate.h"
#import "RTCStatsDelegate.h"
#import "RTCStatsReport+Internal.h"

#include <memory>

#include "webrtc/api/jsep.h"

NSString* const kRTCSessionDescriptionDelegateErrorDomain = @"RTCSDPError";
int const kRTCSessionDescriptionDelegateErrorCode = -1;

namespace webrtc {

class RTCCreateSessionDescriptionObserver
    : public CreateSessionDescriptionObserver {
 public:
  RTCCreateSessionDescriptionObserver(
      id<RTCSessionDescriptionDelegate> delegate,
      RTCPeerConnection* peerConnection) {
    _delegate = delegate;
    _peerConnection = peerConnection;
  }

  void OnSuccess(SessionDescriptionInterface* desc) override {
    RTCSessionDescription* session =
        [[RTCSessionDescription alloc] initWithSessionDescription:desc];
    [_delegate peerConnection:_peerConnection
        didCreateSessionDescription:session
                              error:nil];
    delete desc;
  }

  void OnFailure(const std::string& error) override {
    NSString* str = @(error.c_str());
    NSError* err =
        [NSError errorWithDomain:kRTCSessionDescriptionDelegateErrorDomain
                            code:kRTCSessionDescriptionDelegateErrorCode
                        userInfo:@{@"error" : str}];
    [_delegate peerConnection:_peerConnection
        didCreateSessionDescription:nil
                              error:err];
  }

 private:
  id<RTCSessionDescriptionDelegate> _delegate;
  RTCPeerConnection* _peerConnection;
};

class RTCSetSessionDescriptionObserver : public SetSessionDescriptionObserver {
 public:
  RTCSetSessionDescriptionObserver(id<RTCSessionDescriptionDelegate> delegate,
                                   RTCPeerConnection* peerConnection) {
    _delegate = delegate;
    _peerConnection = peerConnection;
  }

  void OnSuccess() override {
    [_delegate peerConnection:_peerConnection
        didSetSessionDescriptionWithError:nil];
  }

  void OnFailure(const std::string& error) override {
    NSString* str = @(error.c_str());
    NSError* err =
        [NSError errorWithDomain:kRTCSessionDescriptionDelegateErrorDomain
                            code:kRTCSessionDescriptionDelegateErrorCode
                        userInfo:@{@"error" : str}];
    [_delegate peerConnection:_peerConnection
        didSetSessionDescriptionWithError:err];
  }

 private:
  id<RTCSessionDescriptionDelegate> _delegate;
  RTCPeerConnection* _peerConnection;
};

class RTCStatsObserver : public StatsObserver {
 public:
  RTCStatsObserver(id<RTCStatsDelegate> delegate,
                   RTCPeerConnection* peerConnection) {
    _delegate = delegate;
    _peerConnection = peerConnection;
  }

  void OnComplete(const StatsReports& reports) override {
    NSMutableArray* stats = [NSMutableArray arrayWithCapacity:reports.size()];
    for (const auto* report : reports) {
      RTCStatsReport* statsReport =
          [[RTCStatsReport alloc] initWithStatsReport:*report];
      [stats addObject:statsReport];
    }
    [_delegate peerConnection:_peerConnection didGetStats:stats];
  }

 private:
  id<RTCStatsDelegate> _delegate;
  RTCPeerConnection* _peerConnection;
};
}

@implementation RTCPeerConnection {
  NSMutableArray* _localStreams;
  std::unique_ptr<webrtc::RTCPeerConnectionObserver> _observer;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> _peerConnection;
}

- (BOOL)addICECandidate:(RTCICECandidate*)candidate {
  std::unique_ptr<const webrtc::IceCandidateInterface> iceCandidate(
      candidate.candidate);
  return self.peerConnection->AddIceCandidate(iceCandidate.get());
}

- (BOOL)addStream:(RTCMediaStream*)stream {
  BOOL ret = self.peerConnection->AddStream(stream.mediaStream);
  if (!ret) {
    return NO;
  }
  [_localStreams addObject:stream];
  return YES;
}

- (RTCDataChannel*)createDataChannelWithLabel:(NSString*)label
                                       config:(RTCDataChannelInit*)config {
  std::string labelString([label UTF8String]);
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel =
      self.peerConnection->CreateDataChannel(labelString,
                                             config.dataChannelInit);
  return [[RTCDataChannel alloc] initWithDataChannel:dataChannel];
}

- (void)createAnswerWithDelegate:(id<RTCSessionDescriptionDelegate>)delegate
                     constraints:(RTCMediaConstraints*)constraints {
  rtc::scoped_refptr<webrtc::RTCCreateSessionDescriptionObserver>
      observer(new rtc::RefCountedObject<
          webrtc::RTCCreateSessionDescriptionObserver>(delegate, self));
  self.peerConnection->CreateAnswer(observer, constraints.constraints);
}

- (void)createOfferWithDelegate:(id<RTCSessionDescriptionDelegate>)delegate
                    constraints:(RTCMediaConstraints*)constraints {
  rtc::scoped_refptr<webrtc::RTCCreateSessionDescriptionObserver>
      observer(new rtc::RefCountedObject<
          webrtc::RTCCreateSessionDescriptionObserver>(delegate, self));
  self.peerConnection->CreateOffer(observer, constraints.constraints);
}

- (void)removeStream:(RTCMediaStream*)stream {
  self.peerConnection->RemoveStream(stream.mediaStream);
  [_localStreams removeObject:stream];
}

- (void)setLocalDescriptionWithDelegate:
            (id<RTCSessionDescriptionDelegate>)delegate
                     sessionDescription:(RTCSessionDescription*)sdp {
  rtc::scoped_refptr<webrtc::RTCSetSessionDescriptionObserver> observer(
      new rtc::RefCountedObject<webrtc::RTCSetSessionDescriptionObserver>(
          delegate, self));
  self.peerConnection->SetLocalDescription(observer, sdp.sessionDescription);
}

- (void)setRemoteDescriptionWithDelegate:
            (id<RTCSessionDescriptionDelegate>)delegate
                      sessionDescription:(RTCSessionDescription*)sdp {
  rtc::scoped_refptr<webrtc::RTCSetSessionDescriptionObserver> observer(
      new rtc::RefCountedObject<webrtc::RTCSetSessionDescriptionObserver>(
          delegate, self));
  self.peerConnection->SetRemoteDescription(observer, sdp.sessionDescription);
}

- (BOOL)setConfiguration:(RTCConfiguration *)configuration {
  std::unique_ptr<webrtc::PeerConnectionInterface::RTCConfiguration> config(
      [configuration createNativeConfiguration]);
  if (!config) {
    return NO;
  }
  return self.peerConnection->SetConfiguration(*config);
}

- (RTCSessionDescription*)localDescription {
  const webrtc::SessionDescriptionInterface* sdi =
      self.peerConnection->local_description();
  return sdi ? [[RTCSessionDescription alloc] initWithSessionDescription:sdi]
             : nil;
}

- (NSArray*)localStreams {
  return [_localStreams copy];
}

- (RTCSessionDescription*)remoteDescription {
  const webrtc::SessionDescriptionInterface* sdi =
      self.peerConnection->remote_description();
  return sdi ? [[RTCSessionDescription alloc] initWithSessionDescription:sdi]
             : nil;
}

- (RTCICEConnectionState)iceConnectionState {
  return [RTCEnumConverter
      convertIceConnectionStateToObjC:self.peerConnection
                                          ->ice_connection_state()];
}

- (RTCICEGatheringState)iceGatheringState {
  return [RTCEnumConverter
      convertIceGatheringStateToObjC:self.peerConnection
                                         ->ice_gathering_state()];
}

- (RTCSignalingState)signalingState {
  return [RTCEnumConverter
      convertSignalingStateToObjC:self.peerConnection->signaling_state()];
}

- (void)close {
  self.peerConnection->Close();
}

- (BOOL)getStatsWithDelegate:(id<RTCStatsDelegate>)delegate
            mediaStreamTrack:(RTCMediaStreamTrack*)mediaStreamTrack
            statsOutputLevel:(RTCStatsOutputLevel)statsOutputLevel {
  rtc::scoped_refptr<webrtc::RTCStatsObserver> observer(
      new rtc::RefCountedObject<webrtc::RTCStatsObserver>(delegate,
                                                                self));
  webrtc::PeerConnectionInterface::StatsOutputLevel nativeOutputLevel =
      [RTCEnumConverter convertStatsOutputLevelToNative:statsOutputLevel];
  return self.peerConnection->GetStats(
      observer, mediaStreamTrack.mediaTrack, nativeOutputLevel);
}

@end

@implementation RTCPeerConnection (Internal)

- (instancetype)initWithFactory:(webrtc::PeerConnectionFactoryInterface*)factory
     iceServers:(const webrtc::PeerConnectionInterface::IceServers&)iceServers
    constraints:(const webrtc::MediaConstraintsInterface*)constraints {
  NSParameterAssert(factory != nullptr);
  if (self = [super init]) {
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.servers = iceServers;
    _observer.reset(new webrtc::RTCPeerConnectionObserver(self));
    _peerConnection = factory->CreatePeerConnection(
        config, constraints, nullptr, nullptr, _observer.get());
    _localStreams = [[NSMutableArray alloc] init];
  }
  return self;
}

- (instancetype)initWithFactory:(webrtc::PeerConnectionFactoryInterface *)factory
                         config:(const webrtc::PeerConnectionInterface::RTCConfiguration &)config
                    constraints:(const webrtc::MediaConstraintsInterface *)constraints
                       delegate:(id<RTCPeerConnectionDelegate>)delegate {
  NSParameterAssert(factory);
  if (self = [super init]) {
    _observer.reset(new webrtc::RTCPeerConnectionObserver(self));
    _peerConnection =
        factory->CreatePeerConnection(config, constraints, nullptr, nullptr, _observer.get());
    _localStreams = [[NSMutableArray alloc] init];
    _delegate = delegate;
  }
  return self;
}

- (rtc::scoped_refptr<webrtc::PeerConnectionInterface>)peerConnection {
  return _peerConnection;
}

@end
