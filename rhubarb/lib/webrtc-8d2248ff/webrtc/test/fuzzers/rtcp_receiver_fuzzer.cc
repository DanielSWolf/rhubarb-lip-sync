/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "webrtc/base/checks.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_receiver.h"
#include "webrtc/system_wrappers/include/clock.h"

namespace webrtc {

void FuzzOneInput(const uint8_t* data, size_t size) {
  RTCPUtility::RTCPParserV2 rtcp_parser(data, size, true);
  if (!rtcp_parser.IsValid())
    return;

  webrtc::SimulatedClock clock(1234);
  RTCPReceiver receiver(&clock, false, nullptr, nullptr, nullptr, nullptr,
                        nullptr);

  RTCPHelp::RTCPPacketInformation rtcp_packet_information;
  receiver.IncomingRTCPPacket(rtcp_packet_information, &rtcp_parser);
}
}  // namespace webrtc

