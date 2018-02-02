/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/payload_router.h"

#include "webrtc/base/checks.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/modules/video_coding/include/video_codec_interface.h"

namespace webrtc {

namespace {
// Map information from info into rtp.
void CopyCodecSpecific(const CodecSpecificInfo* info, RTPVideoHeader* rtp) {
  RTC_DCHECK(info);
  switch (info->codecType) {
    case kVideoCodecVP8: {
      rtp->codec = kRtpVideoVp8;
      rtp->codecHeader.VP8.InitRTPVideoHeaderVP8();
      rtp->codecHeader.VP8.pictureId = info->codecSpecific.VP8.pictureId;
      rtp->codecHeader.VP8.nonReference = info->codecSpecific.VP8.nonReference;
      rtp->codecHeader.VP8.temporalIdx = info->codecSpecific.VP8.temporalIdx;
      rtp->codecHeader.VP8.layerSync = info->codecSpecific.VP8.layerSync;
      rtp->codecHeader.VP8.tl0PicIdx = info->codecSpecific.VP8.tl0PicIdx;
      rtp->codecHeader.VP8.keyIdx = info->codecSpecific.VP8.keyIdx;
      rtp->simulcastIdx = info->codecSpecific.VP8.simulcastIdx;
      return;
    }
    case kVideoCodecVP9: {
      rtp->codec = kRtpVideoVp9;
      rtp->codecHeader.VP9.InitRTPVideoHeaderVP9();
      rtp->codecHeader.VP9.inter_pic_predicted =
          info->codecSpecific.VP9.inter_pic_predicted;
      rtp->codecHeader.VP9.flexible_mode =
          info->codecSpecific.VP9.flexible_mode;
      rtp->codecHeader.VP9.ss_data_available =
          info->codecSpecific.VP9.ss_data_available;
      rtp->codecHeader.VP9.picture_id = info->codecSpecific.VP9.picture_id;
      rtp->codecHeader.VP9.tl0_pic_idx = info->codecSpecific.VP9.tl0_pic_idx;
      rtp->codecHeader.VP9.temporal_idx = info->codecSpecific.VP9.temporal_idx;
      rtp->codecHeader.VP9.spatial_idx = info->codecSpecific.VP9.spatial_idx;
      rtp->codecHeader.VP9.temporal_up_switch =
          info->codecSpecific.VP9.temporal_up_switch;
      rtp->codecHeader.VP9.inter_layer_predicted =
          info->codecSpecific.VP9.inter_layer_predicted;
      rtp->codecHeader.VP9.gof_idx = info->codecSpecific.VP9.gof_idx;
      rtp->codecHeader.VP9.num_spatial_layers =
          info->codecSpecific.VP9.num_spatial_layers;

      if (info->codecSpecific.VP9.ss_data_available) {
        rtp->codecHeader.VP9.spatial_layer_resolution_present =
            info->codecSpecific.VP9.spatial_layer_resolution_present;
        if (info->codecSpecific.VP9.spatial_layer_resolution_present) {
          for (size_t i = 0; i < info->codecSpecific.VP9.num_spatial_layers;
               ++i) {
            rtp->codecHeader.VP9.width[i] = info->codecSpecific.VP9.width[i];
            rtp->codecHeader.VP9.height[i] = info->codecSpecific.VP9.height[i];
          }
        }
        rtp->codecHeader.VP9.gof.CopyGofInfoVP9(info->codecSpecific.VP9.gof);
      }

      rtp->codecHeader.VP9.num_ref_pics = info->codecSpecific.VP9.num_ref_pics;
      for (int i = 0; i < info->codecSpecific.VP9.num_ref_pics; ++i)
        rtp->codecHeader.VP9.pid_diff[i] = info->codecSpecific.VP9.p_diff[i];
      return;
    }
    case kVideoCodecH264:
      rtp->codec = kRtpVideoH264;
      return;
    case kVideoCodecGeneric:
      rtp->codec = kRtpVideoGeneric;
      rtp->simulcastIdx = info->codecSpecific.generic.simulcast_idx;
      return;
    default:
      return;
  }
}

}  // namespace

PayloadRouter::PayloadRouter(const std::vector<RtpRtcp*>& rtp_modules,
                             int payload_type)
    : active_(false),
      num_sending_modules_(1),
      rtp_modules_(rtp_modules),
      payload_type_(payload_type) {
  UpdateModuleSendingState();
}

PayloadRouter::~PayloadRouter() {}

size_t PayloadRouter::DefaultMaxPayloadLength() {
  const size_t kIpUdpSrtpLength = 44;
  return IP_PACKET_SIZE - kIpUdpSrtpLength;
}

void PayloadRouter::set_active(bool active) {
  rtc::CritScope lock(&crit_);
  if (active_ == active)
    return;
  active_ = active;
  UpdateModuleSendingState();
}

bool PayloadRouter::active() {
  rtc::CritScope lock(&crit_);
  return active_ && !rtp_modules_.empty();
}

void PayloadRouter::SetSendStreams(const std::vector<VideoStream>& streams) {
  RTC_DCHECK_LE(streams.size(), rtp_modules_.size());
  rtc::CritScope lock(&crit_);
  num_sending_modules_ = streams.size();
  streams_ = streams;
  // TODO(perkj): Should SetSendStreams also call SetTargetSendBitrate?
  UpdateModuleSendingState();
}

void PayloadRouter::UpdateModuleSendingState() {
  for (size_t i = 0; i < num_sending_modules_; ++i) {
    rtp_modules_[i]->SetSendingStatus(active_);
    rtp_modules_[i]->SetSendingMediaStatus(active_);
  }
  // Disable inactive modules.
  for (size_t i = num_sending_modules_; i < rtp_modules_.size(); ++i) {
    rtp_modules_[i]->SetSendingStatus(false);
    rtp_modules_[i]->SetSendingMediaStatus(false);
  }
}

int32_t PayloadRouter::Encoded(const EncodedImage& encoded_image,
                               const CodecSpecificInfo* codec_specific_info,
                               const RTPFragmentationHeader* fragmentation) {
  rtc::CritScope lock(&crit_);
  RTC_DCHECK(!rtp_modules_.empty());
  if (!active_ || num_sending_modules_ == 0)
    return -1;

  int stream_idx = 0;

  RTPVideoHeader rtp_video_header;
  memset(&rtp_video_header, 0, sizeof(RTPVideoHeader));
  if (codec_specific_info)
    CopyCodecSpecific(codec_specific_info, &rtp_video_header);
  rtp_video_header.rotation = encoded_image.rotation_;
  rtp_video_header.playout_delay = encoded_image.playout_delay_;

  RTC_DCHECK_LT(rtp_video_header.simulcastIdx, rtp_modules_.size());
  // The simulcast index might actually be larger than the number of modules
  // in case the encoder was processing a frame during a codec reconfig.
  if (rtp_video_header.simulcastIdx >= num_sending_modules_)
    return -1;
  stream_idx = rtp_video_header.simulcastIdx;

  return rtp_modules_[stream_idx]->SendOutgoingData(
      encoded_image._frameType, payload_type_, encoded_image._timeStamp,
      encoded_image.capture_time_ms_, encoded_image._buffer,
      encoded_image._length, fragmentation, &rtp_video_header);
}

void PayloadRouter::SetTargetSendBitrate(uint32_t bitrate_bps) {
  rtc::CritScope lock(&crit_);
  RTC_DCHECK_LE(streams_.size(), rtp_modules_.size());

  // TODO(sprang): Rebase https://codereview.webrtc.org/1913073002/ on top of
  // this.
  int bitrate_remainder = bitrate_bps;
  for (size_t i = 0; i < streams_.size() && bitrate_remainder > 0; ++i) {
    int stream_bitrate = 0;
    if (streams_[i].max_bitrate_bps > bitrate_remainder) {
      stream_bitrate = bitrate_remainder;
    } else {
      stream_bitrate = streams_[i].max_bitrate_bps;
    }
    bitrate_remainder -= stream_bitrate;
    rtp_modules_[i]->SetTargetSendBitrate(stream_bitrate);
  }
}

size_t PayloadRouter::MaxPayloadLength() const {
  size_t min_payload_length = DefaultMaxPayloadLength();
  rtc::CritScope lock(&crit_);
  for (size_t i = 0; i < num_sending_modules_; ++i) {
    size_t module_payload_length = rtp_modules_[i]->MaxDataPayloadLength();
    if (module_payload_length < min_payload_length)
      min_payload_length = module_payload_length;
  }
  return min_payload_length;
}

}  // namespace webrtc
