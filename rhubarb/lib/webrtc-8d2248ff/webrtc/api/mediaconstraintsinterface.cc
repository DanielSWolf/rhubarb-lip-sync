/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/mediaconstraintsinterface.h"

#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/stringencode.h"

namespace webrtc {

const char MediaConstraintsInterface::kValueTrue[] = "true";
const char MediaConstraintsInterface::kValueFalse[] = "false";

// Constraints declared as static members in mediastreaminterface.h
// Specified by draft-alvestrand-constraints-resolution-00b
const char MediaConstraintsInterface::kMinAspectRatio[] = "minAspectRatio";
const char MediaConstraintsInterface::kMaxAspectRatio[] = "maxAspectRatio";
const char MediaConstraintsInterface::kMaxWidth[] = "maxWidth";
const char MediaConstraintsInterface::kMinWidth[] = "minWidth";
const char MediaConstraintsInterface::kMaxHeight[] = "maxHeight";
const char MediaConstraintsInterface::kMinHeight[] = "minHeight";
const char MediaConstraintsInterface::kMaxFrameRate[] = "maxFrameRate";
const char MediaConstraintsInterface::kMinFrameRate[] = "minFrameRate";

// Audio constraints.
const char MediaConstraintsInterface::kEchoCancellation[] =
    "echoCancellation";
const char MediaConstraintsInterface::kGoogEchoCancellation[] =
    "googEchoCancellation";
const char MediaConstraintsInterface::kExtendedFilterEchoCancellation[] =
    "googEchoCancellation2";
const char MediaConstraintsInterface::kDAEchoCancellation[] =
    "googDAEchoCancellation";
const char MediaConstraintsInterface::kAutoGainControl[] =
    "googAutoGainControl";
const char MediaConstraintsInterface::kExperimentalAutoGainControl[] =
    "googAutoGainControl2";
const char MediaConstraintsInterface::kNoiseSuppression[] =
    "googNoiseSuppression";
const char MediaConstraintsInterface::kExperimentalNoiseSuppression[] =
    "googNoiseSuppression2";
const char MediaConstraintsInterface::kIntelligibilityEnhancer[] =
    "intelligibilityEnhancer";
const char MediaConstraintsInterface::kHighpassFilter[] =
    "googHighpassFilter";
const char MediaConstraintsInterface::kTypingNoiseDetection[] =
    "googTypingNoiseDetection";
const char MediaConstraintsInterface::kAudioMirroring[] = "googAudioMirroring";

// Google-specific constraint keys for a local video source (getUserMedia).
const char MediaConstraintsInterface::kNoiseReduction[] = "googNoiseReduction";

// Constraint keys for CreateOffer / CreateAnswer defined in W3C specification.
const char MediaConstraintsInterface::kOfferToReceiveAudio[] =
    "OfferToReceiveAudio";
const char MediaConstraintsInterface::kOfferToReceiveVideo[] =
    "OfferToReceiveVideo";
const char MediaConstraintsInterface::kVoiceActivityDetection[] =
    "VoiceActivityDetection";
const char MediaConstraintsInterface::kIceRestart[] =
    "IceRestart";
// Google specific constraint for BUNDLE enable/disable.
const char MediaConstraintsInterface::kUseRtpMux[] =
    "googUseRtpMUX";

// Below constraints should be used during PeerConnection construction.
const char MediaConstraintsInterface::kEnableDtlsSrtp[] =
    "DtlsSrtpKeyAgreement";
const char MediaConstraintsInterface::kEnableRtpDataChannels[] =
    "RtpDataChannels";
// Google-specific constraint keys.
const char MediaConstraintsInterface::kEnableDscp[] = "googDscp";
const char MediaConstraintsInterface::kEnableIPv6[] = "googIPv6";
const char MediaConstraintsInterface::kEnableVideoSuspendBelowMinBitrate[] =
    "googSuspendBelowMinBitrate";
const char MediaConstraintsInterface::kCombinedAudioVideoBwe[] =
    "googCombinedAudioVideoBwe";
const char MediaConstraintsInterface::kScreencastMinBitrate[] =
    "googScreencastMinBitrate";
// TODO(ronghuawu): Remove once cpu overuse detection is stable.
const char MediaConstraintsInterface::kCpuOveruseDetection[] =
    "googCpuOveruseDetection";
const char MediaConstraintsInterface::kPayloadPadding[] = "googPayloadPadding";


// Set |value| to the value associated with the first appearance of |key|, or
// return false if |key| is not found.
bool MediaConstraintsInterface::Constraints::FindFirst(
    const std::string& key, std::string* value) const {
  for (Constraints::const_iterator iter = begin(); iter != end(); ++iter) {
    if (iter->key == key) {
      *value = iter->value;
      return true;
    }
  }
  return false;
}

// Find the highest-priority instance of the boolean-valued constraint) named by
// |key| and return its value as |value|. |constraints| can be null.
// If |mandatory_constraints| is non-null, it is incremented if the key appears
// among the mandatory constraints.
// Returns true if the key was found and has a valid boolean value.
// If the key appears multiple times as an optional constraint, appearances
// after the first are ignored.
// Note: Because this uses FindFirst, repeated optional constraints whose
// first instance has an unrecognized value are not handled precisely in
// accordance with the specification.
bool FindConstraint(const MediaConstraintsInterface* constraints,
                    const std::string& key, bool* value,
                    size_t* mandatory_constraints) {
  std::string string_value;
  if (!constraints) {
    return false;
  }
  if (constraints->GetMandatory().FindFirst(key, &string_value)) {
    if (mandatory_constraints) {
      ++*mandatory_constraints;
    }
    return rtc::FromString(string_value, value);
  }
  if (constraints->GetOptional().FindFirst(key, &string_value)) {
    return rtc::FromString(string_value, value);
  }
  return false;
}

// As above, but for integers.
bool FindConstraint(const MediaConstraintsInterface* constraints,
                    const std::string& key,
                    int* value,
                    size_t* mandatory_constraints) {
  std::string string_value;
  if (!constraints) {
    return false;
  }
  if (constraints->GetMandatory().FindFirst(key, &string_value)) {
    if (mandatory_constraints) {
      ++*mandatory_constraints;
    }
    return rtc::FromString(string_value, value);
  }
  if (constraints->GetOptional().FindFirst(key, &string_value)) {
    return rtc::FromString(string_value, value);
  }
  return false;
}

void ConstraintToOptionalBool(const MediaConstraintsInterface* constraints,
                              const std::string& key,
                              rtc::Optional<bool>* value_out) {
  bool value;
  bool present = FindConstraint(constraints, key, &value, nullptr);
  if (present) {
    *value_out = rtc::Optional<bool>(value);
  }
}

void ConstraintToOptionalInt(const MediaConstraintsInterface* constraints,
                             const std::string& key,
                             rtc::Optional<int>* value_out) {
  int value;
  bool present = FindConstraint(constraints, key, &value, nullptr);
  if (present) {
    *value_out = rtc::Optional<int>(value);
  }
}

void CopyConstraintsIntoRtcConfiguration(
    const MediaConstraintsInterface* constraints,
    PeerConnectionInterface::RTCConfiguration* configuration) {
  // Copy info from constraints into configuration, if present.
  if (!constraints) {
    return;
  }

  bool enable_ipv6;
  if (FindConstraint(constraints, MediaConstraintsInterface::kEnableIPv6,
                     &enable_ipv6, nullptr)) {
    configuration->disable_ipv6 = !enable_ipv6;
  }
  FindConstraint(constraints, MediaConstraintsInterface::kEnableDscp,
                 &configuration->media_config.enable_dscp, nullptr);
  FindConstraint(
      constraints, MediaConstraintsInterface::kCpuOveruseDetection,
      &configuration->media_config.video.enable_cpu_overuse_detection, nullptr);
  FindConstraint(constraints, MediaConstraintsInterface::kEnableRtpDataChannels,
                 &configuration->enable_rtp_data_channel, nullptr);
  // Find Suspend Below Min Bitrate constraint.
  FindConstraint(constraints,
                 MediaConstraintsInterface::kEnableVideoSuspendBelowMinBitrate,
                 &configuration->media_config.video.suspend_below_min_bitrate,
                 nullptr);
  ConstraintToOptionalInt(constraints,
                          MediaConstraintsInterface::kScreencastMinBitrate,
                          &configuration->screencast_min_bitrate);
  ConstraintToOptionalBool(constraints,
                           MediaConstraintsInterface::kCombinedAudioVideoBwe,
                           &configuration->combined_audio_video_bwe);
  ConstraintToOptionalBool(constraints,
                           MediaConstraintsInterface::kEnableDtlsSrtp,
                           &configuration->enable_dtls_srtp);
}

}  // namespace webrtc
