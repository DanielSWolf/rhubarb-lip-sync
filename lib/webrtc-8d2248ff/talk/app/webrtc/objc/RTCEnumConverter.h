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

// TODO(tkchin): remove this in favor of having objc headers mirror their C++ counterparts.
// TODO(tkchin): see if we can move C++ enums into their own file so we can avoid all this
// conversion code.
#import "RTCTypes.h"

#import "talk/app/webrtc/objc/RTCPeerConnectionInterface+Internal.h"

@interface RTCEnumConverter : NSObject

// TODO(tkchin): rename these.
+ (RTCICEConnectionState)convertIceConnectionStateToObjC:
        (webrtc::PeerConnectionInterface::IceConnectionState)nativeState;

+ (RTCICEGatheringState)convertIceGatheringStateToObjC:
        (webrtc::PeerConnectionInterface::IceGatheringState)nativeState;

+ (RTCSignalingState)convertSignalingStateToObjC:
        (webrtc::PeerConnectionInterface::SignalingState)nativeState;

+ (webrtc::PeerConnectionInterface::StatsOutputLevel)
    convertStatsOutputLevelToNative:(RTCStatsOutputLevel)statsOutputLevel;

+ (RTCSourceState)convertSourceStateToObjC:
        (webrtc::MediaSourceInterface::SourceState)nativeState;

+ (webrtc::MediaStreamTrackInterface::TrackState)convertTrackStateToNative:
        (RTCTrackState)state;

+ (RTCTrackState)convertTrackStateToObjC:
        (webrtc::MediaStreamTrackInterface::TrackState)nativeState;

+ (RTCIceTransportsType)iceTransportsTypeForNativeEnum:
        (webrtc::PeerConnectionInterface::IceTransportsType)nativeEnum;

+ (webrtc::PeerConnectionInterface::IceTransportsType)nativeEnumForIceTransportsType:
        (RTCIceTransportsType)iceTransportsType;

+ (RTCBundlePolicy)bundlePolicyForNativeEnum:
        (webrtc::PeerConnectionInterface::BundlePolicy)nativeEnum;

+ (webrtc::PeerConnectionInterface::BundlePolicy)nativeEnumForBundlePolicy:
        (RTCBundlePolicy)bundlePolicy;

+ (RTCRtcpMuxPolicy)rtcpMuxPolicyForNativeEnum:
        (webrtc::PeerConnectionInterface::RtcpMuxPolicy)nativeEnum;

+ (webrtc::PeerConnectionInterface::RtcpMuxPolicy)nativeEnumForRtcpMuxPolicy:
        (RTCRtcpMuxPolicy)rtcpMuxPolicy;

+ (RTCTcpCandidatePolicy)tcpCandidatePolicyForNativeEnum:
        (webrtc::PeerConnectionInterface::TcpCandidatePolicy)nativeEnum;

+ (webrtc::PeerConnectionInterface::TcpCandidatePolicy)nativeEnumForTcpCandidatePolicy:
        (RTCTcpCandidatePolicy)tcpCandidatePolicy;

@end
