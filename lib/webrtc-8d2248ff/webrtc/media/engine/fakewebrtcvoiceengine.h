/*
 *  Copyright (c) 2010 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVOICEENGINE_H_
#define WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVOICEENGINE_H_

#include <list>
#include <map>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/config.h"
#include "webrtc/media/base/codec.h"
#include "webrtc/media/base/rtputils.h"
#include "webrtc/media/engine/fakewebrtccommon.h"
#include "webrtc/media/engine/webrtcvoe.h"
#include "webrtc/modules/audio_coding/acm2/rent_a_codec.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"

namespace cricket {

static const int kOpusBandwidthNb = 4000;
static const int kOpusBandwidthMb = 6000;
static const int kOpusBandwidthWb = 8000;
static const int kOpusBandwidthSwb = 12000;
static const int kOpusBandwidthFb = 20000;

#define WEBRTC_CHECK_CHANNEL(channel) \
  if (channels_.find(channel) == channels_.end()) return -1;

class FakeAudioProcessing : public webrtc::AudioProcessing {
 public:
  FakeAudioProcessing() : experimental_ns_enabled_(false) {}

  WEBRTC_STUB(Initialize, ())
  WEBRTC_STUB(Initialize, (
      int input_sample_rate_hz,
      int output_sample_rate_hz,
      int reverse_sample_rate_hz,
      webrtc::AudioProcessing::ChannelLayout input_layout,
      webrtc::AudioProcessing::ChannelLayout output_layout,
      webrtc::AudioProcessing::ChannelLayout reverse_layout));
  WEBRTC_STUB(Initialize, (
      const webrtc::ProcessingConfig& processing_config));

  WEBRTC_VOID_FUNC(SetExtraOptions, (const webrtc::Config& config)) {
    experimental_ns_enabled_ = config.Get<webrtc::ExperimentalNs>().enabled;
  }

  WEBRTC_STUB_CONST(proc_sample_rate_hz, ());
  WEBRTC_STUB_CONST(proc_split_sample_rate_hz, ());
  size_t num_input_channels() const override { return 0; }
  size_t num_proc_channels() const override { return 0; }
  size_t num_output_channels() const override { return 0; }
  size_t num_reverse_channels() const override { return 0; }
  WEBRTC_VOID_STUB(set_output_will_be_muted, (bool muted));
  WEBRTC_STUB(ProcessStream, (webrtc::AudioFrame* frame));
  WEBRTC_STUB(ProcessStream, (
      const float* const* src,
      size_t samples_per_channel,
      int input_sample_rate_hz,
      webrtc::AudioProcessing::ChannelLayout input_layout,
      int output_sample_rate_hz,
      webrtc::AudioProcessing::ChannelLayout output_layout,
      float* const* dest));
  WEBRTC_STUB(ProcessStream,
              (const float* const* src,
               const webrtc::StreamConfig& input_config,
               const webrtc::StreamConfig& output_config,
               float* const* dest));
  WEBRTC_STUB(ProcessReverseStream, (webrtc::AudioFrame * frame));
  WEBRTC_STUB(AnalyzeReverseStream, (
      const float* const* data,
      size_t samples_per_channel,
      int sample_rate_hz,
      webrtc::AudioProcessing::ChannelLayout layout));
  WEBRTC_STUB(ProcessReverseStream,
              (const float* const* src,
               const webrtc::StreamConfig& reverse_input_config,
               const webrtc::StreamConfig& reverse_output_config,
               float* const* dest));
  WEBRTC_STUB(set_stream_delay_ms, (int delay));
  WEBRTC_STUB_CONST(stream_delay_ms, ());
  WEBRTC_BOOL_STUB_CONST(was_stream_delay_set, ());
  WEBRTC_VOID_STUB(set_stream_key_pressed, (bool key_pressed));
  WEBRTC_VOID_STUB(set_delay_offset_ms, (int offset));
  WEBRTC_STUB_CONST(delay_offset_ms, ());
  WEBRTC_STUB(StartDebugRecording,
              (const char filename[kMaxFilenameSize], int64_t max_size_bytes));
  WEBRTC_STUB(StartDebugRecording, (FILE * handle, int64_t max_size_bytes));
  WEBRTC_STUB(StopDebugRecording, ());
  WEBRTC_VOID_STUB(UpdateHistogramsOnCallEnd, ());
  webrtc::EchoCancellation* echo_cancellation() const override { return NULL; }
  webrtc::EchoControlMobile* echo_control_mobile() const override {
    return NULL;
  }
  webrtc::GainControl* gain_control() const override { return NULL; }
  webrtc::HighPassFilter* high_pass_filter() const override { return NULL; }
  webrtc::LevelEstimator* level_estimator() const override { return NULL; }
  webrtc::NoiseSuppression* noise_suppression() const override { return NULL; }
  webrtc::VoiceDetection* voice_detection() const override { return NULL; }

  bool experimental_ns_enabled() {
    return experimental_ns_enabled_;
  }

 private:
  bool experimental_ns_enabled_;
};

class FakeWebRtcVoiceEngine
    : public webrtc::VoEAudioProcessing,
      public webrtc::VoEBase, public webrtc::VoECodec,
      public webrtc::VoEHardware,
      public webrtc::VoEVolumeControl {
 public:
  struct Channel {
    Channel() {
      memset(&send_codec, 0, sizeof(send_codec));
    }
    bool playout = false;
    bool vad = false;
    bool codec_fec = false;
    int max_encoding_bandwidth = 0;
    bool opus_dtx = false;
    int cn8_type = 13;
    int cn16_type = 105;
    int associate_send_channel = -1;
    std::vector<webrtc::CodecInst> recv_codecs;
    webrtc::CodecInst send_codec;
    int neteq_capacity = -1;
    bool neteq_fast_accelerate = false;
  };

  FakeWebRtcVoiceEngine() {
    memset(&agc_config_, 0, sizeof(agc_config_));
  }
  ~FakeWebRtcVoiceEngine() override {
    RTC_CHECK(channels_.empty());
  }

  bool ec_metrics_enabled() const { return ec_metrics_enabled_; }

  bool IsInited() const { return inited_; }
  int GetLastChannel() const { return last_channel_; }
  int GetNumChannels() const { return static_cast<int>(channels_.size()); }
  bool GetPlayout(int channel) {
    return channels_[channel]->playout;
  }
  bool GetVAD(int channel) {
    return channels_[channel]->vad;
  }
  bool GetOpusDtx(int channel) {
    return channels_[channel]->opus_dtx;
  }
  bool GetCodecFEC(int channel) {
    return channels_[channel]->codec_fec;
  }
  int GetMaxEncodingBandwidth(int channel) {
    return channels_[channel]->max_encoding_bandwidth;
  }
  int GetSendCNPayloadType(int channel, bool wideband) {
    return (wideband) ?
        channels_[channel]->cn16_type :
        channels_[channel]->cn8_type;
  }
  void set_playout_fail_channel(int channel) {
    playout_fail_channel_ = channel;
  }
  void set_fail_create_channel(bool fail_create_channel) {
    fail_create_channel_ = fail_create_channel;
  }
  int AddChannel(const webrtc::Config& config) {
    if (fail_create_channel_) {
      return -1;
    }
    Channel* ch = new Channel();
    auto db = webrtc::acm2::RentACodec::Database();
    ch->recv_codecs.assign(db.begin(), db.end());
    if (config.Get<webrtc::NetEqCapacityConfig>().enabled) {
      ch->neteq_capacity = config.Get<webrtc::NetEqCapacityConfig>().capacity;
    }
    ch->neteq_fast_accelerate =
        config.Get<webrtc::NetEqFastAccelerate>().enabled;
    channels_[++last_channel_] = ch;
    return last_channel_;
  }

  int GetNumSetSendCodecs() const { return num_set_send_codecs_; }

  int GetAssociateSendChannel(int channel) {
    return channels_[channel]->associate_send_channel;
  }

  WEBRTC_STUB(Release, ());

  // webrtc::VoEBase
  WEBRTC_STUB(RegisterVoiceEngineObserver, (
      webrtc::VoiceEngineObserver& observer));
  WEBRTC_STUB(DeRegisterVoiceEngineObserver, ());
  WEBRTC_FUNC(Init,
              (webrtc::AudioDeviceModule* adm,
               webrtc::AudioProcessing* audioproc,
               const rtc::scoped_refptr<webrtc::AudioDecoderFactory>&
                   decoder_factory)) {
    inited_ = true;
    return 0;
  }
  WEBRTC_FUNC(Terminate, ()) {
    inited_ = false;
    return 0;
  }
  webrtc::AudioProcessing* audio_processing() override {
    return &audio_processing_;
  }
  webrtc::AudioDeviceModule* audio_device_module() override {
    return nullptr;
  }
  WEBRTC_FUNC(CreateChannel, ()) {
    webrtc::Config empty_config;
    return AddChannel(empty_config);
  }
  WEBRTC_FUNC(CreateChannel, (const webrtc::Config& config)) {
    return AddChannel(config);
  }
  WEBRTC_FUNC(DeleteChannel, (int channel)) {
    WEBRTC_CHECK_CHANNEL(channel);
    for (const auto& ch : channels_) {
      if (ch.second->associate_send_channel == channel) {
        ch.second->associate_send_channel = -1;
      }
    }
    delete channels_[channel];
    channels_.erase(channel);
    return 0;
  }
  WEBRTC_STUB(StartReceive, (int channel));
  WEBRTC_FUNC(StartPlayout, (int channel)) {
    if (playout_fail_channel_ != channel) {
      WEBRTC_CHECK_CHANNEL(channel);
      channels_[channel]->playout = true;
      return 0;
    } else {
      // When playout_fail_channel_ == channel, fail the StartPlayout on this
      // channel.
      return -1;
    }
  }
  WEBRTC_STUB(StartSend, (int channel));
  WEBRTC_STUB(StopReceive, (int channel));
  WEBRTC_FUNC(StopPlayout, (int channel)) {
    WEBRTC_CHECK_CHANNEL(channel);
    channels_[channel]->playout = false;
    return 0;
  }
  WEBRTC_STUB(StopSend, (int channel));
  WEBRTC_STUB(GetVersion, (char version[1024]));
  WEBRTC_STUB(LastError, ());
  WEBRTC_FUNC(AssociateSendChannel, (int channel,
                                     int accociate_send_channel)) {
    WEBRTC_CHECK_CHANNEL(channel);
    channels_[channel]->associate_send_channel = accociate_send_channel;
    return 0;
  }
  webrtc::RtcEventLog* GetEventLog() override { return nullptr; }

  // webrtc::VoECodec
  WEBRTC_STUB(NumOfCodecs, ());
  WEBRTC_STUB(GetCodec, (int index, webrtc::CodecInst& codec));
  WEBRTC_FUNC(SetSendCodec, (int channel, const webrtc::CodecInst& codec)) {
    WEBRTC_CHECK_CHANNEL(channel);
    // To match the behavior of the real implementation.
    if (_stricmp(codec.plname, "telephone-event") == 0 ||
        _stricmp(codec.plname, "audio/telephone-event") == 0 ||
        _stricmp(codec.plname, "CN") == 0 ||
        _stricmp(codec.plname, "red") == 0) {
      return -1;
    }
    channels_[channel]->send_codec = codec;
    ++num_set_send_codecs_;
    return 0;
  }
  WEBRTC_FUNC(GetSendCodec, (int channel, webrtc::CodecInst& codec)) {
    WEBRTC_CHECK_CHANNEL(channel);
    codec = channels_[channel]->send_codec;
    return 0;
  }
  WEBRTC_STUB(SetBitRate, (int channel, int bitrate_bps));
  WEBRTC_STUB(GetRecCodec, (int channel, webrtc::CodecInst& codec));
  WEBRTC_FUNC(SetRecPayloadType, (int channel,
                                  const webrtc::CodecInst& codec)) {
    WEBRTC_CHECK_CHANNEL(channel);
    Channel* ch = channels_[channel];
    if (ch->playout)
      return -1;  // Channel is in use.
    // Check if something else already has this slot.
    if (codec.pltype != -1) {
      for (std::vector<webrtc::CodecInst>::iterator it =
          ch->recv_codecs.begin(); it != ch->recv_codecs.end(); ++it) {
        if (it->pltype == codec.pltype &&
            _stricmp(it->plname, codec.plname) != 0) {
          return -1;
        }
      }
    }
    // Otherwise try to find this codec and update its payload type.
    int result = -1;  // not found
    for (std::vector<webrtc::CodecInst>::iterator it = ch->recv_codecs.begin();
         it != ch->recv_codecs.end(); ++it) {
      if (strcmp(it->plname, codec.plname) == 0 &&
          it->plfreq == codec.plfreq &&
          it->channels == codec.channels) {
        it->pltype = codec.pltype;
        result = 0;
      }
    }
    return result;
  }
  WEBRTC_FUNC(SetSendCNPayloadType, (int channel, int type,
                                     webrtc::PayloadFrequencies frequency)) {
    WEBRTC_CHECK_CHANNEL(channel);
    if (frequency == webrtc::kFreq8000Hz) {
      channels_[channel]->cn8_type = type;
    } else if (frequency == webrtc::kFreq16000Hz) {
      channels_[channel]->cn16_type = type;
    }
    return 0;
  }
  WEBRTC_FUNC(GetRecPayloadType, (int channel, webrtc::CodecInst& codec)) {
    WEBRTC_CHECK_CHANNEL(channel);
    Channel* ch = channels_[channel];
    for (std::vector<webrtc::CodecInst>::iterator it = ch->recv_codecs.begin();
         it != ch->recv_codecs.end(); ++it) {
      if (strcmp(it->plname, codec.plname) == 0 &&
          it->plfreq == codec.plfreq &&
          it->channels == codec.channels &&
          it->pltype != -1) {
        codec.pltype = it->pltype;
        return 0;
      }
    }
    return -1;  // not found
  }
  WEBRTC_FUNC(SetVADStatus, (int channel, bool enable, webrtc::VadModes mode,
                             bool disableDTX)) {
    WEBRTC_CHECK_CHANNEL(channel);
    if (channels_[channel]->send_codec.channels == 2) {
      // Replicating VoE behavior; VAD cannot be enabled for stereo.
      return -1;
    }
    channels_[channel]->vad = enable;
    return 0;
  }
  WEBRTC_STUB(GetVADStatus, (int channel, bool& enabled,
                             webrtc::VadModes& mode, bool& disabledDTX));

  WEBRTC_FUNC(SetFECStatus, (int channel, bool enable)) {
    WEBRTC_CHECK_CHANNEL(channel);
    if (_stricmp(channels_[channel]->send_codec.plname, "opus") != 0) {
      // Return -1 if current send codec is not Opus.
      // TODO(minyue): Excludes other codecs if they support inband FEC.
      return -1;
    }
    channels_[channel]->codec_fec = enable;
    return 0;
  }
  WEBRTC_FUNC(GetFECStatus, (int channel, bool& enable)) {
    WEBRTC_CHECK_CHANNEL(channel);
    enable = channels_[channel]->codec_fec;
    return 0;
  }

  WEBRTC_FUNC(SetOpusMaxPlaybackRate, (int channel, int frequency_hz)) {
    WEBRTC_CHECK_CHANNEL(channel);
    if (_stricmp(channels_[channel]->send_codec.plname, "opus") != 0) {
      // Return -1 if current send codec is not Opus.
      return -1;
    }
    if (frequency_hz <= 8000)
      channels_[channel]->max_encoding_bandwidth = kOpusBandwidthNb;
    else if (frequency_hz <= 12000)
      channels_[channel]->max_encoding_bandwidth = kOpusBandwidthMb;
    else if (frequency_hz <= 16000)
      channels_[channel]->max_encoding_bandwidth = kOpusBandwidthWb;
    else if (frequency_hz <= 24000)
      channels_[channel]->max_encoding_bandwidth = kOpusBandwidthSwb;
    else
      channels_[channel]->max_encoding_bandwidth = kOpusBandwidthFb;
    return 0;
  }

  WEBRTC_FUNC(SetOpusDtx, (int channel, bool enable_dtx)) {
    WEBRTC_CHECK_CHANNEL(channel);
    if (_stricmp(channels_[channel]->send_codec.plname, "opus") != 0) {
      // Return -1 if current send codec is not Opus.
      return -1;
    }
    channels_[channel]->opus_dtx = enable_dtx;
    return 0;
  }

  // webrtc::VoEHardware
  WEBRTC_STUB(GetNumOfRecordingDevices, (int& num));
  WEBRTC_STUB(GetNumOfPlayoutDevices, (int& num));
  WEBRTC_STUB(GetRecordingDeviceName, (int i, char* name, char* guid));
  WEBRTC_STUB(GetPlayoutDeviceName, (int i, char* name, char* guid));
  WEBRTC_STUB(SetRecordingDevice, (int, webrtc::StereoChannel));
  WEBRTC_STUB(SetPlayoutDevice, (int));
  WEBRTC_STUB(SetAudioDeviceLayer, (webrtc::AudioLayers));
  WEBRTC_STUB(GetAudioDeviceLayer, (webrtc::AudioLayers&));
  WEBRTC_STUB(SetRecordingSampleRate, (unsigned int samples_per_sec));
  WEBRTC_STUB_CONST(RecordingSampleRate, (unsigned int* samples_per_sec));
  WEBRTC_STUB(SetPlayoutSampleRate, (unsigned int samples_per_sec));
  WEBRTC_STUB_CONST(PlayoutSampleRate, (unsigned int* samples_per_sec));
  WEBRTC_STUB(EnableBuiltInAEC, (bool enable));
  bool BuiltInAECIsAvailable() const override { return false; }
  WEBRTC_STUB(EnableBuiltInAGC, (bool enable));
  bool BuiltInAGCIsAvailable() const override { return false; }
  WEBRTC_STUB(EnableBuiltInNS, (bool enable));
  bool BuiltInNSIsAvailable() const override { return false; }

  // webrtc::VoEVolumeControl
  WEBRTC_STUB(SetSpeakerVolume, (unsigned int));
  WEBRTC_STUB(GetSpeakerVolume, (unsigned int&));
  WEBRTC_STUB(SetMicVolume, (unsigned int));
  WEBRTC_STUB(GetMicVolume, (unsigned int&));
  WEBRTC_STUB(SetInputMute, (int, bool));
  WEBRTC_STUB(GetInputMute, (int, bool&));
  WEBRTC_STUB(GetSpeechInputLevel, (unsigned int&));
  WEBRTC_STUB(GetSpeechOutputLevel, (int, unsigned int&));
  WEBRTC_STUB(GetSpeechInputLevelFullRange, (unsigned int&));
  WEBRTC_STUB(GetSpeechOutputLevelFullRange, (int, unsigned int&));
  WEBRTC_STUB(SetChannelOutputVolumeScaling, (int channel, float scale));
  WEBRTC_STUB(GetChannelOutputVolumeScaling, (int channel, float& scale));
  WEBRTC_STUB(SetOutputVolumePan, (int channel, float left, float right));
  WEBRTC_STUB(GetOutputVolumePan, (int channel, float& left, float& right));

  // webrtc::VoEAudioProcessing
  WEBRTC_FUNC(SetNsStatus, (bool enable, webrtc::NsModes mode)) {
    ns_enabled_ = enable;
    ns_mode_ = mode;
    return 0;
  }
  WEBRTC_FUNC(GetNsStatus, (bool& enabled, webrtc::NsModes& mode)) {
    enabled = ns_enabled_;
    mode = ns_mode_;
    return 0;
  }

  WEBRTC_FUNC(SetAgcStatus, (bool enable, webrtc::AgcModes mode)) {
    agc_enabled_ = enable;
    agc_mode_ = mode;
    return 0;
  }
  WEBRTC_FUNC(GetAgcStatus, (bool& enabled, webrtc::AgcModes& mode)) {
    enabled = agc_enabled_;
    mode = agc_mode_;
    return 0;
  }

  WEBRTC_FUNC(SetAgcConfig, (webrtc::AgcConfig config)) {
    agc_config_ = config;
    return 0;
  }
  WEBRTC_FUNC(GetAgcConfig, (webrtc::AgcConfig& config)) {
    config = agc_config_;
    return 0;
  }
  WEBRTC_FUNC(SetEcStatus, (bool enable, webrtc::EcModes mode)) {
    ec_enabled_ = enable;
    ec_mode_ = mode;
    return 0;
  }
  WEBRTC_FUNC(GetEcStatus, (bool& enabled, webrtc::EcModes& mode)) {
    enabled = ec_enabled_;
    mode = ec_mode_;
    return 0;
  }
  WEBRTC_STUB(EnableDriftCompensation, (bool enable))
  WEBRTC_BOOL_STUB(DriftCompensationEnabled, ())
  WEBRTC_VOID_STUB(SetDelayOffsetMs, (int offset))
  WEBRTC_STUB(DelayOffsetMs, ());
  WEBRTC_FUNC(SetAecmMode, (webrtc::AecmModes mode, bool enableCNG)) {
    aecm_mode_ = mode;
    cng_enabled_ = enableCNG;
    return 0;
  }
  WEBRTC_FUNC(GetAecmMode, (webrtc::AecmModes& mode, bool& enabledCNG)) {
    mode = aecm_mode_;
    enabledCNG = cng_enabled_;
    return 0;
  }
  WEBRTC_STUB(SetRxNsStatus, (int channel, bool enable, webrtc::NsModes mode));
  WEBRTC_STUB(GetRxNsStatus, (int channel, bool& enabled,
                              webrtc::NsModes& mode));
  WEBRTC_STUB(SetRxAgcStatus, (int channel, bool enable,
                               webrtc::AgcModes mode));
  WEBRTC_STUB(GetRxAgcStatus, (int channel, bool& enabled,
                               webrtc::AgcModes& mode));
  WEBRTC_STUB(SetRxAgcConfig, (int channel, webrtc::AgcConfig config));
  WEBRTC_STUB(GetRxAgcConfig, (int channel, webrtc::AgcConfig& config));

  WEBRTC_STUB(RegisterRxVadObserver, (int, webrtc::VoERxVadCallback&));
  WEBRTC_STUB(DeRegisterRxVadObserver, (int channel));
  WEBRTC_STUB(VoiceActivityIndicator, (int channel));
  WEBRTC_FUNC(SetEcMetricsStatus, (bool enable)) {
    ec_metrics_enabled_ = enable;
    return 0;
  }
  WEBRTC_STUB(GetEcMetricsStatus, (bool& enabled));
  WEBRTC_STUB(GetEchoMetrics, (int& ERL, int& ERLE, int& RERL, int& A_NLP));
  WEBRTC_STUB(GetEcDelayMetrics, (int& delay_median, int& delay_std,
      float& fraction_poor_delays));

  WEBRTC_STUB(StartDebugRecording, (const char* fileNameUTF8));
  WEBRTC_STUB(StartDebugRecording, (FILE* handle));
  WEBRTC_STUB(StopDebugRecording, ());

  WEBRTC_FUNC(SetTypingDetectionStatus, (bool enable)) {
    typing_detection_enabled_ = enable;
    return 0;
  }
  WEBRTC_FUNC(GetTypingDetectionStatus, (bool& enabled)) {
    enabled = typing_detection_enabled_;
    return 0;
  }

  WEBRTC_STUB(TimeSinceLastTyping, (int& seconds));
  WEBRTC_STUB(SetTypingDetectionParameters, (int timeWindow,
                                             int costPerTyping,
                                             int reportingThreshold,
                                             int penaltyDecay,
                                             int typeEventDelay));
  int EnableHighPassFilter(bool enable) override {
    highpass_filter_enabled_ = enable;
    return 0;
  }
  bool IsHighPassFilterEnabled() override {
    return highpass_filter_enabled_;
  }
  bool IsStereoChannelSwappingEnabled() override {
    return stereo_swapping_enabled_;
  }
  void EnableStereoChannelSwapping(bool enable) override {
    stereo_swapping_enabled_ = enable;
  }
  int GetNetEqCapacity() const {
    auto ch = channels_.find(last_channel_);
    ASSERT(ch != channels_.end());
    return ch->second->neteq_capacity;
  }
  bool GetNetEqFastAccelerate() const {
    auto ch = channels_.find(last_channel_);
    ASSERT(ch != channels_.end());
    return ch->second->neteq_fast_accelerate;
  }

 private:
  bool inited_ = false;
  int last_channel_ = -1;
  std::map<int, Channel*> channels_;
  bool fail_create_channel_ = false;
  int num_set_send_codecs_ = 0;  // how many times we call SetSendCodec().
  bool ec_enabled_ = false;
  bool ec_metrics_enabled_ = false;
  bool cng_enabled_ = false;
  bool ns_enabled_ = false;
  bool agc_enabled_ = false;
  bool highpass_filter_enabled_ = false;
  bool stereo_swapping_enabled_ = false;
  bool typing_detection_enabled_ = false;
  webrtc::EcModes ec_mode_ = webrtc::kEcDefault;
  webrtc::AecmModes aecm_mode_ = webrtc::kAecmSpeakerphone;
  webrtc::NsModes ns_mode_ = webrtc::kNsDefault;
  webrtc::AgcModes agc_mode_ = webrtc::kAgcDefault;
  webrtc::AgcConfig agc_config_;
  int playout_fail_channel_ = -1;
  FakeAudioProcessing audio_processing_;
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVOICEENGINE_H_
