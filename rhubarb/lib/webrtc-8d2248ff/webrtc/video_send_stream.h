/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_SEND_STREAM_H_

#include <map>
#include <string>

#include "webrtc/common_types.h"
#include "webrtc/common_video/include/frame_callback.h"
#include "webrtc/config.h"
#include "webrtc/media/base/videosinkinterface.h"
#include "webrtc/transport.h"
#include "webrtc/media/base/videosinkinterface.h"

namespace webrtc {

class LoadObserver;
class VideoEncoder;

// Class to deliver captured frame to the video send stream.
class VideoCaptureInput {
 public:
  // These methods do not lock internally and must be called sequentially.
  // If your application switches input sources synchronization must be done
  // externally to make sure that any old frames are not delivered concurrently.
  virtual void IncomingCapturedFrame(const VideoFrame& video_frame) = 0;

 protected:
  virtual ~VideoCaptureInput() {}
};

class VideoSendStream {
 public:
  struct StreamStats {
    FrameCounts frame_counts;
    int width = 0;
    int height = 0;
    // TODO(holmer): Move bitrate_bps out to the webrtc::Call layer.
    int total_bitrate_bps = 0;
    int retransmit_bitrate_bps = 0;
    int avg_delay_ms = 0;
    int max_delay_ms = 0;
    StreamDataCounters rtp_stats;
    RtcpPacketTypeCounter rtcp_packet_type_counts;
    RtcpStatistics rtcp_stats;
  };

  struct Stats {
    std::string encoder_implementation_name = "unknown";
    int input_frame_rate = 0;
    int encode_frame_rate = 0;
    int avg_encode_time_ms = 0;
    int encode_usage_percent = 0;
    int target_media_bitrate_bps = 0;
    int media_bitrate_bps = 0;
    bool suspended = false;
    bool bw_limited_resolution = false;
    std::map<uint32_t, StreamStats> substreams;
  };

  struct Config {
    Config() = delete;
    explicit Config(Transport* send_transport)
        : send_transport(send_transport) {}

    std::string ToString() const;

    struct EncoderSettings {
      std::string ToString() const;

      std::string payload_name;
      int payload_type = -1;

      // TODO(sophiechang): Delete this field when no one is using internal
      // sources anymore.
      bool internal_source = false;

      // Allow 100% encoder utilization. Used for HW encoders where CPU isn't
      // expected to be the limiting factor, but a chip could be running at
      // 30fps (for example) exactly.
      bool full_overuse_time = false;

      // Uninitialized VideoEncoder instance to be used for encoding. Will be
      // initialized from inside the VideoSendStream.
      VideoEncoder* encoder = nullptr;
    } encoder_settings;

    static const size_t kDefaultMaxPacketSize = 1500 - 40;  // TCP over IPv4.
    struct Rtp {
      std::string ToString() const;

      std::vector<uint32_t> ssrcs;

      // See RtcpMode for description.
      RtcpMode rtcp_mode = RtcpMode::kCompound;

      // Max RTP packet size delivered to send transport from VideoEngine.
      size_t max_packet_size = kDefaultMaxPacketSize;

      // RTP header extensions to use for this send stream.
      std::vector<RtpExtension> extensions;

      // See NackConfig for description.
      NackConfig nack;

      // See FecConfig for description.
      FecConfig fec;

      // Settings for RTP retransmission payload format, see RFC 4588 for
      // details.
      struct Rtx {
        std::string ToString() const;
        // SSRCs to use for the RTX streams.
        std::vector<uint32_t> ssrcs;

        // Payload type to use for the RTX stream.
        int payload_type = -1;
      } rtx;

      // RTCP CNAME, see RFC 3550.
      std::string c_name;
    } rtp;

    // Transport for outgoing packets.
    Transport* send_transport = nullptr;

    // Callback for overuse and normal usage based on the jitter of incoming
    // captured frames. 'nullptr' disables the callback.
    LoadObserver* overuse_callback = nullptr;

    // Called for each I420 frame before encoding the frame. Can be used for
    // effects, snapshots etc. 'nullptr' disables the callback.
    rtc::VideoSinkInterface<VideoFrame>* pre_encode_callback = nullptr;

    // Called for each encoded frame, e.g. used for file storage. 'nullptr'
    // disables the callback. Also measures timing and passes the time
    // spent on encoding. This timing will not fire if encoding takes longer
    // than the measuring window, since the sample data will have been dropped.
    EncodedFrameObserver* post_encode_callback = nullptr;

    // Renderer for local preview. The local renderer will be called even if
    // sending hasn't started. 'nullptr' disables local rendering.
    rtc::VideoSinkInterface<VideoFrame>* local_renderer = nullptr;

    // Expected delay needed by the renderer, i.e. the frame will be delivered
    // this many milliseconds, if possible, earlier than expected render time.
    // Only valid if |local_renderer| is set.
    int render_delay_ms = 0;

    // Target delay in milliseconds. A positive value indicates this stream is
    // used for streaming instead of a real-time call.
    int target_delay_ms = 0;

    // True if the stream should be suspended when the available bitrate fall
    // below the minimum configured bitrate. If this variable is false, the
    // stream may send at a rate higher than the estimated available bitrate.
    bool suspend_below_min_bitrate = false;
  };

  // Starts stream activity.
  // When a stream is active, it can receive, process and deliver packets.
  virtual void Start() = 0;
  // Stops stream activity.
  // When a stream is stopped, it can't receive, process or deliver packets.
  virtual void Stop() = 0;

  // Gets interface used to insert captured frames. Valid as long as the
  // VideoSendStream is valid.
  virtual VideoCaptureInput* Input() = 0;

  // Set which streams to send. Must have at least as many SSRCs as configured
  // in the config. Encoder settings are passed on to the encoder instance along
  // with the VideoStream settings.
  virtual void ReconfigureVideoEncoder(const VideoEncoderConfig& config) = 0;

  virtual Stats GetStats() = 0;

 protected:
  virtual ~VideoSendStream() {}
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_SEND_STREAM_H_
