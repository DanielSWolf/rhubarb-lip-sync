/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/engine/webrtcvideoengine2.h"

#include <stdio.h>
#include <algorithm>
#include <set>
#include <string>

#include "webrtc/base/copyonwritebuffer.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/trace_event.h"
#include "webrtc/call.h"
#include "webrtc/media/engine/constants.h"
#include "webrtc/media/engine/simulcast.h"
#include "webrtc/media/engine/webrtcmediaengine.h"
#include "webrtc/media/engine/webrtcvideoencoderfactory.h"
#include "webrtc/media/engine/webrtcvideoframe.h"
#include "webrtc/media/engine/webrtcvoiceengine.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#include "webrtc/modules/video_coding/codecs/vp8/simulcast_encoder_adapter.h"
#include "webrtc/modules/video_coding/codecs/vp9/include/vp9.h"
#include "webrtc/system_wrappers/include/field_trial.h"
#include "webrtc/video_decoder.h"
#include "webrtc/video_encoder.h"

namespace cricket {
namespace {

// Wrap cricket::WebRtcVideoEncoderFactory as a webrtc::VideoEncoderFactory.
class EncoderFactoryAdapter : public webrtc::VideoEncoderFactory {
 public:
  // EncoderFactoryAdapter doesn't take ownership of |factory|, which is owned
  // by e.g. PeerConnectionFactory.
  explicit EncoderFactoryAdapter(cricket::WebRtcVideoEncoderFactory* factory)
      : factory_(factory) {}
  virtual ~EncoderFactoryAdapter() {}

  // Implement webrtc::VideoEncoderFactory.
  webrtc::VideoEncoder* Create() override {
    return factory_->CreateVideoEncoder(webrtc::kVideoCodecVP8);
  }

  void Destroy(webrtc::VideoEncoder* encoder) override {
    return factory_->DestroyVideoEncoder(encoder);
  }

 private:
  cricket::WebRtcVideoEncoderFactory* const factory_;
};

webrtc::Call::Config::BitrateConfig GetBitrateConfigForCodec(
    const VideoCodec& codec) {
  webrtc::Call::Config::BitrateConfig config;
  int bitrate_kbps;
  if (codec.GetParam(kCodecParamMinBitrate, &bitrate_kbps) &&
      bitrate_kbps > 0) {
    config.min_bitrate_bps = bitrate_kbps * 1000;
  } else {
    config.min_bitrate_bps = 0;
  }
  if (codec.GetParam(kCodecParamStartBitrate, &bitrate_kbps) &&
      bitrate_kbps > 0) {
    config.start_bitrate_bps = bitrate_kbps * 1000;
  } else {
    // Do not reconfigure start bitrate unless it's specified and positive.
    config.start_bitrate_bps = -1;
  }
  if (codec.GetParam(kCodecParamMaxBitrate, &bitrate_kbps) &&
      bitrate_kbps > 0) {
    config.max_bitrate_bps = bitrate_kbps * 1000;
  } else {
    config.max_bitrate_bps = -1;
  }
  return config;
}

// An encoder factory that wraps Create requests for simulcastable codec types
// with a webrtc::SimulcastEncoderAdapter. Non simulcastable codec type
// requests are just passed through to the contained encoder factory.
class WebRtcSimulcastEncoderFactory
    : public cricket::WebRtcVideoEncoderFactory {
 public:
  // WebRtcSimulcastEncoderFactory doesn't take ownership of |factory|, which is
  // owned by e.g. PeerConnectionFactory.
  explicit WebRtcSimulcastEncoderFactory(
      cricket::WebRtcVideoEncoderFactory* factory)
      : factory_(factory) {}

  static bool UseSimulcastEncoderFactory(
      const std::vector<VideoCodec>& codecs) {
    // If any codec is VP8, use the simulcast factory. If asked to create a
    // non-VP8 codec, we'll just return a contained factory encoder directly.
    for (const auto& codec : codecs) {
      if (codec.type == webrtc::kVideoCodecVP8) {
        return true;
      }
    }
    return false;
  }

  webrtc::VideoEncoder* CreateVideoEncoder(
      webrtc::VideoCodecType type) override {
    RTC_DCHECK(factory_ != NULL);
    // If it's a codec type we can simulcast, create a wrapped encoder.
    if (type == webrtc::kVideoCodecVP8) {
      return new webrtc::SimulcastEncoderAdapter(
          new EncoderFactoryAdapter(factory_));
    }
    webrtc::VideoEncoder* encoder = factory_->CreateVideoEncoder(type);
    if (encoder) {
      non_simulcast_encoders_.push_back(encoder);
    }
    return encoder;
  }

  const std::vector<VideoCodec>& codecs() const override {
    return factory_->codecs();
  }

  bool EncoderTypeHasInternalSource(
      webrtc::VideoCodecType type) const override {
    return factory_->EncoderTypeHasInternalSource(type);
  }

  void DestroyVideoEncoder(webrtc::VideoEncoder* encoder) override {
    // Check first to see if the encoder wasn't wrapped in a
    // SimulcastEncoderAdapter. In that case, ask the factory to destroy it.
    if (std::remove(non_simulcast_encoders_.begin(),
                    non_simulcast_encoders_.end(),
                    encoder) != non_simulcast_encoders_.end()) {
      factory_->DestroyVideoEncoder(encoder);
      return;
    }

    // Otherwise, SimulcastEncoderAdapter can be deleted directly, and will call
    // DestroyVideoEncoder on the factory for individual encoder instances.
    delete encoder;
  }

 private:
  cricket::WebRtcVideoEncoderFactory* factory_;
  // A list of encoders that were created without being wrapped in a
  // SimulcastEncoderAdapter.
  std::vector<webrtc::VideoEncoder*> non_simulcast_encoders_;
};

bool CodecIsInternallySupported(const std::string& codec_name) {
  if (CodecNamesEq(codec_name, kVp8CodecName)) {
    return true;
  }
  if (CodecNamesEq(codec_name, kVp9CodecName)) {
    return webrtc::VP9Encoder::IsSupported() &&
           webrtc::VP9Decoder::IsSupported();
  }
  if (CodecNamesEq(codec_name, kH264CodecName)) {
    return webrtc::H264Encoder::IsSupported() &&
        webrtc::H264Decoder::IsSupported();
  }
  return false;
}

void AddDefaultFeedbackParams(VideoCodec* codec) {
  codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamCcm, kRtcpFbCcmParamFir));
  codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamNack, kParamValueEmpty));
  codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamNack, kRtcpFbNackParamPli));
  codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamRemb, kParamValueEmpty));
  codec->AddFeedbackParam(
      FeedbackParam(kRtcpFbParamTransportCc, kParamValueEmpty));
}

static VideoCodec MakeVideoCodecWithDefaultFeedbackParams(int payload_type,
                                                          const char* name) {
  VideoCodec codec(payload_type, name, kDefaultVideoMaxWidth,
                   kDefaultVideoMaxHeight, kDefaultVideoMaxFramerate);
  AddDefaultFeedbackParams(&codec);
  return codec;
}

static std::string CodecVectorToString(const std::vector<VideoCodec>& codecs) {
  std::stringstream out;
  out << '{';
  for (size_t i = 0; i < codecs.size(); ++i) {
    out << codecs[i].ToString();
    if (i != codecs.size() - 1) {
      out << ", ";
    }
  }
  out << '}';
  return out.str();
}

static bool ValidateCodecFormats(const std::vector<VideoCodec>& codecs) {
  bool has_video = false;
  for (size_t i = 0; i < codecs.size(); ++i) {
    if (!codecs[i].ValidateCodecFormat()) {
      return false;
    }
    if (codecs[i].GetCodecType() == VideoCodec::CODEC_VIDEO) {
      has_video = true;
    }
  }
  if (!has_video) {
    LOG(LS_ERROR) << "Setting codecs without a video codec is invalid: "
                  << CodecVectorToString(codecs);
    return false;
  }
  return true;
}

static bool ValidateStreamParams(const StreamParams& sp) {
  if (sp.ssrcs.empty()) {
    LOG(LS_ERROR) << "No SSRCs in stream parameters: " << sp.ToString();
    return false;
  }

  std::vector<uint32_t> primary_ssrcs;
  sp.GetPrimarySsrcs(&primary_ssrcs);
  std::vector<uint32_t> rtx_ssrcs;
  sp.GetFidSsrcs(primary_ssrcs, &rtx_ssrcs);
  for (uint32_t rtx_ssrc : rtx_ssrcs) {
    bool rtx_ssrc_present = false;
    for (uint32_t sp_ssrc : sp.ssrcs) {
      if (sp_ssrc == rtx_ssrc) {
        rtx_ssrc_present = true;
        break;
      }
    }
    if (!rtx_ssrc_present) {
      LOG(LS_ERROR) << "RTX SSRC '" << rtx_ssrc
                    << "' missing from StreamParams ssrcs: " << sp.ToString();
      return false;
    }
  }
  if (!rtx_ssrcs.empty() && primary_ssrcs.size() != rtx_ssrcs.size()) {
    LOG(LS_ERROR)
        << "RTX SSRCs exist, but don't cover all SSRCs (unsupported): "
        << sp.ToString();
    return false;
  }

  return true;
}

inline bool ContainsHeaderExtension(
    const std::vector<webrtc::RtpExtension>& extensions,
    const std::string& uri) {
  for (const auto& kv : extensions) {
    if (kv.uri == uri) {
      return true;
    }
  }
  return false;
}

// Returns true if the given codec is disallowed from doing simulcast.
bool IsCodecBlacklistedForSimulcast(const std::string& codec_name) {
  return CodecNamesEq(codec_name, kH264CodecName) ||
         CodecNamesEq(codec_name, kVp9CodecName);
}

// The selected thresholds for QVGA and VGA corresponded to a QP around 10.
// The change in QP declined above the selected bitrates.
static int GetMaxDefaultVideoBitrateKbps(int width, int height) {
  if (width * height <= 320 * 240) {
    return 600;
  } else if (width * height <= 640 * 480) {
    return 1700;
  } else if (width * height <= 960 * 540) {
    return 2000;
  } else {
    return 2500;
  }
}

bool GetVp9LayersFromFieldTrialGroup(int* num_spatial_layers,
                                     int* num_temporal_layers) {
  std::string group = webrtc::field_trial::FindFullName("WebRTC-SupportVP9SVC");
  if (group.empty())
    return false;

  if (sscanf(group.c_str(), "EnabledByFlag_%dSL%dTL", num_spatial_layers,
             num_temporal_layers) != 2) {
    return false;
  }
  const int kMaxSpatialLayers = 2;
  if (*num_spatial_layers > kMaxSpatialLayers || *num_spatial_layers < 1)
    return false;

  const int kMaxTemporalLayers = 3;
  if (*num_temporal_layers > kMaxTemporalLayers || *num_temporal_layers < 1)
    return false;

  return true;
}

int GetDefaultVp9SpatialLayers() {
  int num_sl;
  int num_tl;
  if (GetVp9LayersFromFieldTrialGroup(&num_sl, &num_tl)) {
    return num_sl;
  }
  return 1;
}

int GetDefaultVp9TemporalLayers() {
  int num_sl;
  int num_tl;
  if (GetVp9LayersFromFieldTrialGroup(&num_sl, &num_tl)) {
    return num_tl;
  }
  return 1;
}
}  // namespace

// Constants defined in webrtc/media/engine/constants.h
// TODO(pbos): Move these to a separate constants.cc file.
const int kMinVideoBitrate = 30;
const int kStartVideoBitrate = 300;

const int kVideoMtu = 1200;
const int kVideoRtpBufferSize = 65536;

// This constant is really an on/off, lower-level configurable NACK history
// duration hasn't been implemented.
static const int kNackHistoryMs = 1000;

static const int kDefaultQpMax = 56;

static const int kDefaultRtcpReceiverReportSsrc = 1;

// Down grade resolution at most 2 times for CPU reasons.
static const int kMaxCpuDowngrades = 2;

std::vector<VideoCodec> DefaultVideoCodecList() {
  std::vector<VideoCodec> codecs;
  codecs.push_back(MakeVideoCodecWithDefaultFeedbackParams(kDefaultVp8PlType,
                                                           kVp8CodecName));
  codecs.push_back(
      VideoCodec::CreateRtxCodec(kDefaultRtxVp8PlType, kDefaultVp8PlType));
  if (CodecIsInternallySupported(kVp9CodecName)) {
    codecs.push_back(MakeVideoCodecWithDefaultFeedbackParams(kDefaultVp9PlType,
                                                             kVp9CodecName));
    codecs.push_back(
        VideoCodec::CreateRtxCodec(kDefaultRtxVp9PlType, kDefaultVp9PlType));
  }
  if (CodecIsInternallySupported(kH264CodecName)) {
    VideoCodec codec = MakeVideoCodecWithDefaultFeedbackParams(
        kDefaultH264PlType, kH264CodecName);
    // TODO(hta): Move all parameter generation for SDP into the codec
    // implementation, for all codecs and parameters.
    // TODO(hta): Move selection of profile-level-id to H.264 codec
    // implementation.
    // TODO(hta): Set FMTP parameters for all codecs of type H264.
    codec.SetParam(kH264FmtpProfileLevelId,
                   kH264ProfileLevelConstrainedBaseline);
    codec.SetParam(kH264FmtpLevelAsymmetryAllowed, "1");
    codec.SetParam(kH264FmtpPacketizationMode, "1");
    codecs.push_back(codec);
    codecs.push_back(
        VideoCodec::CreateRtxCodec(kDefaultRtxH264PlType, kDefaultH264PlType));
  }
  codecs.push_back(VideoCodec(kDefaultRedPlType, kRedCodecName));
  codecs.push_back(
      VideoCodec::CreateRtxCodec(kDefaultRtxRedPlType, kDefaultRedPlType));
  codecs.push_back(VideoCodec(kDefaultUlpfecType, kUlpfecCodecName));
  return codecs;
}

std::vector<webrtc::VideoStream>
WebRtcVideoChannel2::WebRtcVideoSendStream::CreateSimulcastVideoStreams(
    const VideoCodec& codec,
    const VideoOptions& options,
    int max_bitrate_bps,
    size_t num_streams) {
  int max_qp = kDefaultQpMax;
  codec.GetParam(kCodecParamMaxQuantization, &max_qp);

  return GetSimulcastConfig(
      num_streams, codec.width, codec.height, max_bitrate_bps, max_qp,
      codec.framerate != 0 ? codec.framerate : kDefaultVideoMaxFramerate);
}

std::vector<webrtc::VideoStream>
WebRtcVideoChannel2::WebRtcVideoSendStream::CreateVideoStreams(
    const VideoCodec& codec,
    const VideoOptions& options,
    int max_bitrate_bps,
    size_t num_streams) {
  int codec_max_bitrate_kbps;
  if (codec.GetParam(kCodecParamMaxBitrate, &codec_max_bitrate_kbps)) {
    max_bitrate_bps = codec_max_bitrate_kbps * 1000;
  }
  if (num_streams != 1) {
    return CreateSimulcastVideoStreams(codec, options, max_bitrate_bps,
                                       num_streams);
  }

  // For unset max bitrates set default bitrate for non-simulcast.
  if (max_bitrate_bps <= 0) {
    max_bitrate_bps =
        GetMaxDefaultVideoBitrateKbps(codec.width, codec.height) * 1000;
  }

  webrtc::VideoStream stream;
  stream.width = codec.width;
  stream.height = codec.height;
  stream.max_framerate =
      codec.framerate != 0 ? codec.framerate : kDefaultVideoMaxFramerate;

  stream.min_bitrate_bps = kMinVideoBitrate * 1000;
  stream.target_bitrate_bps = stream.max_bitrate_bps = max_bitrate_bps;

  int max_qp = kDefaultQpMax;
  codec.GetParam(kCodecParamMaxQuantization, &max_qp);
  stream.max_qp = max_qp;
  std::vector<webrtc::VideoStream> streams;
  streams.push_back(stream);
  return streams;
}

void* WebRtcVideoChannel2::WebRtcVideoSendStream::ConfigureVideoEncoderSettings(
    const VideoCodec& codec) {
  bool is_screencast = parameters_.options.is_screencast.value_or(false);
  // No automatic resizing when using simulcast or screencast.
  bool automatic_resize =
      !is_screencast && parameters_.config.rtp.ssrcs.size() == 1;
  bool frame_dropping = !is_screencast;
  bool denoising;
  bool codec_default_denoising = false;
  if (is_screencast) {
    denoising = false;
  } else {
    // Use codec default if video_noise_reduction is unset.
    codec_default_denoising = !parameters_.options.video_noise_reduction;
    denoising = parameters_.options.video_noise_reduction.value_or(false);
  }

  if (CodecNamesEq(codec.name, kH264CodecName)) {
    encoder_settings_.h264 = webrtc::VideoEncoder::GetDefaultH264Settings();
    encoder_settings_.h264.frameDroppingOn = frame_dropping;
    return &encoder_settings_.h264;
  }
  if (CodecNamesEq(codec.name, kVp8CodecName)) {
    encoder_settings_.vp8 = webrtc::VideoEncoder::GetDefaultVp8Settings();
    encoder_settings_.vp8.automaticResizeOn = automatic_resize;
    // VP8 denoising is enabled by default.
    encoder_settings_.vp8.denoisingOn =
        codec_default_denoising ? true : denoising;
    encoder_settings_.vp8.frameDroppingOn = frame_dropping;
    return &encoder_settings_.vp8;
  }
  if (CodecNamesEq(codec.name, kVp9CodecName)) {
    encoder_settings_.vp9 = webrtc::VideoEncoder::GetDefaultVp9Settings();
    if (is_screencast) {
      // TODO(asapersson): Set to 2 for now since there is a DCHECK in
      // VideoSendStream::ReconfigureVideoEncoder.
      encoder_settings_.vp9.numberOfSpatialLayers = 2;
    } else {
      encoder_settings_.vp9.numberOfSpatialLayers =
          GetDefaultVp9SpatialLayers();
    }
    // VP9 denoising is disabled by default.
    encoder_settings_.vp9.denoisingOn =
        codec_default_denoising ? false : denoising;
    encoder_settings_.vp9.frameDroppingOn = frame_dropping;
    return &encoder_settings_.vp9;
  }
  return NULL;
}

DefaultUnsignalledSsrcHandler::DefaultUnsignalledSsrcHandler()
    : default_recv_ssrc_(0), default_sink_(NULL) {}

UnsignalledSsrcHandler::Action DefaultUnsignalledSsrcHandler::OnUnsignalledSsrc(
    WebRtcVideoChannel2* channel,
    uint32_t ssrc) {
  if (default_recv_ssrc_ != 0) {  // Already one default stream.
    LOG(LS_WARNING) << "Unknown SSRC, but default receive stream already set.";
    return kDropPacket;
  }

  StreamParams sp;
  sp.ssrcs.push_back(ssrc);
  LOG(LS_INFO) << "Creating default receive stream for SSRC=" << ssrc << ".";
  if (!channel->AddRecvStream(sp, true)) {
    LOG(LS_WARNING) << "Could not create default receive stream.";
  }

  channel->SetSink(ssrc, default_sink_);
  default_recv_ssrc_ = ssrc;
  return kDeliverPacket;
}

rtc::VideoSinkInterface<VideoFrame>*
DefaultUnsignalledSsrcHandler::GetDefaultSink() const {
  return default_sink_;
}

void DefaultUnsignalledSsrcHandler::SetDefaultSink(
    VideoMediaChannel* channel,
    rtc::VideoSinkInterface<VideoFrame>* sink) {
  default_sink_ = sink;
  if (default_recv_ssrc_ != 0) {
    channel->SetSink(default_recv_ssrc_, default_sink_);
  }
}

WebRtcVideoEngine2::WebRtcVideoEngine2()
    : initialized_(false),
      external_decoder_factory_(NULL),
      external_encoder_factory_(NULL) {
  LOG(LS_INFO) << "WebRtcVideoEngine2::WebRtcVideoEngine2()";
  video_codecs_ = GetSupportedCodecs();
}

WebRtcVideoEngine2::~WebRtcVideoEngine2() {
  LOG(LS_INFO) << "WebRtcVideoEngine2::~WebRtcVideoEngine2";
}

void WebRtcVideoEngine2::Init() {
  LOG(LS_INFO) << "WebRtcVideoEngine2::Init";
  initialized_ = true;
}

WebRtcVideoChannel2* WebRtcVideoEngine2::CreateChannel(
    webrtc::Call* call,
    const MediaConfig& config,
    const VideoOptions& options) {
  RTC_DCHECK(initialized_);
  LOG(LS_INFO) << "CreateChannel. Options: " << options.ToString();
  return new WebRtcVideoChannel2(call, config, options, video_codecs_,
                                 external_encoder_factory_,
                                 external_decoder_factory_);
}

const std::vector<VideoCodec>& WebRtcVideoEngine2::codecs() const {
  return video_codecs_;
}

RtpCapabilities WebRtcVideoEngine2::GetCapabilities() const {
  RtpCapabilities capabilities;
  capabilities.header_extensions.push_back(
      webrtc::RtpExtension(webrtc::RtpExtension::kTimestampOffsetUri,
                           webrtc::RtpExtension::kTimestampOffsetDefaultId));
  capabilities.header_extensions.push_back(
      webrtc::RtpExtension(webrtc::RtpExtension::kAbsSendTimeUri,
                           webrtc::RtpExtension::kAbsSendTimeDefaultId));
  capabilities.header_extensions.push_back(
      webrtc::RtpExtension(webrtc::RtpExtension::kVideoRotationUri,
                           webrtc::RtpExtension::kVideoRotationDefaultId));
  if (webrtc::field_trial::FindFullName("WebRTC-SendSideBwe") == "Enabled") {
    capabilities.header_extensions.push_back(webrtc::RtpExtension(
        webrtc::RtpExtension::kTransportSequenceNumberUri,
        webrtc::RtpExtension::kTransportSequenceNumberDefaultId));
  }
  capabilities.header_extensions.push_back(
      webrtc::RtpExtension(webrtc::RtpExtension::kPlayoutDelayUri,
                           webrtc::RtpExtension::kPlayoutDelayDefaultId));
  return capabilities;
}

void WebRtcVideoEngine2::SetExternalDecoderFactory(
    WebRtcVideoDecoderFactory* decoder_factory) {
  RTC_DCHECK(!initialized_);
  external_decoder_factory_ = decoder_factory;
}

void WebRtcVideoEngine2::SetExternalEncoderFactory(
    WebRtcVideoEncoderFactory* encoder_factory) {
  RTC_DCHECK(!initialized_);
  if (external_encoder_factory_ == encoder_factory)
    return;

  // No matter what happens we shouldn't hold on to a stale
  // WebRtcSimulcastEncoderFactory.
  simulcast_encoder_factory_.reset();

  if (encoder_factory &&
      WebRtcSimulcastEncoderFactory::UseSimulcastEncoderFactory(
          encoder_factory->codecs())) {
    simulcast_encoder_factory_.reset(
        new WebRtcSimulcastEncoderFactory(encoder_factory));
    encoder_factory = simulcast_encoder_factory_.get();
  }
  external_encoder_factory_ = encoder_factory;

  video_codecs_ = GetSupportedCodecs();
}

std::vector<VideoCodec> WebRtcVideoEngine2::GetSupportedCodecs() const {
  std::vector<VideoCodec> supported_codecs = DefaultVideoCodecList();

  if (external_encoder_factory_ == NULL) {
    LOG(LS_INFO) << "Supported codecs: "
                 << CodecVectorToString(supported_codecs);
    return supported_codecs;
  }

  std::stringstream out;
  const std::vector<WebRtcVideoEncoderFactory::VideoCodec>& codecs =
      external_encoder_factory_->codecs();
  for (size_t i = 0; i < codecs.size(); ++i) {
    out << codecs[i].name;
    if (i != codecs.size() - 1) {
      out << ", ";
    }
    // Don't add internally-supported codecs twice.
    if (CodecIsInternallySupported(codecs[i].name)) {
      continue;
    }

    // External video encoders are given payloads 120-127. This also means that
    // we only support up to 8 external payload types.
    const int kExternalVideoPayloadTypeBase = 120;
    size_t payload_type = kExternalVideoPayloadTypeBase + i;
    RTC_DCHECK(payload_type < 128);
    VideoCodec codec(static_cast<int>(payload_type), codecs[i].name,
                     codecs[i].max_width, codecs[i].max_height,
                     codecs[i].max_fps);

    AddDefaultFeedbackParams(&codec);
    supported_codecs.push_back(codec);
  }
  LOG(LS_INFO) << "Supported codecs (incl. external codecs): "
               << CodecVectorToString(supported_codecs);
  LOG(LS_INFO) << "Codecs supported by the external encoder factory: "
               << out.str();
  return supported_codecs;
}

WebRtcVideoChannel2::WebRtcVideoChannel2(
    webrtc::Call* call,
    const MediaConfig& config,
    const VideoOptions& options,
    const std::vector<VideoCodec>& recv_codecs,
    WebRtcVideoEncoderFactory* external_encoder_factory,
    WebRtcVideoDecoderFactory* external_decoder_factory)
    : VideoMediaChannel(config),
      call_(call),
      unsignalled_ssrc_handler_(&default_unsignalled_ssrc_handler_),
      video_config_(config.video),
      external_encoder_factory_(external_encoder_factory),
      external_decoder_factory_(external_decoder_factory),
      default_send_options_(options),
      red_disabled_by_remote_side_(false) {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());

  rtcp_receiver_report_ssrc_ = kDefaultRtcpReceiverReportSsrc;
  sending_ = false;
  RTC_DCHECK(ValidateCodecFormats(recv_codecs));
  recv_codecs_ = FilterSupportedCodecs(MapCodecs(recv_codecs));
}

WebRtcVideoChannel2::~WebRtcVideoChannel2() {
  for (auto& kv : send_streams_)
    delete kv.second;
  for (auto& kv : receive_streams_)
    delete kv.second;
}

bool WebRtcVideoChannel2::CodecIsExternallySupported(
    const std::string& name) const {
  if (external_encoder_factory_ == NULL) {
    return false;
  }

  const std::vector<WebRtcVideoEncoderFactory::VideoCodec> external_codecs =
      external_encoder_factory_->codecs();
  for (size_t c = 0; c < external_codecs.size(); ++c) {
    if (CodecNamesEq(name, external_codecs[c].name)) {
      return true;
    }
  }
  return false;
}

std::vector<WebRtcVideoChannel2::VideoCodecSettings>
WebRtcVideoChannel2::FilterSupportedCodecs(
    const std::vector<WebRtcVideoChannel2::VideoCodecSettings>& mapped_codecs)
    const {
  std::vector<VideoCodecSettings> supported_codecs;
  for (size_t i = 0; i < mapped_codecs.size(); ++i) {
    const VideoCodecSettings& codec = mapped_codecs[i];
    if (CodecIsInternallySupported(codec.codec.name) ||
        CodecIsExternallySupported(codec.codec.name)) {
      supported_codecs.push_back(codec);
    }
  }
  return supported_codecs;
}

bool WebRtcVideoChannel2::ReceiveCodecsHaveChanged(
    std::vector<VideoCodecSettings> before,
    std::vector<VideoCodecSettings> after) {
  if (before.size() != after.size()) {
    return true;
  }
  // The receive codec order doesn't matter, so we sort the codecs before
  // comparing. This is necessary because currently the
  // only way to change the send codec is to munge SDP, which causes
  // the receive codec list to change order, which causes the streams
  // to be recreates which causes a "blink" of black video.  In order
  // to support munging the SDP in this way without recreating receive
  // streams, we ignore the order of the received codecs so that
  // changing the order doesn't cause this "blink".
  auto comparison =
      [](const VideoCodecSettings& codec1, const VideoCodecSettings& codec2) {
        return codec1.codec.id > codec2.codec.id;
      };
  std::sort(before.begin(), before.end(), comparison);
  std::sort(after.begin(), after.end(), comparison);
  return before != after;
}

bool WebRtcVideoChannel2::GetChangedSendParameters(
    const VideoSendParameters& params,
    ChangedSendParameters* changed_params) const {
  if (!ValidateCodecFormats(params.codecs) ||
      !ValidateRtpExtensions(params.extensions)) {
    return false;
  }

  // Handle send codec.
  const std::vector<VideoCodecSettings> supported_codecs =
      FilterSupportedCodecs(MapCodecs(params.codecs));

  if (supported_codecs.empty()) {
    LOG(LS_ERROR) << "No video codecs supported.";
    return false;
  }

  if (!send_codec_ || supported_codecs.front() != *send_codec_) {
    changed_params->codec =
        rtc::Optional<VideoCodecSettings>(supported_codecs.front());
  }

  // Handle RTP header extensions.
  std::vector<webrtc::RtpExtension> filtered_extensions = FilterRtpExtensions(
      params.extensions, webrtc::RtpExtension::IsSupportedForVideo, true);
  if (!send_rtp_extensions_ || (*send_rtp_extensions_ != filtered_extensions)) {
    changed_params->rtp_header_extensions =
        rtc::Optional<std::vector<webrtc::RtpExtension>>(filtered_extensions);
  }

  // Handle max bitrate.
  if (params.max_bandwidth_bps != send_params_.max_bandwidth_bps &&
      params.max_bandwidth_bps >= 0) {
    // 0 uncaps max bitrate (-1).
    changed_params->max_bandwidth_bps = rtc::Optional<int>(
        params.max_bandwidth_bps == 0 ? -1 : params.max_bandwidth_bps);
  }

  // Handle conference mode.
  if (params.conference_mode != send_params_.conference_mode) {
    changed_params->conference_mode =
        rtc::Optional<bool>(params.conference_mode);
  }

  // Handle RTCP mode.
  if (params.rtcp.reduced_size != send_params_.rtcp.reduced_size) {
    changed_params->rtcp_mode = rtc::Optional<webrtc::RtcpMode>(
        params.rtcp.reduced_size ? webrtc::RtcpMode::kReducedSize
                                 : webrtc::RtcpMode::kCompound);
  }

  return true;
}

rtc::DiffServCodePoint WebRtcVideoChannel2::PreferredDscp() const {
  return rtc::DSCP_AF41;
}

bool WebRtcVideoChannel2::SetSendParameters(const VideoSendParameters& params) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::SetSendParameters");
  LOG(LS_INFO) << "SetSendParameters: " << params.ToString();
  ChangedSendParameters changed_params;
  if (!GetChangedSendParameters(params, &changed_params)) {
    return false;
  }

  if (changed_params.codec) {
    const VideoCodecSettings& codec_settings = *changed_params.codec;
    send_codec_ = rtc::Optional<VideoCodecSettings>(codec_settings);
    LOG(LS_INFO) << "Using codec: " << codec_settings.codec.ToString();
  }

  if (changed_params.rtp_header_extensions) {
    send_rtp_extensions_ = changed_params.rtp_header_extensions;
  }

  if (changed_params.codec || changed_params.max_bandwidth_bps) {
    if (send_codec_) {
      // TODO(holmer): Changing the codec parameters shouldn't necessarily mean
      // that we change the min/max of bandwidth estimation. Reevaluate this.
      bitrate_config_ = GetBitrateConfigForCodec(send_codec_->codec);
      if (!changed_params.codec) {
        // If the codec isn't changing, set the start bitrate to -1 which means
        // "unchanged" so that BWE isn't affected.
        bitrate_config_.start_bitrate_bps = -1;
      }
    }
    if (params.max_bandwidth_bps >= 0) {
      // Note that max_bandwidth_bps intentionally takes priority over the
      // bitrate config for the codec. This allows FEC to be applied above the
      // codec target bitrate.
      // TODO(pbos): Figure out whether b=AS means max bitrate for this
      // WebRtcVideoChannel2 (in which case we're good), or per sender (SSRC),
      // in which case this should not set a Call::BitrateConfig but rather
      // reconfigure all senders.
      bitrate_config_.max_bitrate_bps =
          params.max_bandwidth_bps == 0 ? -1 : params.max_bandwidth_bps;
    }
    call_->SetBitrateConfig(bitrate_config_);
  }

  {
    rtc::CritScope stream_lock(&stream_crit_);
    for (auto& kv : send_streams_) {
      kv.second->SetSendParameters(changed_params);
    }
    if (changed_params.codec || changed_params.rtcp_mode) {
      // Update receive feedback parameters from new codec or RTCP mode.
      LOG(LS_INFO)
          << "SetFeedbackOptions on all the receive streams because the send "
             "codec or RTCP mode has changed.";
      for (auto& kv : receive_streams_) {
        RTC_DCHECK(kv.second != nullptr);
        kv.second->SetFeedbackParameters(
            HasNack(send_codec_->codec), HasRemb(send_codec_->codec),
            HasTransportCc(send_codec_->codec),
            params.rtcp.reduced_size ? webrtc::RtcpMode::kReducedSize
                                     : webrtc::RtcpMode::kCompound);
      }
    }
    if (changed_params.codec) {
      bool red_was_disabled = red_disabled_by_remote_side_;
      red_disabled_by_remote_side_ =
          changed_params.codec->fec.red_payload_type == -1;
      if (red_was_disabled != red_disabled_by_remote_side_) {
        for (auto& kv : receive_streams_) {
          // In practice VideoChannel::SetRemoteContent appears to most of the
          // time also call UpdateRemoteStreams, which recreates the receive
          // streams. If that's always true this call isn't needed.
          kv.second->SetFecDisabledRemotely(red_disabled_by_remote_side_);
        }
      }
    }
  }
  send_params_ = params;
  return true;
}

webrtc::RtpParameters WebRtcVideoChannel2::GetRtpSendParameters(
    uint32_t ssrc) const {
  rtc::CritScope stream_lock(&stream_crit_);
  auto it = send_streams_.find(ssrc);
  if (it == send_streams_.end()) {
    LOG(LS_WARNING) << "Attempting to get RTP send parameters for stream "
                    << "with ssrc " << ssrc << " which doesn't exist.";
    return webrtc::RtpParameters();
  }

  webrtc::RtpParameters rtp_params = it->second->GetRtpParameters();
  // Need to add the common list of codecs to the send stream-specific
  // RTP parameters.
  for (const VideoCodec& codec : send_params_.codecs) {
    rtp_params.codecs.push_back(codec.ToCodecParameters());
  }
  return rtp_params;
}

bool WebRtcVideoChannel2::SetRtpSendParameters(
    uint32_t ssrc,
    const webrtc::RtpParameters& parameters) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::SetRtpSendParameters");
  rtc::CritScope stream_lock(&stream_crit_);
  auto it = send_streams_.find(ssrc);
  if (it == send_streams_.end()) {
    LOG(LS_ERROR) << "Attempting to set RTP send parameters for stream "
                  << "with ssrc " << ssrc << " which doesn't exist.";
    return false;
  }

  // TODO(deadbeef): Handle setting parameters with a list of codecs in a
  // different order (which should change the send codec).
  webrtc::RtpParameters current_parameters = GetRtpSendParameters(ssrc);
  if (current_parameters.codecs != parameters.codecs) {
    LOG(LS_ERROR) << "Using SetParameters to change the set of codecs "
                  << "is not currently supported.";
    return false;
  }

  return it->second->SetRtpParameters(parameters);
}

webrtc::RtpParameters WebRtcVideoChannel2::GetRtpReceiveParameters(
    uint32_t ssrc) const {
  rtc::CritScope stream_lock(&stream_crit_);
  auto it = receive_streams_.find(ssrc);
  if (it == receive_streams_.end()) {
    LOG(LS_WARNING) << "Attempting to get RTP receive parameters for stream "
                    << "with ssrc " << ssrc << " which doesn't exist.";
    return webrtc::RtpParameters();
  }

  // TODO(deadbeef): Return stream-specific parameters.
  webrtc::RtpParameters rtp_params = CreateRtpParametersWithOneEncoding();
  for (const VideoCodec& codec : recv_params_.codecs) {
    rtp_params.codecs.push_back(codec.ToCodecParameters());
  }
  return rtp_params;
}

bool WebRtcVideoChannel2::SetRtpReceiveParameters(
    uint32_t ssrc,
    const webrtc::RtpParameters& parameters) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::SetRtpReceiveParameters");
  rtc::CritScope stream_lock(&stream_crit_);
  auto it = receive_streams_.find(ssrc);
  if (it == receive_streams_.end()) {
    LOG(LS_ERROR) << "Attempting to set RTP receive parameters for stream "
                  << "with ssrc " << ssrc << " which doesn't exist.";
    return false;
  }

  webrtc::RtpParameters current_parameters = GetRtpReceiveParameters(ssrc);
  if (current_parameters != parameters) {
    LOG(LS_ERROR) << "Changing the RTP receive parameters is currently "
                  << "unsupported.";
    return false;
  }
  return true;
}

bool WebRtcVideoChannel2::GetChangedRecvParameters(
    const VideoRecvParameters& params,
    ChangedRecvParameters* changed_params) const {
  if (!ValidateCodecFormats(params.codecs) ||
      !ValidateRtpExtensions(params.extensions)) {
    return false;
  }

  // Handle receive codecs.
  const std::vector<VideoCodecSettings> mapped_codecs =
      MapCodecs(params.codecs);
  if (mapped_codecs.empty()) {
    LOG(LS_ERROR) << "SetRecvParameters called without any video codecs.";
    return false;
  }

  std::vector<VideoCodecSettings> supported_codecs =
      FilterSupportedCodecs(mapped_codecs);

  if (mapped_codecs.size() != supported_codecs.size()) {
    LOG(LS_ERROR) << "SetRecvParameters called with unsupported video codecs.";
    return false;
  }

  if (ReceiveCodecsHaveChanged(recv_codecs_, supported_codecs)) {
    changed_params->codec_settings =
        rtc::Optional<std::vector<VideoCodecSettings>>(supported_codecs);
  }

  // Handle RTP header extensions.
  std::vector<webrtc::RtpExtension> filtered_extensions = FilterRtpExtensions(
      params.extensions, webrtc::RtpExtension::IsSupportedForVideo, false);
  if (filtered_extensions != recv_rtp_extensions_) {
    changed_params->rtp_header_extensions =
        rtc::Optional<std::vector<webrtc::RtpExtension>>(filtered_extensions);
  }

  return true;
}

bool WebRtcVideoChannel2::SetRecvParameters(const VideoRecvParameters& params) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::SetRecvParameters");
  LOG(LS_INFO) << "SetRecvParameters: " << params.ToString();
  ChangedRecvParameters changed_params;
  if (!GetChangedRecvParameters(params, &changed_params)) {
    return false;
  }
  if (changed_params.rtp_header_extensions) {
    recv_rtp_extensions_ = *changed_params.rtp_header_extensions;
  }
  if (changed_params.codec_settings) {
    LOG(LS_INFO) << "Changing recv codecs from "
                 << CodecSettingsVectorToString(recv_codecs_) << " to "
                 << CodecSettingsVectorToString(*changed_params.codec_settings);
    recv_codecs_ = *changed_params.codec_settings;
  }

  {
    rtc::CritScope stream_lock(&stream_crit_);
    for (auto& kv : receive_streams_) {
      kv.second->SetRecvParameters(changed_params);
    }
  }
  recv_params_ = params;
  return true;
}

std::string WebRtcVideoChannel2::CodecSettingsVectorToString(
    const std::vector<VideoCodecSettings>& codecs) {
  std::stringstream out;
  out << '{';
  for (size_t i = 0; i < codecs.size(); ++i) {
    out << codecs[i].codec.ToString();
    if (i != codecs.size() - 1) {
      out << ", ";
    }
  }
  out << '}';
  return out.str();
}

bool WebRtcVideoChannel2::GetSendCodec(VideoCodec* codec) {
  if (!send_codec_) {
    LOG(LS_VERBOSE) << "GetSendCodec: No send codec set.";
    return false;
  }
  *codec = send_codec_->codec;
  return true;
}

bool WebRtcVideoChannel2::SetSend(bool send) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::SetSend");
  LOG(LS_VERBOSE) << "SetSend: " << (send ? "true" : "false");
  if (send && !send_codec_) {
    LOG(LS_ERROR) << "SetSend(true) called before setting codec.";
    return false;
  }
  {
    rtc::CritScope stream_lock(&stream_crit_);
    for (const auto& kv : send_streams_) {
      kv.second->SetSend(send);
    }
  }
  sending_ = send;
  return true;
}

// TODO(nisse): The enable argument was used for mute logic which has
// been moved to VideoBroadcaster. So remove the argument from this
// method.
bool WebRtcVideoChannel2::SetVideoSend(
    uint32_t ssrc,
    bool enable,
    const VideoOptions* options,
    rtc::VideoSourceInterface<cricket::VideoFrame>* source) {
  TRACE_EVENT0("webrtc", "SetVideoSend");
  RTC_DCHECK(ssrc != 0);
  LOG(LS_INFO) << "SetVideoSend (ssrc= " << ssrc << ", enable = " << enable
               << ", options: " << (options ? options->ToString() : "nullptr")
               << ", source = " << (source ? "(source)" : "nullptr") << ")";

  rtc::CritScope stream_lock(&stream_crit_);
  const auto& kv = send_streams_.find(ssrc);
  if (kv == send_streams_.end()) {
    // Allow unknown ssrc only if source is null.
    RTC_CHECK(source == nullptr);
    LOG(LS_ERROR) << "No sending stream on ssrc " << ssrc;
    return false;
  }

  return kv->second->SetVideoSend(enable, options, source);
}

bool WebRtcVideoChannel2::ValidateSendSsrcAvailability(
    const StreamParams& sp) const {
  for (uint32_t ssrc : sp.ssrcs) {
    if (send_ssrcs_.find(ssrc) != send_ssrcs_.end()) {
      LOG(LS_ERROR) << "Send stream with SSRC '" << ssrc << "' already exists.";
      return false;
    }
  }
  return true;
}

bool WebRtcVideoChannel2::ValidateReceiveSsrcAvailability(
    const StreamParams& sp) const {
  for (uint32_t ssrc : sp.ssrcs) {
    if (receive_ssrcs_.find(ssrc) != receive_ssrcs_.end()) {
      LOG(LS_ERROR) << "Receive stream with SSRC '" << ssrc
                    << "' already exists.";
      return false;
    }
  }
  return true;
}

bool WebRtcVideoChannel2::AddSendStream(const StreamParams& sp) {
  LOG(LS_INFO) << "AddSendStream: " << sp.ToString();
  if (!ValidateStreamParams(sp))
    return false;

  rtc::CritScope stream_lock(&stream_crit_);

  if (!ValidateSendSsrcAvailability(sp))
    return false;

  for (uint32_t used_ssrc : sp.ssrcs)
    send_ssrcs_.insert(used_ssrc);

  webrtc::VideoSendStream::Config config(this);
  config.suspend_below_min_bitrate = video_config_.suspend_below_min_bitrate;
  WebRtcVideoSendStream* stream = new WebRtcVideoSendStream(
      call_, sp, config, default_send_options_, external_encoder_factory_,
      video_config_.enable_cpu_overuse_detection,
      bitrate_config_.max_bitrate_bps, send_codec_, send_rtp_extensions_,
      send_params_);

  uint32_t ssrc = sp.first_ssrc();
  RTC_DCHECK(ssrc != 0);
  send_streams_[ssrc] = stream;

  if (rtcp_receiver_report_ssrc_ == kDefaultRtcpReceiverReportSsrc) {
    rtcp_receiver_report_ssrc_ = ssrc;
    LOG(LS_INFO) << "SetLocalSsrc on all the receive streams because we added "
                    "a send stream.";
    for (auto& kv : receive_streams_)
      kv.second->SetLocalSsrc(ssrc);
  }
  if (sending_) {
    stream->SetSend(true);
  }

  return true;
}

bool WebRtcVideoChannel2::RemoveSendStream(uint32_t ssrc) {
  LOG(LS_INFO) << "RemoveSendStream: " << ssrc;

  WebRtcVideoSendStream* removed_stream;
  {
    rtc::CritScope stream_lock(&stream_crit_);
    std::map<uint32_t, WebRtcVideoSendStream*>::iterator it =
        send_streams_.find(ssrc);
    if (it == send_streams_.end()) {
      return false;
    }

    for (uint32_t old_ssrc : it->second->GetSsrcs())
      send_ssrcs_.erase(old_ssrc);

    removed_stream = it->second;
    send_streams_.erase(it);

    // Switch receiver report SSRCs, the one in use is no longer valid.
    if (rtcp_receiver_report_ssrc_ == ssrc) {
      rtcp_receiver_report_ssrc_ = send_streams_.empty()
                                       ? kDefaultRtcpReceiverReportSsrc
                                       : send_streams_.begin()->first;
      LOG(LS_INFO) << "SetLocalSsrc on all the receive streams because the "
                      "previous local SSRC was removed.";

      for (auto& kv : receive_streams_) {
        kv.second->SetLocalSsrc(rtcp_receiver_report_ssrc_);
      }
    }
  }

  delete removed_stream;

  return true;
}

void WebRtcVideoChannel2::DeleteReceiveStream(
    WebRtcVideoChannel2::WebRtcVideoReceiveStream* stream) {
  for (uint32_t old_ssrc : stream->GetSsrcs())
    receive_ssrcs_.erase(old_ssrc);
  delete stream;
}

bool WebRtcVideoChannel2::AddRecvStream(const StreamParams& sp) {
  return AddRecvStream(sp, false);
}

bool WebRtcVideoChannel2::AddRecvStream(const StreamParams& sp,
                                        bool default_stream) {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());

  LOG(LS_INFO) << "AddRecvStream" << (default_stream ? " (default stream)" : "")
               << ": " << sp.ToString();
  if (!ValidateStreamParams(sp))
    return false;

  uint32_t ssrc = sp.first_ssrc();
  RTC_DCHECK(ssrc != 0);  // TODO(pbos): Is this ever valid?

  rtc::CritScope stream_lock(&stream_crit_);
  // Remove running stream if this was a default stream.
  const auto& prev_stream = receive_streams_.find(ssrc);
  if (prev_stream != receive_streams_.end()) {
    if (default_stream || !prev_stream->second->IsDefaultStream()) {
      LOG(LS_ERROR) << "Receive stream for SSRC '" << ssrc
                    << "' already exists.";
      return false;
    }
    DeleteReceiveStream(prev_stream->second);
    receive_streams_.erase(prev_stream);
  }

  if (!ValidateReceiveSsrcAvailability(sp))
    return false;

  for (uint32_t used_ssrc : sp.ssrcs)
    receive_ssrcs_.insert(used_ssrc);

  webrtc::VideoReceiveStream::Config config(this);
  ConfigureReceiverRtp(&config, sp);

  // Set up A/V sync group based on sync label.
  config.sync_group = sp.sync_label;

  config.rtp.remb = send_codec_ ? HasRemb(send_codec_->codec) : false;
  config.rtp.transport_cc =
      send_codec_ ? HasTransportCc(send_codec_->codec) : false;
  config.disable_prerenderer_smoothing =
      video_config_.disable_prerenderer_smoothing;

  receive_streams_[ssrc] = new WebRtcVideoReceiveStream(
      call_, sp, std::move(config), external_decoder_factory_, default_stream,
      recv_codecs_, red_disabled_by_remote_side_);

  return true;
}

void WebRtcVideoChannel2::ConfigureReceiverRtp(
    webrtc::VideoReceiveStream::Config* config,
    const StreamParams& sp) const {
  uint32_t ssrc = sp.first_ssrc();

  config->rtp.remote_ssrc = ssrc;
  config->rtp.local_ssrc = rtcp_receiver_report_ssrc_;

  config->rtp.extensions = recv_rtp_extensions_;
  // Whether or not the receive stream sends reduced size RTCP is determined
  // by the send params.
  // TODO(deadbeef): Once we change "send_params" to "sender_params" and
  // "recv_params" to "receiver_params", we should get this out of
  // receiver_params_.
  config->rtp.rtcp_mode = send_params_.rtcp.reduced_size
                              ? webrtc::RtcpMode::kReducedSize
                              : webrtc::RtcpMode::kCompound;

  // TODO(pbos): This protection is against setting the same local ssrc as
  // remote which is not permitted by the lower-level API. RTCP requires a
  // corresponding sender SSRC. Figure out what to do when we don't have
  // (receive-only) or know a good local SSRC.
  if (config->rtp.remote_ssrc == config->rtp.local_ssrc) {
    if (config->rtp.local_ssrc != kDefaultRtcpReceiverReportSsrc) {
      config->rtp.local_ssrc = kDefaultRtcpReceiverReportSsrc;
    } else {
      config->rtp.local_ssrc = kDefaultRtcpReceiverReportSsrc + 1;
    }
  }

  for (size_t i = 0; i < recv_codecs_.size(); ++i) {
    uint32_t rtx_ssrc;
    if (recv_codecs_[i].rtx_payload_type != -1 &&
        sp.GetFidSsrc(ssrc, &rtx_ssrc)) {
      webrtc::VideoReceiveStream::Config::Rtp::Rtx& rtx =
          config->rtp.rtx[recv_codecs_[i].codec.id];
      rtx.ssrc = rtx_ssrc;
      rtx.payload_type = recv_codecs_[i].rtx_payload_type;
    }
  }
}

bool WebRtcVideoChannel2::RemoveRecvStream(uint32_t ssrc) {
  LOG(LS_INFO) << "RemoveRecvStream: " << ssrc;
  if (ssrc == 0) {
    LOG(LS_ERROR) << "RemoveRecvStream with 0 ssrc is not supported.";
    return false;
  }

  rtc::CritScope stream_lock(&stream_crit_);
  std::map<uint32_t, WebRtcVideoReceiveStream*>::iterator stream =
      receive_streams_.find(ssrc);
  if (stream == receive_streams_.end()) {
    LOG(LS_ERROR) << "Stream not found for ssrc: " << ssrc;
    return false;
  }
  DeleteReceiveStream(stream->second);
  receive_streams_.erase(stream);

  return true;
}

bool WebRtcVideoChannel2::SetSink(uint32_t ssrc,
                                  rtc::VideoSinkInterface<VideoFrame>* sink) {
  LOG(LS_INFO) << "SetSink: ssrc:" << ssrc << " "
               << (sink ? "(ptr)" : "nullptr");
  if (ssrc == 0) {
    default_unsignalled_ssrc_handler_.SetDefaultSink(this, sink);
    return true;
  }

  rtc::CritScope stream_lock(&stream_crit_);
  std::map<uint32_t, WebRtcVideoReceiveStream*>::iterator it =
      receive_streams_.find(ssrc);
  if (it == receive_streams_.end()) {
    return false;
  }

  it->second->SetSink(sink);
  return true;
}

bool WebRtcVideoChannel2::GetStats(VideoMediaInfo* info) {
  TRACE_EVENT0("webrtc", "WebRtcVideoChannel2::GetStats");
  info->Clear();
  FillSenderStats(info);
  FillReceiverStats(info);
  webrtc::Call::Stats stats = call_->GetStats();
  FillBandwidthEstimationStats(stats, info);
  if (stats.rtt_ms != -1) {
    for (size_t i = 0; i < info->senders.size(); ++i) {
      info->senders[i].rtt_ms = stats.rtt_ms;
    }
  }
  return true;
}

void WebRtcVideoChannel2::FillSenderStats(VideoMediaInfo* video_media_info) {
  rtc::CritScope stream_lock(&stream_crit_);
  for (std::map<uint32_t, WebRtcVideoSendStream*>::iterator it =
           send_streams_.begin();
       it != send_streams_.end(); ++it) {
    video_media_info->senders.push_back(it->second->GetVideoSenderInfo());
  }
}

void WebRtcVideoChannel2::FillReceiverStats(VideoMediaInfo* video_media_info) {
  rtc::CritScope stream_lock(&stream_crit_);
  for (std::map<uint32_t, WebRtcVideoReceiveStream*>::iterator it =
           receive_streams_.begin();
       it != receive_streams_.end(); ++it) {
    video_media_info->receivers.push_back(it->second->GetVideoReceiverInfo());
  }
}

void WebRtcVideoChannel2::FillBandwidthEstimationStats(
    const webrtc::Call::Stats& stats,
    VideoMediaInfo* video_media_info) {
  BandwidthEstimationInfo bwe_info;
  bwe_info.available_send_bandwidth = stats.send_bandwidth_bps;
  bwe_info.available_recv_bandwidth = stats.recv_bandwidth_bps;
  bwe_info.bucket_delay = stats.pacer_delay_ms;

  // Get send stream bitrate stats.
  rtc::CritScope stream_lock(&stream_crit_);
  for (std::map<uint32_t, WebRtcVideoSendStream*>::iterator stream =
           send_streams_.begin();
       stream != send_streams_.end(); ++stream) {
    stream->second->FillBandwidthEstimationInfo(&bwe_info);
  }
  video_media_info->bw_estimations.push_back(bwe_info);
}

void WebRtcVideoChannel2::OnPacketReceived(
    rtc::CopyOnWriteBuffer* packet,
    const rtc::PacketTime& packet_time) {
  const webrtc::PacketTime webrtc_packet_time(packet_time.timestamp,
                                              packet_time.not_before);
  const webrtc::PacketReceiver::DeliveryStatus delivery_result =
      call_->Receiver()->DeliverPacket(
          webrtc::MediaType::VIDEO,
          packet->cdata(), packet->size(),
          webrtc_packet_time);
  switch (delivery_result) {
    case webrtc::PacketReceiver::DELIVERY_OK:
      return;
    case webrtc::PacketReceiver::DELIVERY_PACKET_ERROR:
      return;
    case webrtc::PacketReceiver::DELIVERY_UNKNOWN_SSRC:
      break;
  }

  uint32_t ssrc = 0;
  if (!GetRtpSsrc(packet->cdata(), packet->size(), &ssrc)) {
    return;
  }

  int payload_type = 0;
  if (!GetRtpPayloadType(packet->cdata(), packet->size(), &payload_type)) {
    return;
  }

  // See if this payload_type is registered as one that usually gets its own
  // SSRC (RTX) or at least is safe to drop either way (ULPFEC). If it is, and
  // it wasn't handled above by DeliverPacket, that means we don't know what
  // stream it associates with, and we shouldn't ever create an implicit channel
  // for these.
  for (auto& codec : recv_codecs_) {
    if (payload_type == codec.rtx_payload_type ||
        payload_type == codec.fec.red_rtx_payload_type ||
        payload_type == codec.fec.ulpfec_payload_type) {
      return;
    }
  }

  switch (unsignalled_ssrc_handler_->OnUnsignalledSsrc(this, ssrc)) {
    case UnsignalledSsrcHandler::kDropPacket:
      return;
    case UnsignalledSsrcHandler::kDeliverPacket:
      break;
  }

  if (call_->Receiver()->DeliverPacket(
          webrtc::MediaType::VIDEO,
          packet->cdata(), packet->size(),
          webrtc_packet_time) != webrtc::PacketReceiver::DELIVERY_OK) {
    LOG(LS_WARNING) << "Failed to deliver RTP packet on re-delivery.";
    return;
  }
}

void WebRtcVideoChannel2::OnRtcpReceived(
    rtc::CopyOnWriteBuffer* packet,
    const rtc::PacketTime& packet_time) {
  const webrtc::PacketTime webrtc_packet_time(packet_time.timestamp,
                                              packet_time.not_before);
  // TODO(pbos): Check webrtc::PacketReceiver::DELIVERY_OK once we deliver
  // for both audio and video on the same path. Since BundleFilter doesn't
  // filter RTCP anymore incoming RTCP packets could've been going to audio (so
  // logging failures spam the log).
  call_->Receiver()->DeliverPacket(
      webrtc::MediaType::VIDEO,
      packet->cdata(), packet->size(),
      webrtc_packet_time);
}

void WebRtcVideoChannel2::OnReadyToSend(bool ready) {
  LOG(LS_VERBOSE) << "OnReadyToSend: " << (ready ? "Ready." : "Not ready.");
  call_->SignalChannelNetworkState(
      webrtc::MediaType::VIDEO,
      ready ? webrtc::kNetworkUp : webrtc::kNetworkDown);
}

void WebRtcVideoChannel2::OnNetworkRouteChanged(
    const std::string& transport_name,
    const rtc::NetworkRoute& network_route) {
  call_->OnNetworkRouteChanged(transport_name, network_route);
}

void WebRtcVideoChannel2::SetInterface(NetworkInterface* iface) {
  MediaChannel::SetInterface(iface);
  // Set the RTP recv/send buffer to a bigger size
  MediaChannel::SetOption(NetworkInterface::ST_RTP,
                          rtc::Socket::OPT_RCVBUF,
                          kVideoRtpBufferSize);

  // Speculative change to increase the outbound socket buffer size.
  // In b/15152257, we are seeing a significant number of packets discarded
  // due to lack of socket buffer space, although it's not yet clear what the
  // ideal value should be.
  MediaChannel::SetOption(NetworkInterface::ST_RTP,
                          rtc::Socket::OPT_SNDBUF,
                          kVideoRtpBufferSize);
}

bool WebRtcVideoChannel2::SendRtp(const uint8_t* data,
                                  size_t len,
                                  const webrtc::PacketOptions& options) {
  rtc::CopyOnWriteBuffer packet(data, len, kMaxRtpPacketLen);
  rtc::PacketOptions rtc_options;
  rtc_options.packet_id = options.packet_id;
  return MediaChannel::SendPacket(&packet, rtc_options);
}

bool WebRtcVideoChannel2::SendRtcp(const uint8_t* data, size_t len) {
  rtc::CopyOnWriteBuffer packet(data, len, kMaxRtpPacketLen);
  return MediaChannel::SendRtcp(&packet, rtc::PacketOptions());
}

WebRtcVideoChannel2::WebRtcVideoSendStream::VideoSendStreamParameters::
    VideoSendStreamParameters(
        const webrtc::VideoSendStream::Config& config,
        const VideoOptions& options,
        int max_bitrate_bps,
        const rtc::Optional<VideoCodecSettings>& codec_settings)
    : config(config),
      options(options),
      max_bitrate_bps(max_bitrate_bps),
      codec_settings(codec_settings) {}

WebRtcVideoChannel2::WebRtcVideoSendStream::AllocatedEncoder::AllocatedEncoder(
    webrtc::VideoEncoder* encoder,
    webrtc::VideoCodecType type,
    bool external)
    : encoder(encoder),
      external_encoder(nullptr),
      type(type),
      external(external) {
  if (external) {
    external_encoder = encoder;
    this->encoder =
        new webrtc::VideoEncoderSoftwareFallbackWrapper(type, encoder);
  }
}

WebRtcVideoChannel2::WebRtcVideoSendStream::WebRtcVideoSendStream(
    webrtc::Call* call,
    const StreamParams& sp,
    const webrtc::VideoSendStream::Config& config,
    const VideoOptions& options,
    WebRtcVideoEncoderFactory* external_encoder_factory,
    bool enable_cpu_overuse_detection,
    int max_bitrate_bps,
    const rtc::Optional<VideoCodecSettings>& codec_settings,
    const rtc::Optional<std::vector<webrtc::RtpExtension>>& rtp_extensions,
    // TODO(deadbeef): Don't duplicate information between send_params,
    // rtp_extensions, options, etc.
    const VideoSendParameters& send_params)
    : worker_thread_(rtc::Thread::Current()),
      ssrcs_(sp.ssrcs),
      ssrc_groups_(sp.ssrc_groups),
      call_(call),
      cpu_restricted_counter_(0),
      number_of_cpu_adapt_changes_(0),
      source_(nullptr),
      external_encoder_factory_(external_encoder_factory),
      stream_(nullptr),
      parameters_(config, options, max_bitrate_bps, codec_settings),
      rtp_parameters_(CreateRtpParametersWithOneEncoding()),
      pending_encoder_reconfiguration_(false),
      allocated_encoder_(nullptr, webrtc::kVideoCodecUnknown, false),
      sending_(false),
      last_frame_timestamp_ms_(0) {
  parameters_.config.rtp.max_packet_size = kVideoMtu;
  parameters_.conference_mode = send_params.conference_mode;

  sp.GetPrimarySsrcs(&parameters_.config.rtp.ssrcs);
  sp.GetFidSsrcs(parameters_.config.rtp.ssrcs,
                 &parameters_.config.rtp.rtx.ssrcs);
  parameters_.config.rtp.c_name = sp.cname;
  if (rtp_extensions) {
    parameters_.config.rtp.extensions = *rtp_extensions;
  }
  parameters_.config.rtp.rtcp_mode = send_params.rtcp.reduced_size
                                         ? webrtc::RtcpMode::kReducedSize
                                         : webrtc::RtcpMode::kCompound;
  parameters_.config.overuse_callback =
      enable_cpu_overuse_detection ? this : nullptr;

  // Only request rotation at the source when we positively know that the remote
  // side doesn't support the rotation extension. This allows us to prepare the
  // encoder in the expectation that rotation is supported - which is the common
  // case.
  sink_wants_.rotation_applied =
      rtp_extensions &&
      !ContainsHeaderExtension(*rtp_extensions,
                               webrtc::RtpExtension::kVideoRotationUri);

  if (codec_settings) {
    SetCodec(*codec_settings);
  }
}

WebRtcVideoChannel2::WebRtcVideoSendStream::~WebRtcVideoSendStream() {
  DisconnectSource();
  if (stream_ != NULL) {
    call_->DestroyVideoSendStream(stream_);
  }
  DestroyVideoEncoder(&allocated_encoder_);
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::OnFrame(
    const VideoFrame& frame) {
  TRACE_EVENT0("webrtc", "WebRtcVideoSendStream::OnFrame");
  webrtc::VideoFrame video_frame(frame.video_frame_buffer(), 0, 0,
                                 frame.rotation());
  rtc::CritScope cs(&lock_);

  if (video_frame.width() != last_frame_info_.width ||
      video_frame.height() != last_frame_info_.height ||
      video_frame.rotation() != last_frame_info_.rotation ||
      video_frame.is_texture() != last_frame_info_.is_texture) {
    last_frame_info_.width = video_frame.width();
    last_frame_info_.height = video_frame.height();
    last_frame_info_.rotation = video_frame.rotation();
    last_frame_info_.is_texture = video_frame.is_texture();
    pending_encoder_reconfiguration_ = true;

    LOG(LS_INFO) << "Video frame parameters changed: dimensions="
                 << last_frame_info_.width << "x" << last_frame_info_.height
                 << ", rotation=" << last_frame_info_.rotation
                 << ", texture=" << last_frame_info_.is_texture;
  }

  if (stream_ == NULL) {
    // Frame input before send codecs are configured, dropping frame.
    return;
  }

  int64_t frame_delta_ms = frame.GetTimeStamp() / rtc::kNumNanosecsPerMillisec;

  // frame->GetTimeStamp() is essentially a delta, align to webrtc time
  if (!first_frame_timestamp_ms_) {
    first_frame_timestamp_ms_ =
        rtc::Optional<int64_t>(rtc::TimeMillis() - frame_delta_ms);
  }

  last_frame_timestamp_ms_ = *first_frame_timestamp_ms_ + frame_delta_ms;

  video_frame.set_render_time_ms(last_frame_timestamp_ms_);

  if (pending_encoder_reconfiguration_) {
    ReconfigureEncoder();
    pending_encoder_reconfiguration_ = false;
  }

  // Not sending, abort after reconfiguration. Reconfiguration should still
  // occur to permit sending this input as quickly as possible once we start
  // sending (without having to reconfigure then).
  if (!sending_) {
    return;
  }

  stream_->Input()->IncomingCapturedFrame(video_frame);
}

bool WebRtcVideoChannel2::WebRtcVideoSendStream::SetVideoSend(
    bool enable,
    const VideoOptions* options,
    rtc::VideoSourceInterface<cricket::VideoFrame>* source) {
  TRACE_EVENT0("webrtc", "WebRtcVideoSendStream::SetVideoSend");
  RTC_DCHECK(thread_checker_.CalledOnValidThread());

  // Ignore |options| pointer if |enable| is false.
  bool options_present = enable && options;
  bool source_changing = source_ != source;
  if (source_changing) {
    DisconnectSource();
  }

  if (options_present || source_changing) {
    rtc::CritScope cs(&lock_);

    if (options_present) {
      VideoOptions old_options = parameters_.options;
      parameters_.options.SetAll(*options);
      // Reconfigure encoder settings on the naext frame or stream
      // recreation if the options changed.
      if (parameters_.options != old_options) {
        pending_encoder_reconfiguration_ = true;
      }
    }

    if (source_changing) {
      // Reset timestamps to realign new incoming frames to a webrtc timestamp.
      // A new source may have a different timestamp delta than the previous
      // one.
      first_frame_timestamp_ms_ = rtc::Optional<int64_t>();

      if (source == nullptr && stream_ != nullptr) {
        LOG(LS_VERBOSE) << "Disabling capturer, sending black frame.";
        // Force this black frame not to be dropped due to timestamp order
        // check. As IncomingCapturedFrame will drop the frame if this frame's
        // timestamp is less than or equal to last frame's timestamp, it is
        // necessary to give this black frame a larger timestamp than the
        // previous one.
        last_frame_timestamp_ms_ += 1;
        rtc::scoped_refptr<webrtc::I420Buffer> black_buffer(
            webrtc::I420Buffer::Create(last_frame_info_.width,
                                       last_frame_info_.height));
        black_buffer->SetToBlack();

        stream_->Input()->IncomingCapturedFrame(webrtc::VideoFrame(
            black_buffer,  0 /* timestamp (90 kHz) */,
            last_frame_timestamp_ms_, last_frame_info_.rotation));
      }
      source_ = source;
    }
  }

  // |source_->AddOrUpdateSink| may not be called while holding |lock_| since
  // that might cause a lock order inversion.
  if (source_changing && source_) {
    source_->AddOrUpdateSink(this, sink_wants_);
  }
  return true;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::DisconnectSource() {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  if (source_ == NULL) {
    return;
  }

  // |source_->RemoveSink| may not be called while holding |lock_| since
  // that might cause a lock order inversion.
  source_->RemoveSink(this);
  source_ = nullptr;
  // Reset |cpu_restricted_counter_| if the source is changed. It is not
  // possible to know if the video resolution is restricted by CPU usage after
  // the source is changed since the next source might be screen capture
  // with another resolution and frame rate.
  cpu_restricted_counter_ = 0;
}

const std::vector<uint32_t>&
WebRtcVideoChannel2::WebRtcVideoSendStream::GetSsrcs() const {
  return ssrcs_;
}

webrtc::VideoCodecType CodecTypeFromName(const std::string& name) {
  if (CodecNamesEq(name, kVp8CodecName)) {
    return webrtc::kVideoCodecVP8;
  } else if (CodecNamesEq(name, kVp9CodecName)) {
    return webrtc::kVideoCodecVP9;
  } else if (CodecNamesEq(name, kH264CodecName)) {
    return webrtc::kVideoCodecH264;
  }
  return webrtc::kVideoCodecUnknown;
}

WebRtcVideoChannel2::WebRtcVideoSendStream::AllocatedEncoder
WebRtcVideoChannel2::WebRtcVideoSendStream::CreateVideoEncoder(
    const VideoCodec& codec) {
  webrtc::VideoCodecType type = CodecTypeFromName(codec.name);

  // Do not re-create encoders of the same type.
  if (type == allocated_encoder_.type && allocated_encoder_.encoder != NULL) {
    return allocated_encoder_;
  }

  if (external_encoder_factory_ != NULL) {
    webrtc::VideoEncoder* encoder =
        external_encoder_factory_->CreateVideoEncoder(type);
    if (encoder != NULL) {
      return AllocatedEncoder(encoder, type, true);
    }
  }

  if (type == webrtc::kVideoCodecVP8) {
    return AllocatedEncoder(
        webrtc::VideoEncoder::Create(webrtc::VideoEncoder::kVp8), type, false);
  } else if (type == webrtc::kVideoCodecVP9) {
    return AllocatedEncoder(
        webrtc::VideoEncoder::Create(webrtc::VideoEncoder::kVp9), type, false);
  } else if (type == webrtc::kVideoCodecH264) {
    return AllocatedEncoder(
        webrtc::VideoEncoder::Create(webrtc::VideoEncoder::kH264), type, false);
  }

  // This shouldn't happen, we should not be trying to create something we don't
  // support.
  RTC_DCHECK(false);
  return AllocatedEncoder(NULL, webrtc::kVideoCodecUnknown, false);
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::DestroyVideoEncoder(
    AllocatedEncoder* encoder) {
  if (encoder->external) {
    external_encoder_factory_->DestroyVideoEncoder(encoder->external_encoder);
  }
  delete encoder->encoder;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::SetCodec(
    const VideoCodecSettings& codec_settings) {
  parameters_.encoder_config = CreateVideoEncoderConfig(codec_settings.codec);
  RTC_DCHECK(!parameters_.encoder_config.streams.empty());

  AllocatedEncoder new_encoder = CreateVideoEncoder(codec_settings.codec);
  parameters_.config.encoder_settings.encoder = new_encoder.encoder;
  parameters_.config.encoder_settings.full_overuse_time = new_encoder.external;
  parameters_.config.encoder_settings.payload_name = codec_settings.codec.name;
  parameters_.config.encoder_settings.payload_type = codec_settings.codec.id;
  if (new_encoder.external) {
    webrtc::VideoCodecType type = CodecTypeFromName(codec_settings.codec.name);
    parameters_.config.encoder_settings.internal_source =
        external_encoder_factory_->EncoderTypeHasInternalSource(type);
  }
  parameters_.config.rtp.fec = codec_settings.fec;

  // Set RTX payload type if RTX is enabled.
  if (!parameters_.config.rtp.rtx.ssrcs.empty()) {
    if (codec_settings.rtx_payload_type == -1) {
      LOG(LS_WARNING) << "RTX SSRCs configured but there's no configured RTX "
                         "payload type. Ignoring.";
      parameters_.config.rtp.rtx.ssrcs.clear();
    } else {
      parameters_.config.rtp.rtx.payload_type = codec_settings.rtx_payload_type;
    }
  }

  parameters_.config.rtp.nack.rtp_history_ms =
      HasNack(codec_settings.codec) ? kNackHistoryMs : 0;

  parameters_.codec_settings =
      rtc::Optional<WebRtcVideoChannel2::VideoCodecSettings>(codec_settings);

  LOG(LS_INFO) << "RecreateWebRtcStream (send) because of SetCodec.";
  RecreateWebRtcStream();
  if (allocated_encoder_.encoder != new_encoder.encoder) {
    DestroyVideoEncoder(&allocated_encoder_);
    allocated_encoder_ = new_encoder;
  }
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::SetSendParameters(
    const ChangedSendParameters& params) {
  {
    rtc::CritScope cs(&lock_);
    // |recreate_stream| means construction-time parameters have changed and the
    // sending stream needs to be reset with the new config.
    bool recreate_stream = false;
    if (params.rtcp_mode) {
      parameters_.config.rtp.rtcp_mode = *params.rtcp_mode;
      recreate_stream = true;
    }
    if (params.rtp_header_extensions) {
      parameters_.config.rtp.extensions = *params.rtp_header_extensions;
      recreate_stream = true;
    }
    if (params.max_bandwidth_bps) {
      parameters_.max_bitrate_bps = *params.max_bandwidth_bps;
      pending_encoder_reconfiguration_ = true;
    }
    if (params.conference_mode) {
      parameters_.conference_mode = *params.conference_mode;
    }

    // Set codecs and options.
    if (params.codec) {
      SetCodec(*params.codec);
      recreate_stream = false;  // SetCodec has already recreated the stream.
    } else if (params.conference_mode && parameters_.codec_settings) {
      SetCodec(*parameters_.codec_settings);
      recreate_stream = false;  // SetCodec has already recreated the stream.
    }
    if (recreate_stream) {
      LOG(LS_INFO)
          << "RecreateWebRtcStream (send) because of SetSendParameters";
      RecreateWebRtcStream();
    }
  }  // release |lock_|

  // |source_->AddOrUpdateSink| may not be called while holding |lock_| since
  // that might cause a lock order inversion.
  if (params.rtp_header_extensions) {
    sink_wants_.rotation_applied = !ContainsHeaderExtension(
        *params.rtp_header_extensions, webrtc::RtpExtension::kVideoRotationUri);
    if (source_) {
      source_->AddOrUpdateSink(this, sink_wants_);
    }
  }
}

bool WebRtcVideoChannel2::WebRtcVideoSendStream::SetRtpParameters(
    const webrtc::RtpParameters& new_parameters) {
  if (!ValidateRtpParameters(new_parameters)) {
    return false;
  }

  rtc::CritScope cs(&lock_);
  if (new_parameters.encodings[0].max_bitrate_bps !=
      rtp_parameters_.encodings[0].max_bitrate_bps) {
    pending_encoder_reconfiguration_ = true;
  }
  rtp_parameters_ = new_parameters;
  // Codecs are currently handled at the WebRtcVideoChannel2 level.
  rtp_parameters_.codecs.clear();
  // Encoding may have been activated/deactivated.
  UpdateSendState();
  return true;
}

webrtc::RtpParameters
WebRtcVideoChannel2::WebRtcVideoSendStream::GetRtpParameters() const {
  rtc::CritScope cs(&lock_);
  return rtp_parameters_;
}

bool WebRtcVideoChannel2::WebRtcVideoSendStream::ValidateRtpParameters(
    const webrtc::RtpParameters& rtp_parameters) {
  if (rtp_parameters.encodings.size() != 1) {
    LOG(LS_ERROR)
        << "Attempted to set RtpParameters without exactly one encoding";
    return false;
  }
  return true;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::UpdateSendState() {
  // TODO(deadbeef): Need to handle more than one encoding in the future.
  RTC_DCHECK(rtp_parameters_.encodings.size() == 1u);
  if (sending_ && rtp_parameters_.encodings[0].active) {
    RTC_DCHECK(stream_ != nullptr);
    stream_->Start();
  } else {
    if (stream_ != nullptr) {
      stream_->Stop();
    }
  }
}

webrtc::VideoEncoderConfig
WebRtcVideoChannel2::WebRtcVideoSendStream::CreateVideoEncoderConfig(
    const VideoCodec& codec) const {
  webrtc::VideoEncoderConfig encoder_config;
  bool is_screencast = parameters_.options.is_screencast.value_or(false);
  if (is_screencast) {
    encoder_config.min_transmit_bitrate_bps =
        1000 * parameters_.options.screencast_min_bitrate_kbps.value_or(0);
    encoder_config.content_type =
        webrtc::VideoEncoderConfig::ContentType::kScreen;
  } else {
    encoder_config.min_transmit_bitrate_bps = 0;
    encoder_config.content_type =
        webrtc::VideoEncoderConfig::ContentType::kRealtimeVideo;
  }

  // Restrict dimensions according to codec max.
  int width = last_frame_info_.width;
  int height = last_frame_info_.height;
  if (!is_screencast) {
    if (codec.width < width)
      width = codec.width;
    if (codec.height < height)
      height = codec.height;
  }

  VideoCodec clamped_codec = codec;
  clamped_codec.width = width;
  clamped_codec.height = height;

  // By default, the stream count for the codec configuration should match the
  // number of negotiated ssrcs. But if the codec is blacklisted for simulcast
  // or a screencast, only configure a single stream.
  size_t stream_count = parameters_.config.rtp.ssrcs.size();
  if (IsCodecBlacklistedForSimulcast(codec.name) || is_screencast) {
    stream_count = 1;
  }

  int stream_max_bitrate =
      MinPositive(rtp_parameters_.encodings[0].max_bitrate_bps,
                  parameters_.max_bitrate_bps);
  encoder_config.streams = CreateVideoStreams(
      clamped_codec, parameters_.options, stream_max_bitrate, stream_count);
  encoder_config.expect_encode_from_texture = last_frame_info_.is_texture;

  // Conference mode screencast uses 2 temporal layers split at 100kbit.
  if (parameters_.conference_mode && is_screencast &&
      encoder_config.streams.size() == 1) {
    ScreenshareLayerConfig config = ScreenshareLayerConfig::GetDefault();

    // For screenshare in conference mode, tl0 and tl1 bitrates are piggybacked
    // on the VideoCodec struct as target and max bitrates, respectively.
    // See eg. webrtc::VP8EncoderImpl::SetRates().
    encoder_config.streams[0].target_bitrate_bps =
        config.tl0_bitrate_kbps * 1000;
    encoder_config.streams[0].max_bitrate_bps = config.tl1_bitrate_kbps * 1000;
    encoder_config.streams[0].temporal_layer_thresholds_bps.clear();
    encoder_config.streams[0].temporal_layer_thresholds_bps.push_back(
        config.tl0_bitrate_kbps * 1000);
  }
  if (CodecNamesEq(codec.name, kVp9CodecName) && !is_screencast &&
      encoder_config.streams.size() == 1) {
    encoder_config.streams[0].temporal_layer_thresholds_bps.resize(
        GetDefaultVp9TemporalLayers() - 1);
  }
  return encoder_config;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::ReconfigureEncoder() {
  RTC_DCHECK(!parameters_.encoder_config.streams.empty());

  RTC_CHECK(parameters_.codec_settings);
  VideoCodecSettings codec_settings = *parameters_.codec_settings;

  webrtc::VideoEncoderConfig encoder_config =
      CreateVideoEncoderConfig(codec_settings.codec);

  encoder_config.encoder_specific_settings = ConfigureVideoEncoderSettings(
      codec_settings.codec);

  stream_->ReconfigureVideoEncoder(encoder_config);

  encoder_config.encoder_specific_settings = NULL;

  parameters_.encoder_config = encoder_config;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::SetSend(bool send) {
  rtc::CritScope cs(&lock_);
  sending_ = send;
  UpdateSendState();
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::OnLoadUpdate(Load load) {
  if (worker_thread_ != rtc::Thread::Current()) {
    invoker_.AsyncInvoke<void>(
        RTC_FROM_HERE, worker_thread_,
        rtc::Bind(&WebRtcVideoChannel2::WebRtcVideoSendStream::OnLoadUpdate,
                  this, load));
    return;
  }
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  if (!source_) {
    return;
  }
  {
    rtc::CritScope cs(&lock_);
    LOG(LS_INFO) << "OnLoadUpdate " << load << ", is_screencast: "
                 << (parameters_.options.is_screencast
                         ? (*parameters_.options.is_screencast ? "true"
                                                               : "false")
                         : "unset");
    // Do not adapt resolution for screen content as this will likely result in
    // blurry and unreadable text.
    if (parameters_.options.is_screencast.value_or(false))
      return;

    rtc::Optional<int> max_pixel_count;
    rtc::Optional<int> max_pixel_count_step_up;
    if (load == kOveruse) {
      if (cpu_restricted_counter_ >= kMaxCpuDowngrades) {
        return;
      }
      // The input video frame size will have a resolution with less than or
      // equal to |max_pixel_count| depending on how the source can scale the
      // input frame size.
      max_pixel_count = rtc::Optional<int>(
          (last_frame_info_.height * last_frame_info_.width * 3) / 5);
      // Increase |number_of_cpu_adapt_changes_| if
      // sink_wants_.max_pixel_count will be changed since
      // last time |source_->AddOrUpdateSink| was called. That is, this will
      // result in a new request for the source to change resolution.
      if (!sink_wants_.max_pixel_count ||
          *sink_wants_.max_pixel_count > *max_pixel_count) {
        ++number_of_cpu_adapt_changes_;
        ++cpu_restricted_counter_;
      }
    } else {
      RTC_DCHECK(load == kUnderuse);
      // The input video frame size will have a resolution with "one step up"
      // pixels than |max_pixel_count_step_up| where "one step up" depends on
      // how the source can scale the input frame size.
      max_pixel_count_step_up =
          rtc::Optional<int>(last_frame_info_.height * last_frame_info_.width);
      // Increase |number_of_cpu_adapt_changes_| if
      // sink_wants_.max_pixel_count_step_up will be changed since
      // last time |source_->AddOrUpdateSink| was called. That is, this will
      // result in a new request for the source to change resolution.
      if (sink_wants_.max_pixel_count ||
          (sink_wants_.max_pixel_count_step_up &&
           *sink_wants_.max_pixel_count_step_up < *max_pixel_count_step_up)) {
        ++number_of_cpu_adapt_changes_;
        --cpu_restricted_counter_;
      }
    }
    sink_wants_.max_pixel_count = max_pixel_count;
    sink_wants_.max_pixel_count_step_up = max_pixel_count_step_up;
  }
  // |source_->AddOrUpdateSink| may not be called while holding |lock_| since
  // that might cause a lock order inversion.
  source_->AddOrUpdateSink(this, sink_wants_);
}

VideoSenderInfo
WebRtcVideoChannel2::WebRtcVideoSendStream::GetVideoSenderInfo() {
  VideoSenderInfo info;
  webrtc::VideoSendStream::Stats stats;
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  {
    rtc::CritScope cs(&lock_);
    for (uint32_t ssrc : parameters_.config.rtp.ssrcs)
      info.add_ssrc(ssrc);

    if (parameters_.codec_settings)
      info.codec_name = parameters_.codec_settings->codec.name;
    for (size_t i = 0; i < parameters_.encoder_config.streams.size(); ++i) {
      if (i == parameters_.encoder_config.streams.size() - 1) {
        info.preferred_bitrate +=
            parameters_.encoder_config.streams[i].max_bitrate_bps;
      } else {
        info.preferred_bitrate +=
            parameters_.encoder_config.streams[i].target_bitrate_bps;
      }
    }

    if (stream_ == NULL)
      return info;

    stats = stream_->GetStats();
  }
  info.adapt_changes = number_of_cpu_adapt_changes_;
  info.adapt_reason =
      cpu_restricted_counter_ <= 0 ? ADAPTREASON_NONE : ADAPTREASON_CPU;

  // Get bandwidth limitation info from stream_->GetStats().
  // Input resolution (output from video_adapter) can be further scaled down or
  // higher video layer(s) can be dropped due to bitrate constraints.
  // Note, adapt_changes only include changes from the video_adapter.
  if (stats.bw_limited_resolution)
    info.adapt_reason |= ADAPTREASON_BANDWIDTH;

  info.encoder_implementation_name = stats.encoder_implementation_name;
  info.ssrc_groups = ssrc_groups_;
  info.framerate_input = stats.input_frame_rate;
  info.framerate_sent = stats.encode_frame_rate;
  info.avg_encode_ms = stats.avg_encode_time_ms;
  info.encode_usage_percent = stats.encode_usage_percent;

  info.nominal_bitrate = stats.media_bitrate_bps;

  info.send_frame_width = 0;
  info.send_frame_height = 0;
  for (std::map<uint32_t, webrtc::VideoSendStream::StreamStats>::iterator it =
           stats.substreams.begin();
       it != stats.substreams.end(); ++it) {
    // TODO(pbos): Wire up additional stats, such as padding bytes.
    webrtc::VideoSendStream::StreamStats stream_stats = it->second;
    info.bytes_sent += stream_stats.rtp_stats.transmitted.payload_bytes +
                       stream_stats.rtp_stats.transmitted.header_bytes +
                       stream_stats.rtp_stats.transmitted.padding_bytes;
    info.packets_sent += stream_stats.rtp_stats.transmitted.packets;
    info.packets_lost += stream_stats.rtcp_stats.cumulative_lost;
    if (stream_stats.width > info.send_frame_width)
      info.send_frame_width = stream_stats.width;
    if (stream_stats.height > info.send_frame_height)
      info.send_frame_height = stream_stats.height;
    info.firs_rcvd += stream_stats.rtcp_packet_type_counts.fir_packets;
    info.nacks_rcvd += stream_stats.rtcp_packet_type_counts.nack_packets;
    info.plis_rcvd += stream_stats.rtcp_packet_type_counts.pli_packets;
  }

  if (!stats.substreams.empty()) {
    // TODO(pbos): Report fraction lost per SSRC.
    webrtc::VideoSendStream::StreamStats first_stream_stats =
        stats.substreams.begin()->second;
    info.fraction_lost =
        static_cast<float>(first_stream_stats.rtcp_stats.fraction_lost) /
        (1 << 8);
  }

  return info;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::FillBandwidthEstimationInfo(
    BandwidthEstimationInfo* bwe_info) {
  rtc::CritScope cs(&lock_);
  if (stream_ == NULL) {
    return;
  }
  webrtc::VideoSendStream::Stats stats = stream_->GetStats();
  for (std::map<uint32_t, webrtc::VideoSendStream::StreamStats>::iterator it =
           stats.substreams.begin();
       it != stats.substreams.end(); ++it) {
    bwe_info->transmit_bitrate += it->second.total_bitrate_bps;
    bwe_info->retransmit_bitrate += it->second.retransmit_bitrate_bps;
  }
  bwe_info->target_enc_bitrate += stats.target_media_bitrate_bps;
  bwe_info->actual_enc_bitrate += stats.media_bitrate_bps;
}

void WebRtcVideoChannel2::WebRtcVideoSendStream::RecreateWebRtcStream() {
  if (stream_ != NULL) {
    call_->DestroyVideoSendStream(stream_);
  }

  RTC_CHECK(parameters_.codec_settings);
  RTC_DCHECK_EQ((parameters_.encoder_config.content_type ==
                 webrtc::VideoEncoderConfig::ContentType::kScreen),
                parameters_.options.is_screencast.value_or(false))
      << "encoder content type inconsistent with screencast option";
  parameters_.encoder_config.encoder_specific_settings =
      ConfigureVideoEncoderSettings(parameters_.codec_settings->codec);

  webrtc::VideoSendStream::Config config = parameters_.config;
  if (!config.rtp.rtx.ssrcs.empty() && config.rtp.rtx.payload_type == -1) {
    LOG(LS_WARNING) << "RTX SSRCs configured but there's no configured RTX "
                       "payload type the set codec. Ignoring RTX.";
    config.rtp.rtx.ssrcs.clear();
  }
  stream_ = call_->CreateVideoSendStream(config, parameters_.encoder_config);

  parameters_.encoder_config.encoder_specific_settings = NULL;
  pending_encoder_reconfiguration_ = false;

  if (sending_) {
    stream_->Start();
  }
}

WebRtcVideoChannel2::WebRtcVideoReceiveStream::WebRtcVideoReceiveStream(
    webrtc::Call* call,
    const StreamParams& sp,
    webrtc::VideoReceiveStream::Config config,
    WebRtcVideoDecoderFactory* external_decoder_factory,
    bool default_stream,
    const std::vector<VideoCodecSettings>& recv_codecs,
    bool red_disabled_by_remote_side)
    : call_(call),
      ssrcs_(sp.ssrcs),
      ssrc_groups_(sp.ssrc_groups),
      stream_(NULL),
      default_stream_(default_stream),
      config_(std::move(config)),
      red_disabled_by_remote_side_(red_disabled_by_remote_side),
      external_decoder_factory_(external_decoder_factory),
      sink_(NULL),
      last_width_(-1),
      last_height_(-1),
      first_frame_timestamp_(-1),
      estimated_remote_start_ntp_time_ms_(0) {
  config_.renderer = this;
  std::vector<AllocatedDecoder> old_decoders;
  ConfigureCodecs(recv_codecs, &old_decoders);
  RecreateWebRtcStream();
  RTC_DCHECK(old_decoders.empty());
}

WebRtcVideoChannel2::WebRtcVideoReceiveStream::AllocatedDecoder::
    AllocatedDecoder(webrtc::VideoDecoder* decoder,
                     webrtc::VideoCodecType type,
                     bool external)
    : decoder(decoder),
      external_decoder(nullptr),
      type(type),
      external(external) {
  if (external) {
    external_decoder = decoder;
    this->decoder =
        new webrtc::VideoDecoderSoftwareFallbackWrapper(type, external_decoder);
  }
}

WebRtcVideoChannel2::WebRtcVideoReceiveStream::~WebRtcVideoReceiveStream() {
  call_->DestroyVideoReceiveStream(stream_);
  ClearDecoders(&allocated_decoders_);
}

const std::vector<uint32_t>&
WebRtcVideoChannel2::WebRtcVideoReceiveStream::GetSsrcs() const {
  return ssrcs_;
}

WebRtcVideoChannel2::WebRtcVideoReceiveStream::AllocatedDecoder
WebRtcVideoChannel2::WebRtcVideoReceiveStream::CreateOrReuseVideoDecoder(
    std::vector<AllocatedDecoder>* old_decoders,
    const VideoCodec& codec) {
  webrtc::VideoCodecType type = CodecTypeFromName(codec.name);

  for (size_t i = 0; i < old_decoders->size(); ++i) {
    if ((*old_decoders)[i].type == type) {
      AllocatedDecoder decoder = (*old_decoders)[i];
      (*old_decoders)[i] = old_decoders->back();
      old_decoders->pop_back();
      return decoder;
    }
  }

  if (external_decoder_factory_ != NULL) {
    webrtc::VideoDecoder* decoder =
        external_decoder_factory_->CreateVideoDecoder(type);
    if (decoder != NULL) {
      return AllocatedDecoder(decoder, type, true);
    }
  }

  if (type == webrtc::kVideoCodecVP8) {
    return AllocatedDecoder(
        webrtc::VideoDecoder::Create(webrtc::VideoDecoder::kVp8), type, false);
  }

  if (type == webrtc::kVideoCodecVP9) {
    return AllocatedDecoder(
        webrtc::VideoDecoder::Create(webrtc::VideoDecoder::kVp9), type, false);
  }

  if (type == webrtc::kVideoCodecH264) {
    return AllocatedDecoder(
        webrtc::VideoDecoder::Create(webrtc::VideoDecoder::kH264), type, false);
  }

  return AllocatedDecoder(
      webrtc::VideoDecoder::Create(webrtc::VideoDecoder::kUnsupportedCodec),
      webrtc::kVideoCodecUnknown, false);
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::ConfigureCodecs(
    const std::vector<VideoCodecSettings>& recv_codecs,
    std::vector<AllocatedDecoder>* old_decoders) {
  *old_decoders = allocated_decoders_;
  allocated_decoders_.clear();
  config_.decoders.clear();
  for (size_t i = 0; i < recv_codecs.size(); ++i) {
    AllocatedDecoder allocated_decoder =
        CreateOrReuseVideoDecoder(old_decoders, recv_codecs[i].codec);
    allocated_decoders_.push_back(allocated_decoder);

    webrtc::VideoReceiveStream::Decoder decoder;
    decoder.decoder = allocated_decoder.decoder;
    decoder.payload_type = recv_codecs[i].codec.id;
    decoder.payload_name = recv_codecs[i].codec.name;
    config_.decoders.push_back(decoder);
  }

  // TODO(pbos): Reconfigure RTX based on incoming recv_codecs.
  config_.rtp.fec = recv_codecs.front().fec;
  config_.rtp.nack.rtp_history_ms =
      HasNack(recv_codecs.begin()->codec) ? kNackHistoryMs : 0;
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::SetLocalSsrc(
    uint32_t local_ssrc) {
  // TODO(pbos): Consider turning this sanity check into a RTC_DCHECK. You
  // should not be able to create a sender with the same SSRC as a receiver, but
  // right now this can't be done due to unittests depending on receiving what
  // they are sending from the same MediaChannel.
  if (local_ssrc == config_.rtp.remote_ssrc) {
    LOG(LS_INFO) << "Ignoring call to SetLocalSsrc because parameters are "
                    "unchanged; local_ssrc=" << local_ssrc;
    return;
  }

  config_.rtp.local_ssrc = local_ssrc;
  LOG(LS_INFO)
      << "RecreateWebRtcStream (recv) because of SetLocalSsrc; local_ssrc="
      << local_ssrc;
  RecreateWebRtcStream();
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::SetFeedbackParameters(
    bool nack_enabled,
    bool remb_enabled,
    bool transport_cc_enabled,
    webrtc::RtcpMode rtcp_mode) {
  int nack_history_ms = nack_enabled ? kNackHistoryMs : 0;
  if (config_.rtp.nack.rtp_history_ms == nack_history_ms &&
      config_.rtp.remb == remb_enabled &&
      config_.rtp.transport_cc == transport_cc_enabled &&
      config_.rtp.rtcp_mode == rtcp_mode) {
    LOG(LS_INFO)
        << "Ignoring call to SetFeedbackParameters because parameters are "
           "unchanged; nack="
        << nack_enabled << ", remb=" << remb_enabled
        << ", transport_cc=" << transport_cc_enabled;
    return;
  }
  config_.rtp.remb = remb_enabled;
  config_.rtp.nack.rtp_history_ms = nack_history_ms;
  config_.rtp.transport_cc = transport_cc_enabled;
  config_.rtp.rtcp_mode = rtcp_mode;
  LOG(LS_INFO)
      << "RecreateWebRtcStream (recv) because of SetFeedbackParameters; nack="
      << nack_enabled << ", remb=" << remb_enabled
      << ", transport_cc=" << transport_cc_enabled;
  RecreateWebRtcStream();
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::SetRecvParameters(
    const ChangedRecvParameters& params) {
  bool needs_recreation = false;
  std::vector<AllocatedDecoder> old_decoders;
  if (params.codec_settings) {
    ConfigureCodecs(*params.codec_settings, &old_decoders);
    needs_recreation = true;
  }
  if (params.rtp_header_extensions) {
    config_.rtp.extensions = *params.rtp_header_extensions;
    needs_recreation = true;
  }
  if (needs_recreation) {
    LOG(LS_INFO) << "RecreateWebRtcStream (recv) because of SetRecvParameters";
    RecreateWebRtcStream();
    ClearDecoders(&old_decoders);
  }
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::RecreateWebRtcStream() {
  if (stream_ != NULL) {
    call_->DestroyVideoReceiveStream(stream_);
  }
  webrtc::VideoReceiveStream::Config config = config_.Copy();
  if (red_disabled_by_remote_side_) {
    config.rtp.fec.red_payload_type = -1;
    config.rtp.fec.ulpfec_payload_type = -1;
    config.rtp.fec.red_rtx_payload_type = -1;
  }
  stream_ = call_->CreateVideoReceiveStream(std::move(config));
  stream_->Start();
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::ClearDecoders(
    std::vector<AllocatedDecoder>* allocated_decoders) {
  for (size_t i = 0; i < allocated_decoders->size(); ++i) {
    if ((*allocated_decoders)[i].external) {
      external_decoder_factory_->DestroyVideoDecoder(
          (*allocated_decoders)[i].external_decoder);
    }
    delete (*allocated_decoders)[i].decoder;
  }
  allocated_decoders->clear();
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::OnFrame(
    const webrtc::VideoFrame& frame) {
  rtc::CritScope crit(&sink_lock_);

  if (first_frame_timestamp_ < 0)
    first_frame_timestamp_ = frame.timestamp();
  int64_t rtp_time_elapsed_since_first_frame =
      (timestamp_wraparound_handler_.Unwrap(frame.timestamp()) -
       first_frame_timestamp_);
  int64_t elapsed_time_ms = rtp_time_elapsed_since_first_frame /
                            (cricket::kVideoCodecClockrate / 1000);
  if (frame.ntp_time_ms() > 0)
    estimated_remote_start_ntp_time_ms_ = frame.ntp_time_ms() - elapsed_time_ms;

  if (sink_ == NULL) {
    LOG(LS_WARNING) << "VideoReceiveStream not connected to a VideoSink.";
    return;
  }

  last_width_ = frame.width();
  last_height_ = frame.height();

  const WebRtcVideoFrame render_frame(
      frame.video_frame_buffer(), frame.rotation(),
      frame.render_time_ms() * rtc::kNumNanosecsPerMicrosec);
  sink_->OnFrame(render_frame);
}

bool WebRtcVideoChannel2::WebRtcVideoReceiveStream::IsDefaultStream() const {
  return default_stream_;
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::SetSink(
    rtc::VideoSinkInterface<cricket::VideoFrame>* sink) {
  rtc::CritScope crit(&sink_lock_);
  sink_ = sink;
}

std::string
WebRtcVideoChannel2::WebRtcVideoReceiveStream::GetCodecNameFromPayloadType(
    int payload_type) {
  for (const webrtc::VideoReceiveStream::Decoder& decoder : config_.decoders) {
    if (decoder.payload_type == payload_type) {
      return decoder.payload_name;
    }
  }
  return "";
}

VideoReceiverInfo
WebRtcVideoChannel2::WebRtcVideoReceiveStream::GetVideoReceiverInfo() {
  VideoReceiverInfo info;
  info.ssrc_groups = ssrc_groups_;
  info.add_ssrc(config_.rtp.remote_ssrc);
  webrtc::VideoReceiveStream::Stats stats = stream_->GetStats();
  info.decoder_implementation_name = stats.decoder_implementation_name;
  info.bytes_rcvd = stats.rtp_stats.transmitted.payload_bytes +
                    stats.rtp_stats.transmitted.header_bytes +
                    stats.rtp_stats.transmitted.padding_bytes;
  info.packets_rcvd = stats.rtp_stats.transmitted.packets;
  info.packets_lost = stats.rtcp_stats.cumulative_lost;
  info.fraction_lost =
      static_cast<float>(stats.rtcp_stats.fraction_lost) / (1 << 8);

  info.framerate_rcvd = stats.network_frame_rate;
  info.framerate_decoded = stats.decode_frame_rate;
  info.framerate_output = stats.render_frame_rate;

  {
    rtc::CritScope frame_cs(&sink_lock_);
    info.frame_width = last_width_;
    info.frame_height = last_height_;
    info.capture_start_ntp_time_ms = estimated_remote_start_ntp_time_ms_;
  }

  info.decode_ms = stats.decode_ms;
  info.max_decode_ms = stats.max_decode_ms;
  info.current_delay_ms = stats.current_delay_ms;
  info.target_delay_ms = stats.target_delay_ms;
  info.jitter_buffer_ms = stats.jitter_buffer_ms;
  info.min_playout_delay_ms = stats.min_playout_delay_ms;
  info.render_delay_ms = stats.render_delay_ms;

  info.codec_name = GetCodecNameFromPayloadType(stats.current_payload_type);

  info.firs_sent = stats.rtcp_packet_type_counts.fir_packets;
  info.plis_sent = stats.rtcp_packet_type_counts.pli_packets;
  info.nacks_sent = stats.rtcp_packet_type_counts.nack_packets;

  return info;
}

void WebRtcVideoChannel2::WebRtcVideoReceiveStream::SetFecDisabledRemotely(
    bool disable) {
  red_disabled_by_remote_side_ = disable;
  RecreateWebRtcStream();
}

WebRtcVideoChannel2::VideoCodecSettings::VideoCodecSettings()
    : rtx_payload_type(-1) {}

bool WebRtcVideoChannel2::VideoCodecSettings::operator==(
    const WebRtcVideoChannel2::VideoCodecSettings& other) const {
  return codec == other.codec &&
         fec.ulpfec_payload_type == other.fec.ulpfec_payload_type &&
         fec.red_payload_type == other.fec.red_payload_type &&
         fec.red_rtx_payload_type == other.fec.red_rtx_payload_type &&
         rtx_payload_type == other.rtx_payload_type;
}

bool WebRtcVideoChannel2::VideoCodecSettings::operator!=(
    const WebRtcVideoChannel2::VideoCodecSettings& other) const {
  return !(*this == other);
}

std::vector<WebRtcVideoChannel2::VideoCodecSettings>
WebRtcVideoChannel2::MapCodecs(const std::vector<VideoCodec>& codecs) {
  RTC_DCHECK(!codecs.empty());

  std::vector<VideoCodecSettings> video_codecs;
  std::map<int, bool> payload_used;
  std::map<int, VideoCodec::CodecType> payload_codec_type;
  // |rtx_mapping| maps video payload type to rtx payload type.
  std::map<int, int> rtx_mapping;

  webrtc::FecConfig fec_settings;

  for (size_t i = 0; i < codecs.size(); ++i) {
    const VideoCodec& in_codec = codecs[i];
    int payload_type = in_codec.id;

    if (payload_used[payload_type]) {
      LOG(LS_ERROR) << "Payload type already registered: "
                    << in_codec.ToString();
      return std::vector<VideoCodecSettings>();
    }
    payload_used[payload_type] = true;
    payload_codec_type[payload_type] = in_codec.GetCodecType();

    switch (in_codec.GetCodecType()) {
      case VideoCodec::CODEC_RED: {
        // RED payload type, should not have duplicates.
        RTC_DCHECK(fec_settings.red_payload_type == -1);
        fec_settings.red_payload_type = in_codec.id;
        continue;
      }

      case VideoCodec::CODEC_ULPFEC: {
        // ULPFEC payload type, should not have duplicates.
        RTC_DCHECK(fec_settings.ulpfec_payload_type == -1);
        fec_settings.ulpfec_payload_type = in_codec.id;
        continue;
      }

      case VideoCodec::CODEC_RTX: {
        int associated_payload_type;
        if (!in_codec.GetParam(kCodecParamAssociatedPayloadType,
                               &associated_payload_type) ||
            !IsValidRtpPayloadType(associated_payload_type)) {
          LOG(LS_ERROR)
              << "RTX codec with invalid or no associated payload type: "
              << in_codec.ToString();
          return std::vector<VideoCodecSettings>();
        }
        rtx_mapping[associated_payload_type] = in_codec.id;
        continue;
      }

      case VideoCodec::CODEC_VIDEO:
        break;
    }

    video_codecs.push_back(VideoCodecSettings());
    video_codecs.back().codec = in_codec;
  }

  // One of these codecs should have been a video codec. Only having FEC
  // parameters into this code is a logic error.
  RTC_DCHECK(!video_codecs.empty());

  for (std::map<int, int>::const_iterator it = rtx_mapping.begin();
       it != rtx_mapping.end();
       ++it) {
    if (!payload_used[it->first]) {
      LOG(LS_ERROR) << "RTX mapped to payload not in codec list.";
      return std::vector<VideoCodecSettings>();
    }
    if (payload_codec_type[it->first] != VideoCodec::CODEC_VIDEO &&
        payload_codec_type[it->first] != VideoCodec::CODEC_RED) {
      LOG(LS_ERROR) << "RTX not mapped to regular video codec or RED codec.";
      return std::vector<VideoCodecSettings>();
    }

    if (it->first == fec_settings.red_payload_type) {
      fec_settings.red_rtx_payload_type = it->second;
    }
  }

  for (size_t i = 0; i < video_codecs.size(); ++i) {
    video_codecs[i].fec = fec_settings;
    if (rtx_mapping[video_codecs[i].codec.id] != 0 &&
        rtx_mapping[video_codecs[i].codec.id] !=
            fec_settings.red_payload_type) {
      video_codecs[i].rtx_payload_type = rtx_mapping[video_codecs[i].codec.id];
    }
  }

  return video_codecs;
}

}  // namespace cricket
