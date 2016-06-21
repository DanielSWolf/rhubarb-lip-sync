/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video/rtp_stream_receiver.h"

#include <vector>

#include "webrtc/base/logging.h"
#include "webrtc/common_types.h"
#include "webrtc/config.h"
#include "webrtc/modules/pacing/packet_router.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/rtp_rtcp/include/fec_receiver.h"
#include "webrtc/modules/rtp_rtcp/include/receive_statistics.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_cvo.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_header_parser.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_receiver.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/video_coding/video_coding_impl.h"
#include "webrtc/system_wrappers/include/metrics.h"
#include "webrtc/system_wrappers/include/timestamp_extrapolator.h"
#include "webrtc/system_wrappers/include/trace.h"
#include "webrtc/video/receive_statistics_proxy.h"
#include "webrtc/video/vie_remb.h"

namespace webrtc {

std::unique_ptr<RtpRtcp> CreateRtpRtcpModule(
    ReceiveStatistics* receive_statistics,
    Transport* outgoing_transport,
    RtcpRttStats* rtt_stats,
    RtcpPacketTypeCounterObserver* rtcp_packet_type_counter_observer,
    RemoteBitrateEstimator* remote_bitrate_estimator,
    RtpPacketSender* paced_sender,
    TransportSequenceNumberAllocator* transport_sequence_number_allocator) {
  RtpRtcp::Configuration configuration;
  configuration.audio = false;
  configuration.receiver_only = true;
  configuration.receive_statistics = receive_statistics;
  configuration.outgoing_transport = outgoing_transport;
  configuration.intra_frame_callback = nullptr;
  configuration.rtt_stats = rtt_stats;
  configuration.rtcp_packet_type_counter_observer =
      rtcp_packet_type_counter_observer;
  configuration.paced_sender = paced_sender;
  configuration.transport_sequence_number_allocator =
      transport_sequence_number_allocator;
  configuration.send_bitrate_observer = nullptr;
  configuration.send_frame_count_observer = nullptr;
  configuration.send_side_delay_observer = nullptr;
  configuration.send_packet_observer = nullptr;
  configuration.bandwidth_callback = nullptr;
  configuration.transport_feedback_callback = nullptr;

  std::unique_ptr<RtpRtcp> rtp_rtcp(RtpRtcp::CreateRtpRtcp(configuration));
  rtp_rtcp->SetSendingStatus(false);
  rtp_rtcp->SetSendingMediaStatus(false);
  rtp_rtcp->SetRTCPStatus(RtcpMode::kCompound);

  return rtp_rtcp;
}

static const int kPacketLogIntervalMs = 10000;

RtpStreamReceiver::RtpStreamReceiver(
    vcm::VideoReceiver* video_receiver,
    RemoteBitrateEstimator* remote_bitrate_estimator,
    Transport* transport,
    RtcpRttStats* rtt_stats,
    PacedSender* paced_sender,
    PacketRouter* packet_router,
    VieRemb* remb,
    const VideoReceiveStream::Config* config,
    ReceiveStatisticsProxy* receive_stats_proxy,
    ProcessThread* process_thread)
    : clock_(Clock::GetRealTimeClock()),
      config_(*config),
      video_receiver_(video_receiver),
      remote_bitrate_estimator_(remote_bitrate_estimator),
      packet_router_(packet_router),
      remb_(remb),
      process_thread_(process_thread),
      ntp_estimator_(clock_),
      rtp_payload_registry_(RTPPayloadStrategy::CreateStrategy(false)),
      rtp_header_parser_(RtpHeaderParser::Create()),
      rtp_receiver_(RtpReceiver::CreateVideoReceiver(clock_,
                                                     this,
                                                     this,
                                                     &rtp_payload_registry_)),
      rtp_receive_statistics_(ReceiveStatistics::Create(clock_)),
      fec_receiver_(FecReceiver::Create(this)),
      receiving_(false),
      restored_packet_in_use_(false),
      last_packet_log_ms_(-1),
      rtp_rtcp_(CreateRtpRtcpModule(rtp_receive_statistics_.get(),
                                    transport,
                                    rtt_stats,
                                    receive_stats_proxy,
                                    remote_bitrate_estimator_,
                                    paced_sender,
                                    packet_router)) {
  packet_router_->AddRtpModule(rtp_rtcp_.get());
  rtp_receive_statistics_->RegisterRtpStatisticsCallback(receive_stats_proxy);
  rtp_receive_statistics_->RegisterRtcpStatisticsCallback(receive_stats_proxy);

  RTC_DCHECK(config_.rtp.rtcp_mode != RtcpMode::kOff)
      << "A stream should not be configured with RTCP disabled. This value is "
         "reserved for internal usage.";
  RTC_DCHECK(config_.rtp.remote_ssrc != 0);
  // TODO(pbos): What's an appropriate local_ssrc for receive-only streams?
  RTC_DCHECK(config_.rtp.local_ssrc != 0);
  RTC_DCHECK(config_.rtp.remote_ssrc != config_.rtp.local_ssrc);

  rtp_rtcp_->SetRTCPStatus(config_.rtp.rtcp_mode);
  rtp_rtcp_->SetSSRC(config_.rtp.local_ssrc);
  rtp_rtcp_->SetKeyFrameRequestMethod(kKeyFrameReqPliRtcp);
  if (config_.rtp.remb) {
    rtp_rtcp_->SetREMBStatus(true);
    remb_->AddReceiveChannel(rtp_rtcp_.get());
  }

  for (size_t i = 0; i < config_.rtp.extensions.size(); ++i) {
    EnableReceiveRtpHeaderExtension(config_.rtp.extensions[i].uri,
                                    config_.rtp.extensions[i].id);
  }

  static const int kMaxPacketAgeToNack = 450;
  const int max_reordering_threshold = (config_.rtp.nack.rtp_history_ms > 0)
                                           ? kMaxPacketAgeToNack
                                           : kDefaultMaxReorderingThreshold;
  rtp_receive_statistics_->SetMaxReorderingThreshold(max_reordering_threshold);

  // TODO(pbos): Support multiple RTX, per video payload.
  for (const auto& kv : config_.rtp.rtx) {
    RTC_DCHECK(kv.second.ssrc != 0);
    RTC_DCHECK(kv.second.payload_type != 0);

    rtp_payload_registry_.SetRtxSsrc(kv.second.ssrc);
    rtp_payload_registry_.SetRtxPayloadType(kv.second.payload_type,
                                            kv.first);
  }

  // If set to true, the RTX payload type mapping supplied in
  // |SetRtxPayloadType| will be used when restoring RTX packets. Without it,
  // RTX packets will always be restored to the last non-RTX packet payload type
  // received.
  // TODO(holmer): When Chrome no longer depends on this being false by default,
  // always use the mapping and remove this whole codepath.
  rtp_payload_registry_.set_use_rtx_payload_mapping_on_restore(
      config_.rtp.use_rtx_payload_mapping_on_restore);

  if (IsFecEnabled()) {
    VideoCodec ulpfec_codec = {};
    ulpfec_codec.codecType = kVideoCodecULPFEC;
    strncpy(ulpfec_codec.plName, "ulpfec", sizeof(ulpfec_codec.plName));
    ulpfec_codec.plType = config_.rtp.fec.ulpfec_payload_type;
    RTC_CHECK(SetReceiveCodec(ulpfec_codec));

    VideoCodec red_codec = {};
    red_codec.codecType = kVideoCodecRED;
    strncpy(red_codec.plName, "red", sizeof(red_codec.plName));
    red_codec.plType = config_.rtp.fec.red_payload_type;
    RTC_CHECK(SetReceiveCodec(red_codec));
    if (config_.rtp.fec.red_rtx_payload_type != -1) {
      rtp_payload_registry_.SetRtxPayloadType(
          config_.rtp.fec.red_rtx_payload_type,
          config_.rtp.fec.red_payload_type);
    }

    rtp_rtcp_->SetGenericFECStatus(true,
                                   config_.rtp.fec.red_payload_type,
                                   config_.rtp.fec.ulpfec_payload_type);
  }

  if (config_.rtp.rtcp_xr.receiver_reference_time_report)
    rtp_rtcp_->SetRtcpXrRrtrStatus(true);

  // Stats callback for CNAME changes.
  rtp_rtcp_->RegisterRtcpStatisticsCallback(receive_stats_proxy);

  process_thread_->RegisterModule(rtp_receive_statistics_.get());
  process_thread_->RegisterModule(rtp_rtcp_.get());
}

RtpStreamReceiver::~RtpStreamReceiver() {
  process_thread_->DeRegisterModule(rtp_receive_statistics_.get());
  process_thread_->DeRegisterModule(rtp_rtcp_.get());

  packet_router_->RemoveRtpModule(rtp_rtcp_.get());
  rtp_rtcp_->SetREMBStatus(false);
  remb_->RemoveReceiveChannel(rtp_rtcp_.get());
  UpdateHistograms();
}

bool RtpStreamReceiver::SetReceiveCodec(const VideoCodec& video_codec) {
  int8_t old_pltype = -1;
  if (rtp_payload_registry_.ReceivePayloadType(
          video_codec.plName, kVideoPayloadTypeFrequency, 0,
          video_codec.maxBitrate, &old_pltype) != -1) {
    rtp_payload_registry_.DeRegisterReceivePayload(old_pltype);
  }

  return rtp_receiver_->RegisterReceivePayload(
             video_codec.plName, video_codec.plType, kVideoPayloadTypeFrequency,
             0, 0) == 0;
}

uint32_t RtpStreamReceiver::GetRemoteSsrc() const {
  return rtp_receiver_->SSRC();
}

int RtpStreamReceiver::GetCsrcs(uint32_t* csrcs) const {
  return rtp_receiver_->CSRCs(csrcs);
}

RtpReceiver* RtpStreamReceiver::GetRtpReceiver() const {
  return rtp_receiver_.get();
}

int32_t RtpStreamReceiver::OnReceivedPayloadData(
    const uint8_t* payload_data,
    size_t payload_size,
    const WebRtcRTPHeader* rtp_header) {
  RTC_DCHECK(video_receiver_);
  WebRtcRTPHeader rtp_header_with_ntp = *rtp_header;
  rtp_header_with_ntp.ntp_time_ms =
      ntp_estimator_.Estimate(rtp_header->header.timestamp);
  if (video_receiver_->IncomingPacket(payload_data, payload_size,
                                      rtp_header_with_ntp) != 0) {
    // Check this...
    return -1;
  }
  return 0;
}

bool RtpStreamReceiver::OnRecoveredPacket(const uint8_t* rtp_packet,
                                          size_t rtp_packet_length) {
  RTPHeader header;
  if (!rtp_header_parser_->Parse(rtp_packet, rtp_packet_length, &header)) {
    return false;
  }
  header.payload_type_frequency = kVideoPayloadTypeFrequency;
  bool in_order = IsPacketInOrder(header);
  return ReceivePacket(rtp_packet, rtp_packet_length, header, in_order);
}

// TODO(pbos): Remove as soon as audio can handle a changing payload type
// without this callback.
int32_t RtpStreamReceiver::OnInitializeDecoder(
    const int8_t payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const int frequency,
    const size_t channels,
    const uint32_t rate) {
  RTC_NOTREACHED();
  return 0;
}

void RtpStreamReceiver::OnIncomingSSRCChanged(const uint32_t ssrc) {
  rtp_rtcp_->SetRemoteSSRC(ssrc);
}

bool RtpStreamReceiver::DeliverRtp(const uint8_t* rtp_packet,
                                   size_t rtp_packet_length,
                                   const PacketTime& packet_time) {
  RTC_DCHECK(remote_bitrate_estimator_);
  {
    rtc::CritScope lock(&receive_cs_);
    if (!receiving_) {
      return false;
    }
  }

  RTPHeader header;
  if (!rtp_header_parser_->Parse(rtp_packet, rtp_packet_length,
                                 &header)) {
    return false;
  }
  size_t payload_length = rtp_packet_length - header.headerLength;
  int64_t arrival_time_ms;
  int64_t now_ms = clock_->TimeInMilliseconds();
  if (packet_time.timestamp != -1)
    arrival_time_ms = (packet_time.timestamp + 500) / 1000;
  else
    arrival_time_ms = now_ms;

  {
    // Periodically log the RTP header of incoming packets.
    rtc::CritScope lock(&receive_cs_);
    if (now_ms - last_packet_log_ms_ > kPacketLogIntervalMs) {
      std::stringstream ss;
      ss << "Packet received on SSRC: " << header.ssrc << " with payload type: "
         << static_cast<int>(header.payloadType) << ", timestamp: "
         << header.timestamp << ", sequence number: " << header.sequenceNumber
         << ", arrival time: " << arrival_time_ms;
      if (header.extension.hasTransmissionTimeOffset)
        ss << ", toffset: " << header.extension.transmissionTimeOffset;
      if (header.extension.hasAbsoluteSendTime)
        ss << ", abs send time: " << header.extension.absoluteSendTime;
      LOG(LS_INFO) << ss.str();
      last_packet_log_ms_ = now_ms;
    }
  }

  remote_bitrate_estimator_->IncomingPacket(arrival_time_ms, payload_length,
                                            header);
  header.payload_type_frequency = kVideoPayloadTypeFrequency;

  bool in_order = IsPacketInOrder(header);
  rtp_payload_registry_.SetIncomingPayloadType(header);
  bool ret = ReceivePacket(rtp_packet, rtp_packet_length, header, in_order);
  // Update receive statistics after ReceivePacket.
  // Receive statistics will be reset if the payload type changes (make sure
  // that the first packet is included in the stats).
  rtp_receive_statistics_->IncomingPacket(
      header, rtp_packet_length, IsPacketRetransmitted(header, in_order));
  return ret;
}

int32_t RtpStreamReceiver::RequestKeyFrame() {
  return rtp_rtcp_->RequestKeyFrame();
}

int32_t RtpStreamReceiver::SliceLossIndicationRequest(
    const uint64_t picture_id) {
  return rtp_rtcp_->SendRTCPSliceLossIndication(
      static_cast<uint8_t>(picture_id));
}

bool RtpStreamReceiver::IsFecEnabled() const {
  return config_.rtp.fec.red_payload_type != -1 &&
      config_.rtp.fec.ulpfec_payload_type != -1;
}

bool RtpStreamReceiver::IsRetransmissionsEnabled() const {
  return config_.rtp.nack.rtp_history_ms > 0;
}

void RtpStreamReceiver::RequestPacketRetransmit(
    const std::vector<uint16_t>& sequence_numbers) {
  rtp_rtcp_->SendNack(sequence_numbers);
}

int32_t RtpStreamReceiver::ResendPackets(const uint16_t* sequence_numbers,
                                         uint16_t length) {
  return rtp_rtcp_->SendNACK(sequence_numbers, length);
}

bool RtpStreamReceiver::ReceivePacket(const uint8_t* packet,
                                      size_t packet_length,
                                      const RTPHeader& header,
                                      bool in_order) {
  if (rtp_payload_registry_.IsEncapsulated(header)) {
    return ParseAndHandleEncapsulatingHeader(packet, packet_length, header);
  }
  const uint8_t* payload = packet + header.headerLength;
  assert(packet_length >= header.headerLength);
  size_t payload_length = packet_length - header.headerLength;
  PayloadUnion payload_specific;
  if (!rtp_payload_registry_.GetPayloadSpecifics(header.payloadType,
                                                 &payload_specific)) {
    return false;
  }
  return rtp_receiver_->IncomingRtpPacket(header, payload, payload_length,
                                          payload_specific, in_order);
}

bool RtpStreamReceiver::ParseAndHandleEncapsulatingHeader(
    const uint8_t* packet, size_t packet_length, const RTPHeader& header) {
  if (rtp_payload_registry_.IsRed(header)) {
    int8_t ulpfec_pt = rtp_payload_registry_.ulpfec_payload_type();
    if (packet[header.headerLength] == ulpfec_pt) {
      rtp_receive_statistics_->FecPacketReceived(header, packet_length);
      // Notify video_receiver about received FEC packets to avoid NACKing these
      // packets.
      NotifyReceiverOfFecPacket(header);
    }
    if (fec_receiver_->AddReceivedRedPacket(
            header, packet, packet_length, ulpfec_pt) != 0) {
      return false;
    }
    return fec_receiver_->ProcessReceivedFec() == 0;
  } else if (rtp_payload_registry_.IsRtx(header)) {
    if (header.headerLength + header.paddingLength == packet_length) {
      // This is an empty packet and should be silently dropped before trying to
      // parse the RTX header.
      return true;
    }
    // Remove the RTX header and parse the original RTP header.
    if (packet_length < header.headerLength)
      return false;
    if (packet_length > sizeof(restored_packet_))
      return false;
    rtc::CritScope lock(&receive_cs_);
    if (restored_packet_in_use_) {
      LOG(LS_WARNING) << "Multiple RTX headers detected, dropping packet.";
      return false;
    }
    if (!rtp_payload_registry_.RestoreOriginalPacket(
            restored_packet_, packet, &packet_length, rtp_receiver_->SSRC(),
            header)) {
      LOG(LS_WARNING) << "Incoming RTX packet: Invalid RTP header ssrc: "
                      << header.ssrc << " payload type: "
                      << static_cast<int>(header.payloadType);
      return false;
    }
    restored_packet_in_use_ = true;
    bool ret = OnRecoveredPacket(restored_packet_, packet_length);
    restored_packet_in_use_ = false;
    return ret;
  }
  return false;
}

void RtpStreamReceiver::NotifyReceiverOfFecPacket(const RTPHeader& header) {
  int8_t last_media_payload_type =
      rtp_payload_registry_.last_received_media_payload_type();
  if (last_media_payload_type < 0) {
    LOG(LS_WARNING) << "Failed to get last media payload type.";
    return;
  }
  // Fake an empty media packet.
  WebRtcRTPHeader rtp_header = {};
  rtp_header.header = header;
  rtp_header.header.payloadType = last_media_payload_type;
  rtp_header.header.paddingLength = 0;
  PayloadUnion payload_specific;
  if (!rtp_payload_registry_.GetPayloadSpecifics(last_media_payload_type,
                                                 &payload_specific)) {
    LOG(LS_WARNING) << "Failed to get payload specifics.";
    return;
  }
  rtp_header.type.Video.codec = payload_specific.Video.videoCodecType;
  rtp_header.type.Video.rotation = kVideoRotation_0;
  if (header.extension.hasVideoRotation) {
    rtp_header.type.Video.rotation =
        ConvertCVOByteToVideoRotation(header.extension.videoRotation);
  }
  rtp_header.type.Video.playout_delay = header.extension.playout_delay;

  OnReceivedPayloadData(nullptr, 0, &rtp_header);
}

bool RtpStreamReceiver::DeliverRtcp(const uint8_t* rtcp_packet,
                                    size_t rtcp_packet_length) {
  {
    rtc::CritScope lock(&receive_cs_);
    if (!receiving_) {
      return false;
    }
  }

  rtp_rtcp_->IncomingRtcpPacket(rtcp_packet, rtcp_packet_length);

  int64_t rtt = 0;
  rtp_rtcp_->RTT(rtp_receiver_->SSRC(), &rtt, nullptr, nullptr, nullptr);
  if (rtt == 0) {
    // Waiting for valid rtt.
    return true;
  }
  uint32_t ntp_secs = 0;
  uint32_t ntp_frac = 0;
  uint32_t rtp_timestamp = 0;
  if (rtp_rtcp_->RemoteNTP(&ntp_secs, &ntp_frac, nullptr, nullptr,
                           &rtp_timestamp) != 0) {
    // Waiting for RTCP.
    return true;
  }
  ntp_estimator_.UpdateRtcpTimestamp(rtt, ntp_secs, ntp_frac, rtp_timestamp);

  return true;
}

void RtpStreamReceiver::SignalNetworkState(NetworkState state) {
  rtp_rtcp_->SetRTCPStatus(state == kNetworkUp ? config_.rtp.rtcp_mode
                                               : RtcpMode::kOff);
}

void RtpStreamReceiver::StartReceive() {
  rtc::CritScope lock(&receive_cs_);
  receiving_ = true;
}

void RtpStreamReceiver::StopReceive() {
  rtc::CritScope lock(&receive_cs_);
  receiving_ = false;
}

bool RtpStreamReceiver::IsPacketInOrder(const RTPHeader& header) const {
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  return statistician->IsPacketInOrder(header.sequenceNumber);
}

bool RtpStreamReceiver::IsPacketRetransmitted(const RTPHeader& header,
                                              bool in_order) const {
  // Retransmissions are handled separately if RTX is enabled.
  if (rtp_payload_registry_.RtxEnabled())
    return false;
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  // Check if this is a retransmission.
  int64_t min_rtt = 0;
  rtp_rtcp_->RTT(rtp_receiver_->SSRC(), nullptr, nullptr, &min_rtt, nullptr);
  return !in_order &&
      statistician->IsRetransmitOfOldPacket(header, min_rtt);
}

void RtpStreamReceiver::UpdateHistograms() {
  FecPacketCounter counter = fec_receiver_->GetPacketCounter();
  if (counter.num_packets > 0) {
    RTC_LOGGED_HISTOGRAM_PERCENTAGE(
        "WebRTC.Video.ReceivedFecPacketsInPercent",
        static_cast<int>(counter.num_fec_packets * 100 / counter.num_packets));
  }
  if (counter.num_fec_packets > 0) {
    RTC_LOGGED_HISTOGRAM_PERCENTAGE(
        "WebRTC.Video.RecoveredMediaPacketsInPercentOfFec",
        static_cast<int>(counter.num_recovered_packets * 100 /
                         counter.num_fec_packets));
  }
}

void RtpStreamReceiver::EnableReceiveRtpHeaderExtension(
    const std::string& extension, int id) {
  // One-byte-extension local identifiers are in the range 1-14 inclusive.
  RTC_DCHECK_GE(id, 1);
  RTC_DCHECK_LE(id, 14);
  RTC_DCHECK(RtpExtension::IsSupportedForVideo(extension));
  RTC_CHECK(rtp_header_parser_->RegisterRtpHeaderExtension(
      StringToRtpExtensionType(extension), id));
}

}  // namespace webrtc
