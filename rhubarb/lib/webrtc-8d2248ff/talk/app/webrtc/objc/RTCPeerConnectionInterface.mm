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

#import "talk/app/webrtc/objc/RTCPeerConnectionInterface+Internal.h"

#import "talk/app/webrtc/objc/RTCEnumConverter.h"
#import "talk/app/webrtc/objc/RTCICEServer+Internal.h"
#import "talk/app/webrtc/objc/public/RTCLogging.h"

#include <memory>

#include "webrtc/base/rtccertificategenerator.h"

@implementation RTCConfiguration

@synthesize iceTransportsType = _iceTransportsType;
@synthesize iceServers = _iceServers;
@synthesize bundlePolicy = _bundlePolicy;
@synthesize rtcpMuxPolicy = _rtcpMuxPolicy;
@synthesize tcpCandidatePolicy = _tcpCandidatePolicy;
@synthesize audioJitterBufferMaxPackets = _audioJitterBufferMaxPackets;
@synthesize iceConnectionReceivingTimeout = _iceConnectionReceivingTimeout;
@synthesize iceBackupCandidatePairPingInterval = _iceBackupCandidatePairPingInterval;
@synthesize keyType = _keyType;

- (instancetype)init {
  if (self = [super init]) {
    // Copy defaults.
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    _iceTransportsType = [RTCEnumConverter iceTransportsTypeForNativeEnum:config.type];
    _bundlePolicy = [RTCEnumConverter bundlePolicyForNativeEnum:config.bundle_policy];
    _rtcpMuxPolicy = [RTCEnumConverter rtcpMuxPolicyForNativeEnum:config.rtcp_mux_policy];
    _tcpCandidatePolicy =
        [RTCEnumConverter tcpCandidatePolicyForNativeEnum:config.tcp_candidate_policy];
    _audioJitterBufferMaxPackets = config.audio_jitter_buffer_max_packets;
    _iceConnectionReceivingTimeout = config.ice_connection_receiving_timeout;
    _iceBackupCandidatePairPingInterval = config.ice_backup_candidate_pair_ping_interval;
    _keyType = kRTCEncryptionKeyTypeECDSA;
  }
  return self;
}

- (instancetype)initWithIceTransportsType:(RTCIceTransportsType)iceTransportsType
                             bundlePolicy:(RTCBundlePolicy)bundlePolicy
                            rtcpMuxPolicy:(RTCRtcpMuxPolicy)rtcpMuxPolicy
                       tcpCandidatePolicy:(RTCTcpCandidatePolicy)tcpCandidatePolicy
              audioJitterBufferMaxPackets:(int)audioJitterBufferMaxPackets
            iceConnectionReceivingTimeout:(int)iceConnectionReceivingTimeout
       iceBackupCandidatePairPingInterval:(int)iceBackupCandidatePairPingInterval {
  if (self = [super init]) {
    _iceTransportsType = iceTransportsType;
    _bundlePolicy = bundlePolicy;
    _rtcpMuxPolicy = rtcpMuxPolicy;
    _tcpCandidatePolicy = tcpCandidatePolicy;
    _audioJitterBufferMaxPackets = audioJitterBufferMaxPackets;
    _iceConnectionReceivingTimeout = iceConnectionReceivingTimeout;
    _iceBackupCandidatePairPingInterval = iceBackupCandidatePairPingInterval;
  }
  return self;
}

#pragma mark - Private

- (webrtc::PeerConnectionInterface::RTCConfiguration *)
    createNativeConfiguration {
  std::unique_ptr<webrtc::PeerConnectionInterface::RTCConfiguration>
      nativeConfig(new webrtc::PeerConnectionInterface::RTCConfiguration());
  nativeConfig->type =
      [RTCEnumConverter nativeEnumForIceTransportsType:_iceTransportsType];
  for (RTCICEServer *iceServer : _iceServers) {
    nativeConfig->servers.push_back(iceServer.iceServer);
  }
  nativeConfig->bundle_policy =
      [RTCEnumConverter nativeEnumForBundlePolicy:_bundlePolicy];
  nativeConfig->rtcp_mux_policy =
      [RTCEnumConverter nativeEnumForRtcpMuxPolicy:_rtcpMuxPolicy];
  nativeConfig->tcp_candidate_policy =
      [RTCEnumConverter nativeEnumForTcpCandidatePolicy:_tcpCandidatePolicy];
  nativeConfig->audio_jitter_buffer_max_packets = _audioJitterBufferMaxPackets;
  nativeConfig->ice_connection_receiving_timeout =
      _iceConnectionReceivingTimeout;
  nativeConfig->ice_backup_candidate_pair_ping_interval =
      _iceBackupCandidatePairPingInterval;
  rtc::KeyType keyType =
      [[self class] nativeEncryptionKeyTypeForKeyType:_keyType];
  if (keyType != rtc::KT_DEFAULT) {
    rtc::scoped_refptr<rtc::RTCCertificate> certificate =
        rtc::RTCCertificateGenerator::GenerateCertificate(
            rtc::KeyParams(keyType), rtc::Optional<uint64_t>());
    if (!certificate) {
      RTCLogError(@"Failed to generate certificate.");
      return nullptr;
    }
    nativeConfig->certificates.push_back(certificate);
  }
  return nativeConfig.release();
}

+ (rtc::KeyType)nativeEncryptionKeyTypeForKeyType:
    (RTCEncryptionKeyType)keyType {
  switch (keyType) {
    case kRTCEncryptionKeyTypeRSA:
      return rtc::KT_RSA;
    case kRTCEncryptionKeyTypeECDSA:
      return rtc::KT_ECDSA;
  }
}

@end
