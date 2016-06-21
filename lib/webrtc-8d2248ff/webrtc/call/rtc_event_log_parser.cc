/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/call/rtc_event_log_parser.h"

#include <string.h>

#include <fstream>

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/call.h"
#include "webrtc/call/rtc_event_log.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/system_wrappers/include/file_wrapper.h"

namespace webrtc {

namespace {
MediaType GetRuntimeMediaType(rtclog::MediaType media_type) {
  switch (media_type) {
    case rtclog::MediaType::ANY:
      return MediaType::ANY;
    case rtclog::MediaType::AUDIO:
      return MediaType::AUDIO;
    case rtclog::MediaType::VIDEO:
      return MediaType::VIDEO;
    case rtclog::MediaType::DATA:
      return MediaType::DATA;
  }
  RTC_NOTREACHED();
  return MediaType::ANY;
}

RtcpMode GetRuntimeRtcpMode(rtclog::VideoReceiveConfig::RtcpMode rtcp_mode) {
  switch (rtcp_mode) {
    case rtclog::VideoReceiveConfig::RTCP_COMPOUND:
      return RtcpMode::kCompound;
    case rtclog::VideoReceiveConfig::RTCP_REDUCEDSIZE:
      return RtcpMode::kReducedSize;
  }
  RTC_NOTREACHED();
  return RtcpMode::kOff;
}

ParsedRtcEventLog::EventType GetRuntimeEventType(
    rtclog::Event::EventType event_type) {
  switch (event_type) {
    case rtclog::Event::UNKNOWN_EVENT:
      return ParsedRtcEventLog::EventType::UNKNOWN_EVENT;
    case rtclog::Event::LOG_START:
      return ParsedRtcEventLog::EventType::LOG_START;
    case rtclog::Event::LOG_END:
      return ParsedRtcEventLog::EventType::LOG_END;
    case rtclog::Event::RTP_EVENT:
      return ParsedRtcEventLog::EventType::RTP_EVENT;
    case rtclog::Event::RTCP_EVENT:
      return ParsedRtcEventLog::EventType::RTCP_EVENT;
    case rtclog::Event::AUDIO_PLAYOUT_EVENT:
      return ParsedRtcEventLog::EventType::AUDIO_PLAYOUT_EVENT;
    case rtclog::Event::BWE_PACKET_LOSS_EVENT:
      return ParsedRtcEventLog::EventType::BWE_PACKET_LOSS_EVENT;
    case rtclog::Event::BWE_PACKET_DELAY_EVENT:
      return ParsedRtcEventLog::EventType::BWE_PACKET_DELAY_EVENT;
    case rtclog::Event::VIDEO_RECEIVER_CONFIG_EVENT:
      return ParsedRtcEventLog::EventType::VIDEO_RECEIVER_CONFIG_EVENT;
    case rtclog::Event::VIDEO_SENDER_CONFIG_EVENT:
      return ParsedRtcEventLog::EventType::VIDEO_SENDER_CONFIG_EVENT;
    case rtclog::Event::AUDIO_RECEIVER_CONFIG_EVENT:
      return ParsedRtcEventLog::EventType::AUDIO_RECEIVER_CONFIG_EVENT;
    case rtclog::Event::AUDIO_SENDER_CONFIG_EVENT:
      return ParsedRtcEventLog::EventType::AUDIO_SENDER_CONFIG_EVENT;
  }
  RTC_NOTREACHED();
  return ParsedRtcEventLog::EventType::UNKNOWN_EVENT;
}

bool ParseVarInt(std::FILE* file, uint64_t* varint, size_t* bytes_read) {
  uint8_t byte;
  *varint = 0;
  for (*bytes_read = 0; *bytes_read < 10 && fread(&byte, 1, 1, file) == 1;
       ++(*bytes_read)) {
    // The most significant bit of each byte is 0 if it is the last byte in
    // the varint and 1 otherwise. Thus, we take the 7 least significant bits
    // of each byte and shift them 7 bits for each byte read previously to get
    // the (unsigned) integer.
    *varint |= static_cast<uint64_t>(byte & 0x7F) << (7 * *bytes_read);
    if ((byte & 0x80) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool ParsedRtcEventLog::ParseFile(const std::string& filename) {
  stream_.clear();
  const size_t kMaxEventSize = (1u << 16) - 1;
  char tmp_buffer[kMaxEventSize];

  std::FILE* file = fopen(filename.c_str(), "rb");
  if (!file) {
    LOG(LS_WARNING) << "Could not open file for reading.";
    return false;
  }

  while (1) {
    // Peek at the next message tag. The tag number is defined as
    // (fieldnumber << 3) | wire_type. In our case, the field number is
    // supposed to be 1 and the wire type for an length-delimited field is 2.
    const uint64_t kExpectedTag = (1 << 3) | 2;
    uint64_t tag;
    size_t bytes_read;
    if (!ParseVarInt(file, &tag, &bytes_read) || tag != kExpectedTag) {
      fclose(file);
      if (bytes_read == 0) {
        return true;  // Reached end of file.
      }
      LOG(LS_WARNING) << "Missing field tag from beginning of protobuf event.";
      return false;
    }

    // Peek at the length field.
    uint64_t message_length;
    if (!ParseVarInt(file, &message_length, &bytes_read)) {
      LOG(LS_WARNING) << "Missing message length after protobuf field tag.";
      fclose(file);
      return false;
    } else if (message_length > kMaxEventSize) {
      LOG(LS_WARNING) << "Protobuf message length is too large.";
      fclose(file);
      return false;
    }

    if (fread(tmp_buffer, 1, message_length, file) != message_length) {
      LOG(LS_WARNING) << "Failed to read protobuf message from file.";
      fclose(file);
      return false;
    }

    rtclog::Event event;
    if (!event.ParseFromArray(tmp_buffer, message_length)) {
      LOG(LS_WARNING) << "Failed to parse protobuf message.";
      fclose(file);
      return false;
    }
    stream_.push_back(event);
  }
}

size_t ParsedRtcEventLog::GetNumberOfEvents() const {
  return stream_.size();
}

int64_t ParsedRtcEventLog::GetTimestamp(size_t index) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_timestamp_us());
  return event.timestamp_us();
}

ParsedRtcEventLog::EventType ParsedRtcEventLog::GetEventType(
    size_t index) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_type());
  return GetRuntimeEventType(event.type());
}

// The header must have space for at least IP_PACKET_SIZE bytes.
void ParsedRtcEventLog::GetRtpHeader(size_t index,
                                     PacketDirection* incoming,
                                     MediaType* media_type,
                                     uint8_t* header,
                                     size_t* header_length,
                                     size_t* total_length) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::RTP_EVENT);
  RTC_CHECK(event.has_rtp_packet());
  const rtclog::RtpPacket& rtp_packet = event.rtp_packet();
  // Get direction of packet.
  RTC_CHECK(rtp_packet.has_incoming());
  if (incoming != nullptr) {
    *incoming = rtp_packet.incoming() ? kIncomingPacket : kOutgoingPacket;
  }
  // Get media type.
  RTC_CHECK(rtp_packet.has_type());
  if (media_type != nullptr) {
    *media_type = GetRuntimeMediaType(rtp_packet.type());
  }
  // Get packet length.
  RTC_CHECK(rtp_packet.has_packet_length());
  if (total_length != nullptr) {
    *total_length = rtp_packet.packet_length();
  }
  // Get header length.
  RTC_CHECK(rtp_packet.has_header());
  if (header_length != nullptr) {
    *header_length = rtp_packet.header().size();
  }
  // Get header contents.
  if (header != nullptr) {
    const size_t kMinRtpHeaderSize = 12;
    RTC_CHECK_GE(rtp_packet.header().size(), kMinRtpHeaderSize);
    RTC_CHECK_LE(rtp_packet.header().size(),
                 static_cast<size_t>(IP_PACKET_SIZE));
    memcpy(header, rtp_packet.header().data(), rtp_packet.header().size());
  }
}

// The packet must have space for at least IP_PACKET_SIZE bytes.
void ParsedRtcEventLog::GetRtcpPacket(size_t index,
                                      PacketDirection* incoming,
                                      MediaType* media_type,
                                      uint8_t* packet,
                                      size_t* length) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::RTCP_EVENT);
  RTC_CHECK(event.has_rtcp_packet());
  const rtclog::RtcpPacket& rtcp_packet = event.rtcp_packet();
  // Get direction of packet.
  RTC_CHECK(rtcp_packet.has_incoming());
  if (incoming != nullptr) {
    *incoming = rtcp_packet.incoming() ? kIncomingPacket : kOutgoingPacket;
  }
  // Get media type.
  RTC_CHECK(rtcp_packet.has_type());
  if (media_type != nullptr) {
    *media_type = GetRuntimeMediaType(rtcp_packet.type());
  }
  // Get packet length.
  RTC_CHECK(rtcp_packet.has_packet_data());
  if (length != nullptr) {
    *length = rtcp_packet.packet_data().size();
  }
  // Get packet contents.
  if (packet != nullptr) {
    RTC_CHECK_LE(rtcp_packet.packet_data().size(),
                 static_cast<unsigned>(IP_PACKET_SIZE));
    memcpy(packet, rtcp_packet.packet_data().data(),
           rtcp_packet.packet_data().size());
  }
}

void ParsedRtcEventLog::GetVideoReceiveConfig(
    size_t index,
    VideoReceiveStream::Config* config) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(config != nullptr);
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::VIDEO_RECEIVER_CONFIG_EVENT);
  RTC_CHECK(event.has_video_receiver_config());
  const rtclog::VideoReceiveConfig& receiver_config =
      event.video_receiver_config();
  // Get SSRCs.
  RTC_CHECK(receiver_config.has_remote_ssrc());
  config->rtp.remote_ssrc = receiver_config.remote_ssrc();
  RTC_CHECK(receiver_config.has_local_ssrc());
  config->rtp.local_ssrc = receiver_config.local_ssrc();
  // Get RTCP settings.
  RTC_CHECK(receiver_config.has_rtcp_mode());
  config->rtp.rtcp_mode = GetRuntimeRtcpMode(receiver_config.rtcp_mode());
  RTC_CHECK(receiver_config.has_remb());
  config->rtp.remb = receiver_config.remb();
  // Get RTX map.
  config->rtp.rtx.clear();
  for (int i = 0; i < receiver_config.rtx_map_size(); i++) {
    const rtclog::RtxMap& map = receiver_config.rtx_map(i);
    RTC_CHECK(map.has_payload_type());
    RTC_CHECK(map.has_config());
    RTC_CHECK(map.config().has_rtx_ssrc());
    RTC_CHECK(map.config().has_rtx_payload_type());
    webrtc::VideoReceiveStream::Config::Rtp::Rtx rtx_pair;
    rtx_pair.ssrc = map.config().rtx_ssrc();
    rtx_pair.payload_type = map.config().rtx_payload_type();
    config->rtp.rtx.insert(std::make_pair(map.payload_type(), rtx_pair));
  }
  // Get header extensions.
  config->rtp.extensions.clear();
  for (int i = 0; i < receiver_config.header_extensions_size(); i++) {
    RTC_CHECK(receiver_config.header_extensions(i).has_name());
    RTC_CHECK(receiver_config.header_extensions(i).has_id());
    const std::string& name = receiver_config.header_extensions(i).name();
    int id = receiver_config.header_extensions(i).id();
    config->rtp.extensions.push_back(RtpExtension(name, id));
  }
  // Get decoders.
  config->decoders.clear();
  for (int i = 0; i < receiver_config.decoders_size(); i++) {
    RTC_CHECK(receiver_config.decoders(i).has_name());
    RTC_CHECK(receiver_config.decoders(i).has_payload_type());
    VideoReceiveStream::Decoder decoder;
    decoder.payload_name = receiver_config.decoders(i).name();
    decoder.payload_type = receiver_config.decoders(i).payload_type();
    config->decoders.push_back(decoder);
  }
}

void ParsedRtcEventLog::GetVideoSendConfig(
    size_t index,
    VideoSendStream::Config* config) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(config != nullptr);
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::VIDEO_SENDER_CONFIG_EVENT);
  RTC_CHECK(event.has_video_sender_config());
  const rtclog::VideoSendConfig& sender_config = event.video_sender_config();
  // Get SSRCs.
  config->rtp.ssrcs.clear();
  for (int i = 0; i < sender_config.ssrcs_size(); i++) {
    config->rtp.ssrcs.push_back(sender_config.ssrcs(i));
  }
  // Get header extensions.
  config->rtp.extensions.clear();
  for (int i = 0; i < sender_config.header_extensions_size(); i++) {
    RTC_CHECK(sender_config.header_extensions(i).has_name());
    RTC_CHECK(sender_config.header_extensions(i).has_id());
    const std::string& name = sender_config.header_extensions(i).name();
    int id = sender_config.header_extensions(i).id();
    config->rtp.extensions.push_back(RtpExtension(name, id));
  }
  // Get RTX settings.
  config->rtp.rtx.ssrcs.clear();
  for (int i = 0; i < sender_config.rtx_ssrcs_size(); i++) {
    config->rtp.rtx.ssrcs.push_back(sender_config.rtx_ssrcs(i));
  }
  if (sender_config.rtx_ssrcs_size() > 0) {
    RTC_CHECK(sender_config.has_rtx_payload_type());
    config->rtp.rtx.payload_type = sender_config.rtx_payload_type();
  } else {
    // Reset RTX payload type default value if no RTX SSRCs are used.
    config->rtp.rtx.payload_type = -1;
  }
  // Get encoder.
  RTC_CHECK(sender_config.has_encoder());
  RTC_CHECK(sender_config.encoder().has_name());
  RTC_CHECK(sender_config.encoder().has_payload_type());
  config->encoder_settings.payload_name = sender_config.encoder().name();
  config->encoder_settings.payload_type =
      sender_config.encoder().payload_type();
}

void ParsedRtcEventLog::GetAudioPlayout(size_t index, uint32_t* ssrc) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::AUDIO_PLAYOUT_EVENT);
  RTC_CHECK(event.has_audio_playout_event());
  const rtclog::AudioPlayoutEvent& loss_event = event.audio_playout_event();
  RTC_CHECK(loss_event.has_local_ssrc());
  if (ssrc != nullptr) {
    *ssrc = loss_event.local_ssrc();
  }
}

void ParsedRtcEventLog::GetBwePacketLossEvent(size_t index,
                                              int32_t* bitrate,
                                              uint8_t* fraction_loss,
                                              int32_t* total_packets) const {
  RTC_CHECK_LT(index, GetNumberOfEvents());
  const rtclog::Event& event = stream_[index];
  RTC_CHECK(event.has_type());
  RTC_CHECK_EQ(event.type(), rtclog::Event::BWE_PACKET_LOSS_EVENT);
  RTC_CHECK(event.has_bwe_packet_loss_event());
  const rtclog::BwePacketLossEvent& loss_event = event.bwe_packet_loss_event();
  RTC_CHECK(loss_event.has_bitrate());
  if (bitrate != nullptr) {
    *bitrate = loss_event.bitrate();
  }
  RTC_CHECK(loss_event.has_fraction_loss());
  if (fraction_loss != nullptr) {
    *fraction_loss = loss_event.fraction_loss();
  }
  RTC_CHECK(loss_event.has_total_packets());
  if (total_packets != nullptr) {
    *total_packets = loss_event.total_packets();
  }
}

}  // namespace webrtc
