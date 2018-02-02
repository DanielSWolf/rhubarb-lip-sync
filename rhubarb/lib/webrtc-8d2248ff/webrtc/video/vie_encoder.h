/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_VIE_ENCODER_H_
#define WEBRTC_VIDEO_VIE_ENCODER_H_

#include <memory>
#include <string>
#include <vector>

#include "webrtc/base/criticalsection.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/video_encoder.h"
#include "webrtc/media/base/videosinkinterface.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/modules/video_coding/include/video_coding_defines.h"
#include "webrtc/modules/video_coding/video_coding_impl.h"
#include "webrtc/modules/video_processing/include/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Config;
class EncodedImageCallback;
class OveruseFrameDetector;
class PacedSender;
class ProcessThread;
class SendStatisticsProxy;
class ViEBitrateObserver;
class ViEEffectFilter;
class VideoEncoder;

// VieEncoder represent a video encoder that accepts raw video frames as input
// and produces an encoded bit stream.
// Usage:
// 1. Instantiate
// 2. Call Init
// 3. Call RegisterExternalEncoder if available.
// 4. Call SetEncoder with the codec settings and the object that shall receive
//    the encoded bit stream.
// 5. For each available raw video frame call EncodeVideoFrame.
class ViEEncoder : public VideoEncoderRateObserver,
                   public EncodedImageCallback,
                   public VCMSendStatisticsCallback {
 public:
  friend class ViEBitrateObserver;

  ViEEncoder(uint32_t number_of_cores,
             ProcessThread* module_process_thread,
             SendStatisticsProxy* stats_proxy,
             OveruseFrameDetector* overuse_detector,
             EncodedImageCallback* sink);
  ~ViEEncoder();

  vcm::VideoSender* video_sender();

  // Returns the id of the owning channel.
  int Owner() const;

  // Codec settings.
  int32_t RegisterExternalEncoder(VideoEncoder* encoder,
                                  uint8_t pl_type,
                                  bool internal_source);
  int32_t DeRegisterExternalEncoder(uint8_t pl_type);
  void SetEncoder(const VideoCodec& video_codec,
                  size_t max_data_payload_length);

  void EncodeVideoFrame(const VideoFrame& video_frame);
  void SendKeyFrame();

  // Returns the time when the encoder last received an input frame or produced
  // an encoded frame.
  int64_t time_of_last_frame_activity_ms();

  // Implements VideoEncoderRateObserver.
  // TODO(perkj): Refactor VideoEncoderRateObserver. This is only used for
  // stats. The stats should be set in VideoSendStream instead.
  // |bitrate_bps| is the target bitrate and |framerate| is the input frame
  // rate so it has nothing to do with the actual encoder.
  void OnSetRates(uint32_t bitrate_bps, int framerate) override;

  // Implements EncodedImageCallback.
  int32_t Encoded(const EncodedImage& encoded_image,
                  const CodecSpecificInfo* codec_specific_info,
                  const RTPFragmentationHeader* fragmentation) override;

  // Implements VideoSendStatisticsCallback.
  void SendStatistics(uint32_t bit_rate,
                      uint32_t frame_rate,
                      const std::string& encoder_name) override;

  // virtual to test EncoderStateFeedback with mocks.
  virtual void OnReceivedIntraFrameRequest(size_t stream_index);
  virtual void OnReceivedSLI(uint8_t picture_id);
  virtual void OnReceivedRPSI(uint64_t picture_id);

  void OnBitrateUpdated(uint32_t bitrate_bps,
                        uint8_t fraction_lost,
                        int64_t round_trip_time_ms);

 private:
  bool EncoderPaused() const EXCLUSIVE_LOCKS_REQUIRED(data_cs_);
  void TraceFrameDropStart() EXCLUSIVE_LOCKS_REQUIRED(data_cs_);
  void TraceFrameDropEnd() EXCLUSIVE_LOCKS_REQUIRED(data_cs_);

  const uint32_t number_of_cores_;
  EncodedImageCallback* const sink_;

  const std::unique_ptr<VideoProcessing> vp_;
  vcm::VideoSender video_sender_;

  rtc::CriticalSection data_cs_;

  SendStatisticsProxy* const stats_proxy_;
  OveruseFrameDetector* const overuse_detector_;

  // The time we last received an input frame or encoded frame. This is used to
  // track when video is stopped long enough that we also want to stop sending
  // padding.
  int64_t time_of_last_frame_activity_ms_ GUARDED_BY(data_cs_);
  VideoCodec encoder_config_ GUARDED_BY(data_cs_);
  uint32_t last_observed_bitrate_bps_ GUARDED_BY(data_cs_);
  bool encoder_paused_and_dropped_frame_ GUARDED_BY(data_cs_);

  ProcessThread* module_process_thread_;

  bool has_received_sli_ GUARDED_BY(data_cs_);
  uint8_t picture_id_sli_ GUARDED_BY(data_cs_);
  bool has_received_rpsi_ GUARDED_BY(data_cs_);
  uint64_t picture_id_rpsi_ GUARDED_BY(data_cs_);

  bool video_suspended_ GUARDED_BY(data_cs_);
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIE_ENCODER_H_
