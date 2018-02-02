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

#import "RTCPeerConnectionObserver.h"

#import "RTCDataChannel+Internal.h"
#import "RTCEnumConverter.h"
#import "RTCICECandidate+Internal.h"
#import "RTCMediaStream+Internal.h"

namespace webrtc {

RTCPeerConnectionObserver::RTCPeerConnectionObserver(
    RTCPeerConnection* peerConnection) {
  _peerConnection = peerConnection;
}

RTCPeerConnectionObserver::~RTCPeerConnectionObserver() {
}

void RTCPeerConnectionObserver::OnSignalingChange(
    PeerConnectionInterface::SignalingState new_state) {
  RTCSignalingState state =
      [RTCEnumConverter convertSignalingStateToObjC:new_state];
  [_peerConnection.delegate peerConnection:_peerConnection
                     signalingStateChanged:state];
}

void RTCPeerConnectionObserver::OnAddStream(MediaStreamInterface* stream) {
  RTCMediaStream* mediaStream =
      [[RTCMediaStream alloc] initWithMediaStream:stream];
  [_peerConnection.delegate peerConnection:_peerConnection
                               addedStream:mediaStream];
}

void RTCPeerConnectionObserver::OnRemoveStream(MediaStreamInterface* stream) {
  RTCMediaStream* mediaStream =
      [[RTCMediaStream alloc] initWithMediaStream:stream];
  [_peerConnection.delegate peerConnection:_peerConnection
                             removedStream:mediaStream];
}

void RTCPeerConnectionObserver::OnDataChannel(
    DataChannelInterface* data_channel) {
  RTCDataChannel* dataChannel =
      [[RTCDataChannel alloc] initWithDataChannel:data_channel];
  [_peerConnection.delegate peerConnection:_peerConnection
                        didOpenDataChannel:dataChannel];
}

void RTCPeerConnectionObserver::OnRenegotiationNeeded() {
  id<RTCPeerConnectionDelegate> delegate = _peerConnection.delegate;
  [delegate peerConnectionOnRenegotiationNeeded:_peerConnection];
}

void RTCPeerConnectionObserver::OnIceConnectionChange(
    PeerConnectionInterface::IceConnectionState new_state) {
  RTCICEConnectionState state =
      [RTCEnumConverter convertIceConnectionStateToObjC:new_state];
  [_peerConnection.delegate peerConnection:_peerConnection
                      iceConnectionChanged:state];
}

void RTCPeerConnectionObserver::OnIceGatheringChange(
    PeerConnectionInterface::IceGatheringState new_state) {
  RTCICEGatheringState state =
      [RTCEnumConverter convertIceGatheringStateToObjC:new_state];
  [_peerConnection.delegate peerConnection:_peerConnection
                       iceGatheringChanged:state];
}

void RTCPeerConnectionObserver::OnIceCandidate(
    const IceCandidateInterface* candidate) {
  RTCICECandidate* iceCandidate =
      [[RTCICECandidate alloc] initWithCandidate:candidate];
  [_peerConnection.delegate peerConnection:_peerConnection
                           gotICECandidate:iceCandidate];
}

}  // namespace webrtc
