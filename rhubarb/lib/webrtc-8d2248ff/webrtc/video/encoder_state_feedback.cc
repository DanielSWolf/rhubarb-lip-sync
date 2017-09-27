/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/encoder_state_feedback.h"

#include "webrtc/base/checks.h"
#include "webrtc/video/vie_encoder.h"

static const int kMinKeyFrameRequestIntervalMs = 300;

namespace webrtc {

EncoderStateFeedback::EncoderStateFeedback(Clock* clock,
                                           const std::vector<uint32_t>& ssrcs,
                                           ViEEncoder* encoder)
    : clock_(clock),
      ssrcs_(ssrcs),
      vie_encoder_(encoder),
      time_last_intra_request_ms_(ssrcs.size(), -1) {
  RTC_DCHECK(!ssrcs.empty());
}

bool EncoderStateFeedback::HasSsrc(uint32_t ssrc) {
  for (uint32_t registered_ssrc : ssrcs_) {
    if (registered_ssrc == ssrc)
      return true;
  }
  return false;
}

size_t EncoderStateFeedback::GetStreamIndex(uint32_t ssrc) {
  for (size_t i = 0; i < ssrcs_.size(); ++i) {
    if (ssrcs_[i] == ssrc)
      return i;
  }
  RTC_NOTREACHED() << "Unknown ssrc " << ssrc;
  return 0;
}

void EncoderStateFeedback::OnReceivedIntraFrameRequest(uint32_t ssrc) {
  if (!HasSsrc(ssrc))
    return;

  size_t index = GetStreamIndex(ssrc);
  {
    int64_t now_ms = clock_->TimeInMilliseconds();
    rtc::CritScope lock(&crit_);
    if (time_last_intra_request_ms_[index] + kMinKeyFrameRequestIntervalMs >
        now_ms) {
      return;
    }
    time_last_intra_request_ms_[index] = now_ms;
  }

  vie_encoder_->OnReceivedIntraFrameRequest(index);
}

void EncoderStateFeedback::OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) {
  if (!HasSsrc(ssrc))
    return;

  vie_encoder_->OnReceivedSLI(picture_id);
}

void EncoderStateFeedback::OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) {
  if (!HasSsrc(ssrc))
    return;

  vie_encoder_->OnReceivedRPSI(picture_id);
}

// Sending SSRCs for this encoder should never change since they are configured
// once and not reconfigured, however, OnLocalSsrcChanged is called when the
// RtpModules are created with a different SSRC than what will be used in the
// end.
// TODO(perkj): Can we make sure the RTP module is created with the right SSRC
// from the beginning so this method is not triggered during creation ?
void EncoderStateFeedback::OnLocalSsrcChanged(uint32_t old_ssrc,
                                              uint32_t new_ssrc) {
  if (!RTC_DCHECK_IS_ON)
    return;

  if (old_ssrc == 0)  // old_ssrc == 0 during creation.
    return;
  // SSRC shouldn't change to something we haven't already registered with the
  // encoder.
  RTC_DCHECK(HasSsrc(new_ssrc));
}

}  // namespace webrtc
