/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_
#define WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_
#pragma once

#include <string>

#include "webrtc/base/basictypes.h"

extern const char kAudioLabel[];
extern const char kVideoLabel[];
extern const char kStreamLabel[];
extern const uint16_t kDefaultServerPort;

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value);
std::string GetPeerConnectionString();
std::string GetDefaultServerName();
std::string GetPeerName();

#endif  // WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_
