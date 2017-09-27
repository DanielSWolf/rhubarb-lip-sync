/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_

#include <memory>
#include <vector>

#include "webrtc/call.h"
#include "webrtc/call/transport_adapter.h"
#include "webrtc/common_video/include/incoming_video_stream.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_coding/video_coding_impl.h"
#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/video/receive_statistics_proxy.h"
#include "webrtc/video/rtp_stream_receiver.h"
#include "webrtc/video/video_stream_decoder.h"
#include "webrtc/video_receive_stream.h"

namespace webrtc {

class CallStats;
class CongestionController;
class IvfFileWriter;
class ProcessThread;
class VoiceEngine;
class VieRemb;

namespace internal {

class VideoReceiveStream : public webrtc::VideoReceiveStream,
                           public rtc::VideoSinkInterface<VideoFrame>,
                           public EncodedImageCallback,
                           public NackSender,
                           public KeyFrameRequestSender {
 public:
  VideoReceiveStream(int num_cpu_cores,
                     CongestionController* congestion_controller,
                     VideoReceiveStream::Config config,
                     webrtc::VoiceEngine* voice_engine,
                     ProcessThread* process_thread,
                     CallStats* call_stats,
                     VieRemb* remb);
  ~VideoReceiveStream() override;

  void SignalNetworkState(NetworkState state);
  bool DeliverRtcp(const uint8_t* packet, size_t length);
  bool DeliverRtp(const uint8_t* packet,
                  size_t length,
                  const PacketTime& packet_time);

  // webrtc::VideoReceiveStream implementation.
  void Start() override;
  void Stop() override;

  webrtc::VideoReceiveStream::Stats GetStats() const override;

  // Overrides rtc::VideoSinkInterface<VideoFrame>.
  void OnFrame(const VideoFrame& video_frame) override;

  // Overrides EncodedImageCallback.
  int32_t Encoded(const EncodedImage& encoded_image,
                  const CodecSpecificInfo* codec_specific_info,
                  const RTPFragmentationHeader* fragmentation) override;

  const Config& config() const { return config_; }

  void SetSyncChannel(VoiceEngine* voice_engine, int audio_channel_id);

  // Implements NackSender.
  void SendNack(const std::vector<uint16_t>& sequence_numbers) override;

  // Implements KeyFrameRequestSender.
  void RequestKeyFrame() override;

 private:
  static bool DecodeThreadFunction(void* ptr);
  void Decode();

  TransportAdapter transport_adapter_;
  const VideoReceiveStream::Config config_;
  ProcessThread* const process_thread_;
  Clock* const clock_;

  rtc::PlatformThread decode_thread_;

  CongestionController* const congestion_controller_;
  CallStats* const call_stats_;

  vcm::VideoReceiver video_receiver_;
  std::unique_ptr<rtc::VideoSinkInterface<VideoFrame>> incoming_video_stream_;
  ReceiveStatisticsProxy stats_proxy_;
  RtpStreamReceiver rtp_stream_receiver_;
  std::unique_ptr<VideoStreamDecoder> video_stream_decoder_;
  ViESyncModule vie_sync_;

  std::unique_ptr<IvfFileWriter> ivf_writer_;
};
}  // namespace internal
}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_
