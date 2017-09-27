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

#import <Foundation/Foundation.h>

#import "RTCICEServer.h"
#import "RTCMediaConstraints.h"
#import "RTCMediaStream.h"
#import "RTCPair.h"
#import "RTCPeerConnection.h"
#import "RTCPeerConnectionFactory.h"
#import "RTCPeerConnectionSyncObserver.h"
#import "RTCSessionDescription.h"
#import "RTCSessionDescriptionSyncObserver.h"
#import "RTCVideoRenderer.h"
#import "RTCVideoTrack.h"

#include "webrtc/base/gunit.h"
#include "webrtc/base/ssladapter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const NSTimeInterval kRTCPeerConnectionTestTimeout = 20;

@interface RTCFakeRenderer : NSObject <RTCVideoRenderer>
@end

@implementation RTCFakeRenderer

- (void)setSize:(CGSize)size {}
- (void)renderFrame:(RTCI420Frame*)frame {}

@end

@interface RTCPeerConnectionTest : NSObject

// Returns whether the two sessions are of the same type.
+ (BOOL)isSession:(RTCSessionDescription*)session1
    ofSameTypeAsSession:(RTCSessionDescription*)session2;

// Create and add tracks to pc, with the given source, label, and IDs
- (RTCMediaStream*)addTracksToPeerConnection:(RTCPeerConnection*)pc
                                 withFactory:(RTCPeerConnectionFactory*)factory
                                 videoSource:(RTCVideoSource*)videoSource
                                 streamLabel:(NSString*)streamLabel
                                videoTrackID:(NSString*)videoTrackID
                                audioTrackID:(NSString*)audioTrackID;

- (void)testCompleteSessionWithFactory:(RTCPeerConnectionFactory*)factory;

@end

@implementation RTCPeerConnectionTest

+ (BOOL)isSession:(RTCSessionDescription*)session1
    ofSameTypeAsSession:(RTCSessionDescription*)session2 {
  return [session1.type isEqual:session2.type];
}

- (RTCMediaStream*)addTracksToPeerConnection:(RTCPeerConnection*)pc
                                 withFactory:(RTCPeerConnectionFactory*)factory
                                 videoSource:(RTCVideoSource*)videoSource
                                 streamLabel:(NSString*)streamLabel
                                videoTrackID:(NSString*)videoTrackID
                                audioTrackID:(NSString*)audioTrackID {
  RTCMediaStream* localMediaStream = [factory mediaStreamWithLabel:streamLabel];
  // TODO(zeke): Fix this test to create a fake video capturer so that a track
  // can be created.
  if (videoSource) {
    RTCVideoTrack* videoTrack =
        [factory videoTrackWithID:videoTrackID source:videoSource];
    RTCFakeRenderer* videoRenderer = [[RTCFakeRenderer alloc] init];
    [videoTrack addRenderer:videoRenderer];
    [localMediaStream addVideoTrack:videoTrack];
    // Test that removal/re-add works.
    [localMediaStream removeVideoTrack:videoTrack];
    [localMediaStream addVideoTrack:videoTrack];
  }
  RTCAudioTrack* audioTrack = [factory audioTrackWithID:audioTrackID];
  [localMediaStream addAudioTrack:audioTrack];
  [pc addStream:localMediaStream];
  return localMediaStream;
}

- (void)testCompleteSessionWithFactory:(RTCPeerConnectionFactory*)factory {
  NSArray* mandatory = @[
    [[RTCPair alloc] initWithKey:@"DtlsSrtpKeyAgreement" value:@"true"],
    [[RTCPair alloc] initWithKey:@"internalSctpDataChannels" value:@"true"],
  ];
  RTCMediaConstraints* constraints = [[RTCMediaConstraints alloc] init];
  RTCMediaConstraints* pcConstraints =
      [[RTCMediaConstraints alloc] initWithMandatoryConstraints:mandatory
                                            optionalConstraints:nil];

  RTCPeerConnectionSyncObserver* offeringExpectations =
      [[RTCPeerConnectionSyncObserver alloc] init];
  RTCPeerConnection* pcOffer =
      [factory peerConnectionWithICEServers:nil
                                constraints:pcConstraints
                                   delegate:offeringExpectations];

  RTCPeerConnectionSyncObserver* answeringExpectations =
      [[RTCPeerConnectionSyncObserver alloc] init];

  RTCPeerConnection* pcAnswer =
      [factory peerConnectionWithICEServers:nil
                                constraints:pcConstraints
                                   delegate:answeringExpectations];
  // TODO(hughv): Create video capturer
  RTCVideoCapturer* capturer = nil;
  RTCVideoSource* videoSource =
      [factory videoSourceWithCapturer:capturer constraints:constraints];

  // Here and below, "oLMS" refers to offerer's local media stream, and "aLMS"
  // refers to the answerer's local media stream, with suffixes of "a0" and "v0"
  // for audio and video tracks, resp.  These mirror chrome historical naming.
  RTCMediaStream* oLMSUnused = [self addTracksToPeerConnection:pcOffer
                                                   withFactory:factory
                                                   videoSource:videoSource
                                                   streamLabel:@"oLMS"
                                                  videoTrackID:@"oLMSv0"
                                                  audioTrackID:@"oLMSa0"];

  RTCDataChannel* offerDC =
      [pcOffer createDataChannelWithLabel:@"offerDC"
                                   config:[[RTCDataChannelInit alloc] init]];
  EXPECT_TRUE([offerDC.label isEqual:@"offerDC"]);
  offerDC.delegate = offeringExpectations;
  offeringExpectations.dataChannel = offerDC;

  RTCSessionDescriptionSyncObserver* sdpObserver =
      [[RTCSessionDescriptionSyncObserver alloc] init];
  [pcOffer createOfferWithDelegate:sdpObserver constraints:constraints];
  [sdpObserver wait];
  EXPECT_TRUE(sdpObserver.success);
  RTCSessionDescription* offerSDP = sdpObserver.sessionDescription;
  EXPECT_EQ([@"offer" compare:offerSDP.type options:NSCaseInsensitiveSearch],
            NSOrderedSame);
  EXPECT_GT([offerSDP.description length], 0);

  sdpObserver = [[RTCSessionDescriptionSyncObserver alloc] init];
  [answeringExpectations expectSignalingChange:RTCSignalingHaveRemoteOffer];
  [answeringExpectations expectAddStream:@"oLMS"];
  [pcAnswer setRemoteDescriptionWithDelegate:sdpObserver
                          sessionDescription:offerSDP];
  [sdpObserver wait];

  RTCMediaStream* aLMSUnused = [self addTracksToPeerConnection:pcAnswer
                                                   withFactory:factory
                                                   videoSource:videoSource
                                                   streamLabel:@"aLMS"
                                                  videoTrackID:@"aLMSv0"
                                                  audioTrackID:@"aLMSa0"];

  sdpObserver = [[RTCSessionDescriptionSyncObserver alloc] init];
  [pcAnswer createAnswerWithDelegate:sdpObserver constraints:constraints];
  [sdpObserver wait];
  EXPECT_TRUE(sdpObserver.success);
  RTCSessionDescription* answerSDP = sdpObserver.sessionDescription;
  EXPECT_EQ([@"answer" compare:answerSDP.type options:NSCaseInsensitiveSearch],
            NSOrderedSame);
  EXPECT_GT([answerSDP.description length], 0);

  [offeringExpectations expectICECandidates:2];
  // It's possible to only have 1 ICE candidate for the answerer, since we use
  // BUNDLE and rtcp-mux by default, and don't provide any ICE servers in this
  // test.
  [answeringExpectations expectICECandidates:1];

  sdpObserver = [[RTCSessionDescriptionSyncObserver alloc] init];
  [answeringExpectations expectSignalingChange:RTCSignalingStable];
  [pcAnswer setLocalDescriptionWithDelegate:sdpObserver
                         sessionDescription:answerSDP];
  [sdpObserver wait];
  EXPECT_TRUE(sdpObserver.sessionDescription == NULL);

  sdpObserver = [[RTCSessionDescriptionSyncObserver alloc] init];
  [offeringExpectations expectSignalingChange:RTCSignalingHaveLocalOffer];
  [pcOffer setLocalDescriptionWithDelegate:sdpObserver
                        sessionDescription:offerSDP];
  [sdpObserver wait];
  EXPECT_TRUE(sdpObserver.sessionDescription == NULL);

  [offeringExpectations expectICEConnectionChange:RTCICEConnectionChecking];
  [offeringExpectations expectICEConnectionChange:RTCICEConnectionConnected];
  // TODO(fischman): figure out why this is flaky and re-introduce (and remove
  // special-casing from the observer!).
  // [offeringExpectations expectICEConnectionChange:RTCICEConnectionCompleted];
  [answeringExpectations expectICEConnectionChange:RTCICEConnectionChecking];
  [answeringExpectations expectICEConnectionChange:RTCICEConnectionConnected];

  [offeringExpectations expectStateChange:kRTCDataChannelStateOpen];
  [answeringExpectations expectDataChannel:@"offerDC"];
  [answeringExpectations expectStateChange:kRTCDataChannelStateOpen];

  [offeringExpectations expectICEGatheringChange:RTCICEGatheringComplete];
  [answeringExpectations expectICEGatheringChange:RTCICEGatheringComplete];

  sdpObserver = [[RTCSessionDescriptionSyncObserver alloc] init];
  [offeringExpectations expectSignalingChange:RTCSignalingStable];
  [offeringExpectations expectAddStream:@"aLMS"];
  [pcOffer setRemoteDescriptionWithDelegate:sdpObserver
                         sessionDescription:answerSDP];
  [sdpObserver wait];
  EXPECT_TRUE(sdpObserver.sessionDescription == NULL);

  EXPECT_TRUE([offerSDP.type isEqual:pcOffer.localDescription.type]);
  EXPECT_TRUE([answerSDP.type isEqual:pcOffer.remoteDescription.type]);
  EXPECT_TRUE([offerSDP.type isEqual:pcAnswer.remoteDescription.type]);
  EXPECT_TRUE([answerSDP.type isEqual:pcAnswer.localDescription.type]);

  for (RTCICECandidate* candidate in offeringExpectations
           .releaseReceivedICECandidates) {
    [pcAnswer addICECandidate:candidate];
  }
  for (RTCICECandidate* candidate in answeringExpectations
           .releaseReceivedICECandidates) {
    [pcOffer addICECandidate:candidate];
  }

  EXPECT_TRUE(
      [offeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                kRTCPeerConnectionTestTimeout]);
  EXPECT_TRUE(
      [answeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                 kRTCPeerConnectionTestTimeout]);

  EXPECT_EQ(pcOffer.signalingState, RTCSignalingStable);
  EXPECT_EQ(pcAnswer.signalingState, RTCSignalingStable);

  // Test send and receive UTF-8 text
  NSString* text = @"你好";
  NSData* textData = [text dataUsingEncoding:NSUTF8StringEncoding];
  RTCDataBuffer* buffer =
      [[RTCDataBuffer alloc] initWithData:textData isBinary:NO];
  [answeringExpectations expectMessage:[textData copy] isBinary:NO];
  EXPECT_TRUE([offeringExpectations.dataChannel sendData:buffer]);
  EXPECT_TRUE(
      [answeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                 kRTCPeerConnectionTestTimeout]);

  // Test send and receive binary data
  const size_t byteLength = 5;
  char bytes[byteLength] = {1, 2, 3, 4, 5};
  NSData* byteData = [NSData dataWithBytes:bytes length:byteLength];
  buffer = [[RTCDataBuffer alloc] initWithData:byteData isBinary:YES];
  [answeringExpectations expectMessage:[byteData copy] isBinary:YES];
  EXPECT_TRUE([offeringExpectations.dataChannel sendData:buffer]);
  EXPECT_TRUE(
      [answeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                 kRTCPeerConnectionTestTimeout]);

  [offeringExpectations expectStateChange:kRTCDataChannelStateClosing];
  [answeringExpectations expectStateChange:kRTCDataChannelStateClosing];
  [offeringExpectations expectStateChange:kRTCDataChannelStateClosed];
  [answeringExpectations expectStateChange:kRTCDataChannelStateClosed];

  [answeringExpectations.dataChannel close];
  [offeringExpectations.dataChannel close];

  EXPECT_TRUE(
      [offeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                kRTCPeerConnectionTestTimeout]);
  EXPECT_TRUE(
      [answeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                 kRTCPeerConnectionTestTimeout]);
  // Don't need to listen to further state changes.
  // TODO(tkchin): figure out why Closed->Closing without this.
  offeringExpectations.dataChannel.delegate = nil;
  answeringExpectations.dataChannel.delegate = nil;

  // Let the audio feedback run for 2s to allow human testing and to ensure
  // things stabilize.  TODO(fischman): replace seconds with # of video frames,
  // when we have video flowing.
  [[NSRunLoop currentRunLoop]
      runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2]];

  [offeringExpectations expectICEConnectionChange:RTCICEConnectionClosed];
  [answeringExpectations expectICEConnectionChange:RTCICEConnectionClosed];
  [offeringExpectations expectSignalingChange:RTCSignalingClosed];
  [answeringExpectations expectSignalingChange:RTCSignalingClosed];

  [pcOffer close];
  [pcAnswer close];

  EXPECT_TRUE(
      [offeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                kRTCPeerConnectionTestTimeout]);
  EXPECT_TRUE(
      [answeringExpectations waitForAllExpectationsToBeSatisfiedWithTimeout:
                                 kRTCPeerConnectionTestTimeout]);

  capturer = nil;
  videoSource = nil;
  pcOffer = nil;
  pcAnswer = nil;
  // TODO(fischman): be stricter about shutdown checks; ensure thread
  // counts return to where they were before the test kicked off, and
  // that all objects have in fact shut down.
}

@end

// TODO(fischman): move {Initialize,Cleanup}SSL into alloc/dealloc of
// RTCPeerConnectionTest and avoid the appearance of RTCPeerConnectionTest being
// a TestBase since it's not.
TEST(RTCPeerConnectionTest, SessionTest) {
  @autoreleasepool {
    rtc::InitializeSSL();
    // Since |factory| will own the signaling & worker threads, it's important
    // that it outlive the created PeerConnections since they self-delete on the
    // signaling thread, and if |factory| is freed first then a last refcount on
    // the factory will expire during this teardown, causing the signaling
    // thread to try to Join() with itself.  This is a hack to ensure that the
    // factory outlives RTCPeerConnection:dealloc.
    // See https://code.google.com/p/webrtc/issues/detail?id=3100.
    RTCPeerConnectionFactory* factory = [[RTCPeerConnectionFactory alloc] init];
    @autoreleasepool {
      RTCPeerConnectionTest* pcTest = [[RTCPeerConnectionTest alloc] init];
      [pcTest testCompleteSessionWithFactory:factory];
    }
    rtc::CleanupSSL();
  }
}
