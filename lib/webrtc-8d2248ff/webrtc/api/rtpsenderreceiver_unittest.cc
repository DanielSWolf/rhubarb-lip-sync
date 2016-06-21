/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <string>
#include <utility>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/api/audiotrack.h"
#include "webrtc/api/mediastream.h"
#include "webrtc/api/remoteaudiosource.h"
#include "webrtc/api/rtpreceiver.h"
#include "webrtc/api/rtpsender.h"
#include "webrtc/api/streamcollection.h"
#include "webrtc/api/test/fakevideotracksource.h"
#include "webrtc/api/videotracksource.h"
#include "webrtc/api/videotrack.h"
#include "webrtc/base/gunit.h"
#include "webrtc/media/base/mediachannel.h"

using ::testing::_;
using ::testing::Exactly;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

static const char kStreamLabel1[] = "local_stream_1";
static const char kVideoTrackId[] = "video_1";
static const char kAudioTrackId[] = "audio_1";
static const uint32_t kVideoSsrc = 98;
static const uint32_t kVideoSsrc2 = 100;
static const uint32_t kAudioSsrc = 99;
static const uint32_t kAudioSsrc2 = 101;

namespace webrtc {

// Helper class to test RtpSender/RtpReceiver.
class MockAudioProvider : public AudioProviderInterface {
 public:
  // TODO(nisse): Valid overrides commented out, because the gmock
  // methods don't use any override declarations, and we want to avoid
  // warnings from -Winconsistent-missing-override. See
  // http://crbug.com/428099.
  ~MockAudioProvider() /* override */ {}

  MOCK_METHOD2(SetAudioPlayout,
               void(uint32_t ssrc,
                    bool enable));
  MOCK_METHOD4(SetAudioSend,
               void(uint32_t ssrc,
                    bool enable,
                    const cricket::AudioOptions& options,
                    cricket::AudioSource* source));
  MOCK_METHOD2(SetAudioPlayoutVolume, void(uint32_t ssrc, double volume));
  MOCK_CONST_METHOD1(GetAudioRtpSendParameters, RtpParameters(uint32_t ssrc));
  MOCK_METHOD2(SetAudioRtpSendParameters,
               bool(uint32_t ssrc, const RtpParameters&));
  MOCK_CONST_METHOD1(GetAudioRtpReceiveParameters,
                     RtpParameters(uint32_t ssrc));
  MOCK_METHOD2(SetAudioRtpReceiveParameters,
               bool(uint32_t ssrc, const RtpParameters&));

  void SetRawAudioSink(
      uint32_t, std::unique_ptr<AudioSinkInterface> sink) /* override */ {
    sink_ = std::move(sink);
  }

 private:
  std::unique_ptr<AudioSinkInterface> sink_;
};

// Helper class to test RtpSender/RtpReceiver.
class MockVideoProvider : public VideoProviderInterface {
 public:
  virtual ~MockVideoProvider() {}
  MOCK_METHOD3(SetVideoPlayout,
               void(uint32_t ssrc,
                    bool enable,
                    rtc::VideoSinkInterface<cricket::VideoFrame>* sink));
  MOCK_METHOD4(SetVideoSend,
               void(uint32_t ssrc,
                    bool enable,
                    const cricket::VideoOptions* options,
                    rtc::VideoSourceInterface<cricket::VideoFrame>* source));

  MOCK_CONST_METHOD1(GetVideoRtpSendParameters, RtpParameters(uint32_t ssrc));
  MOCK_METHOD2(SetVideoRtpSendParameters,
               bool(uint32_t ssrc, const RtpParameters&));
  MOCK_CONST_METHOD1(GetVideoRtpReceiveParameters,
                     RtpParameters(uint32_t ssrc));
  MOCK_METHOD2(SetVideoRtpReceiveParameters,
               bool(uint32_t ssrc, const RtpParameters&));
};

class RtpSenderReceiverTest : public testing::Test {
 public:
  virtual void SetUp() {
    stream_ = MediaStream::Create(kStreamLabel1);
  }

  void AddVideoTrack() {
    rtc::scoped_refptr<VideoTrackSourceInterface> source(
        FakeVideoTrackSource::Create());
    video_track_ = VideoTrack::Create(kVideoTrackId, source);
    EXPECT_TRUE(stream_->AddTrack(video_track_));
  }

  void CreateAudioRtpSender() {
    audio_track_ = AudioTrack::Create(kAudioTrackId, NULL);
    EXPECT_TRUE(stream_->AddTrack(audio_track_));
    EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
    audio_rtp_sender_ =
        new AudioRtpSender(stream_->GetAudioTracks()[0], stream_->label(),
                           &audio_provider_, nullptr);
    audio_rtp_sender_->SetSsrc(kAudioSsrc);
  }

  void CreateVideoRtpSender() {
    AddVideoTrack();
    EXPECT_CALL(video_provider_,
                SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
    video_rtp_sender_ = new VideoRtpSender(stream_->GetVideoTracks()[0],
                                           stream_->label(), &video_provider_);
    video_rtp_sender_->SetSsrc(kVideoSsrc);
  }

  void DestroyAudioRtpSender() {
    EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _))
        .Times(1);
    audio_rtp_sender_ = nullptr;
  }

  void DestroyVideoRtpSender() {
    EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
        .Times(1);
    video_rtp_sender_ = nullptr;
  }

  void CreateAudioRtpReceiver() {
    audio_track_ = AudioTrack::Create(
        kAudioTrackId, RemoteAudioSource::Create(kAudioSsrc, NULL));
    EXPECT_TRUE(stream_->AddTrack(audio_track_));
    EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, true));
    audio_rtp_receiver_ = new AudioRtpReceiver(stream_, kAudioTrackId,
                                               kAudioSsrc, &audio_provider_);
    audio_track_ = audio_rtp_receiver_->audio_track();
  }

  void CreateVideoRtpReceiver() {
    EXPECT_CALL(video_provider_, SetVideoPlayout(kVideoSsrc, true, _));
    video_rtp_receiver_ =
        new VideoRtpReceiver(stream_, kVideoTrackId, rtc::Thread::Current(),
                             kVideoSsrc, &video_provider_);
    video_track_ = video_rtp_receiver_->video_track();
  }

  void DestroyAudioRtpReceiver() {
    EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, false));
    audio_rtp_receiver_ = nullptr;
  }

  void DestroyVideoRtpReceiver() {
    EXPECT_CALL(video_provider_, SetVideoPlayout(kVideoSsrc, false, NULL));
    video_rtp_receiver_ = nullptr;
  }

 protected:
  MockAudioProvider audio_provider_;
  MockVideoProvider video_provider_;
  rtc::scoped_refptr<AudioRtpSender> audio_rtp_sender_;
  rtc::scoped_refptr<VideoRtpSender> video_rtp_sender_;
  rtc::scoped_refptr<AudioRtpReceiver> audio_rtp_receiver_;
  rtc::scoped_refptr<VideoRtpReceiver> video_rtp_receiver_;
  rtc::scoped_refptr<MediaStreamInterface> stream_;
  rtc::scoped_refptr<VideoTrackInterface> video_track_;
  rtc::scoped_refptr<AudioTrackInterface> audio_track_;
};

// Test that |audio_provider_| is notified when an audio track is associated
// and disassociated with an AudioRtpSender.
TEST_F(RtpSenderReceiverTest, AddAndDestroyAudioRtpSender) {
  CreateAudioRtpSender();
  DestroyAudioRtpSender();
}

// Test that |video_provider_| is notified when a video track is associated and
// disassociated with a VideoRtpSender.
TEST_F(RtpSenderReceiverTest, AddAndDestroyVideoRtpSender) {
  CreateVideoRtpSender();
  DestroyVideoRtpSender();
}

// Test that |audio_provider_| is notified when a remote audio and track is
// associated and disassociated with an AudioRtpReceiver.
TEST_F(RtpSenderReceiverTest, AddAndDestroyAudioRtpReceiver) {
  CreateAudioRtpReceiver();
  DestroyAudioRtpReceiver();
}

// Test that |video_provider_| is notified when a remote
// video track is associated and disassociated with a VideoRtpReceiver.
TEST_F(RtpSenderReceiverTest, AddAndDestroyVideoRtpReceiver) {
  CreateVideoRtpReceiver();
  DestroyVideoRtpReceiver();
}

TEST_F(RtpSenderReceiverTest, LocalAudioTrackDisable) {
  CreateAudioRtpSender();

  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _));
  audio_track_->set_enabled(false);

  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  audio_track_->set_enabled(true);

  DestroyAudioRtpSender();
}

TEST_F(RtpSenderReceiverTest, RemoteAudioTrackDisable) {
  CreateAudioRtpReceiver();

  EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, false));
  audio_track_->set_enabled(false);

  EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, true));
  audio_track_->set_enabled(true);

  DestroyAudioRtpReceiver();
}

TEST_F(RtpSenderReceiverTest, LocalVideoTrackDisable) {
  CreateVideoRtpSender();

  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, false, _, video_track_.get()));
  video_track_->set_enabled(false);

  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
  video_track_->set_enabled(true);

  DestroyVideoRtpSender();
}

TEST_F(RtpSenderReceiverTest, RemoteVideoTrackState) {
  CreateVideoRtpReceiver();

  EXPECT_EQ(webrtc::MediaStreamTrackInterface::kLive, video_track_->state());
  EXPECT_EQ(webrtc::MediaSourceInterface::kLive,
            video_track_->GetSource()->state());

  DestroyVideoRtpReceiver();

  EXPECT_EQ(webrtc::MediaStreamTrackInterface::kEnded, video_track_->state());
  EXPECT_EQ(webrtc::MediaSourceInterface::kEnded,
            video_track_->GetSource()->state());
}

TEST_F(RtpSenderReceiverTest, RemoteVideoTrackDisable) {
  CreateVideoRtpReceiver();

  video_track_->set_enabled(false);

  video_track_->set_enabled(true);

  DestroyVideoRtpReceiver();
}

TEST_F(RtpSenderReceiverTest, RemoteAudioTrackSetVolume) {
  CreateAudioRtpReceiver();

  double volume = 0.5;
  EXPECT_CALL(audio_provider_, SetAudioPlayoutVolume(kAudioSsrc, volume));
  audio_track_->GetSource()->SetVolume(volume);

  // Disable the audio track, this should prevent setting the volume.
  EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, false));
  audio_track_->set_enabled(false);
  audio_track_->GetSource()->SetVolume(1.0);

  EXPECT_CALL(audio_provider_, SetAudioPlayout(kAudioSsrc, true));
  audio_track_->set_enabled(true);

  double new_volume = 0.8;
  EXPECT_CALL(audio_provider_, SetAudioPlayoutVolume(kAudioSsrc, new_volume));
  audio_track_->GetSource()->SetVolume(new_volume);

  DestroyAudioRtpReceiver();
}

// Test that provider methods aren't called without both a track and an SSRC.
TEST_F(RtpSenderReceiverTest, AudioSenderWithoutTrackAndSsrc) {
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(&audio_provider_, nullptr);
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  EXPECT_TRUE(sender->SetTrack(track));
  EXPECT_TRUE(sender->SetTrack(nullptr));
  sender->SetSsrc(kAudioSsrc);
  sender->SetSsrc(0);
  // Just let it get destroyed and make sure it doesn't call any methods on the
  // provider interface.
}

// Test that provider methods aren't called without both a track and an SSRC.
TEST_F(RtpSenderReceiverTest, VideoSenderWithoutTrackAndSsrc) {
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(&video_provider_);
  EXPECT_TRUE(sender->SetTrack(video_track_));
  EXPECT_TRUE(sender->SetTrack(nullptr));
  sender->SetSsrc(kVideoSsrc);
  sender->SetSsrc(0);
  // Just let it get destroyed and make sure it doesn't call any methods on the
  // provider interface.
}

// Test that an audio sender calls the expected methods on the provider once
// it has a track and SSRC, when the SSRC is set first.
TEST_F(RtpSenderReceiverTest, AudioSenderEarlyWarmupSsrcThenTrack) {
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(&audio_provider_, nullptr);
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  sender->SetSsrc(kAudioSsrc);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  sender->SetTrack(track);

  // Calls expected from destructor.
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _)).Times(1);
}

// Test that an audio sender calls the expected methods on the provider once
// it has a track and SSRC, when the SSRC is set last.
TEST_F(RtpSenderReceiverTest, AudioSenderEarlyWarmupTrackThenSsrc) {
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(&audio_provider_, nullptr);
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  sender->SetTrack(track);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  sender->SetSsrc(kAudioSsrc);

  // Calls expected from destructor.
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _)).Times(1);
}

// Test that a video sender calls the expected methods on the provider once
// it has a track and SSRC, when the SSRC is set first.
TEST_F(RtpSenderReceiverTest, VideoSenderEarlyWarmupSsrcThenTrack) {
  AddVideoTrack();
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(&video_provider_);
  sender->SetSsrc(kVideoSsrc);
  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
  sender->SetTrack(video_track_);

  // Calls expected from destructor.
  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
      .Times(1);
}

// Test that a video sender calls the expected methods on the provider once
// it has a track and SSRC, when the SSRC is set last.
TEST_F(RtpSenderReceiverTest, VideoSenderEarlyWarmupTrackThenSsrc) {
  AddVideoTrack();
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(&video_provider_);
  sender->SetTrack(video_track_);
  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
  sender->SetSsrc(kVideoSsrc);

  // Calls expected from destructor.
  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
      .Times(1);
}

// Test that the sender is disconnected from the provider when its SSRC is
// set to 0.
TEST_F(RtpSenderReceiverTest, AudioSenderSsrcSetToZero) {
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(track, kStreamLabel1, &audio_provider_, nullptr);
  sender->SetSsrc(kAudioSsrc);

  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _)).Times(1);
  sender->SetSsrc(0);

  // Make sure it's SetSsrc that called methods on the provider, and not the
  // destructor.
  EXPECT_CALL(audio_provider_, SetAudioSend(_, _, _, _)).Times(0);
}

// Test that the sender is disconnected from the provider when its SSRC is
// set to 0.
TEST_F(RtpSenderReceiverTest, VideoSenderSsrcSetToZero) {
  AddVideoTrack();
  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(video_track_, kStreamLabel1, &video_provider_);
  sender->SetSsrc(kVideoSsrc);

  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
      .Times(1);
  sender->SetSsrc(0);

  // Make sure it's SetSsrc that called methods on the provider, and not the
  // destructor.
  EXPECT_CALL(video_provider_, SetVideoSend(_, _, _, _)).Times(0);
}

TEST_F(RtpSenderReceiverTest, AudioSenderTrackSetToNull) {
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(track, kStreamLabel1, &audio_provider_, nullptr);
  sender->SetSsrc(kAudioSsrc);

  // Expect that SetAudioSend will be called before the reference to the track
  // is released.
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, nullptr))
      .Times(1)
      .WillOnce(InvokeWithoutArgs([&track] {
        EXPECT_LT(2, track->AddRef());
        track->Release();
      }));
  EXPECT_TRUE(sender->SetTrack(nullptr));

  // Make sure it's SetTrack that called methods on the provider, and not the
  // destructor.
  EXPECT_CALL(audio_provider_, SetAudioSend(_, _, _, _)).Times(0);
}

TEST_F(RtpSenderReceiverTest, VideoSenderTrackSetToNull) {
  rtc::scoped_refptr<VideoTrackSourceInterface> source(
      FakeVideoTrackSource::Create());
  rtc::scoped_refptr<VideoTrackInterface> track =
      VideoTrack::Create(kVideoTrackId, source);
  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, true, _, track.get()));
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(track, kStreamLabel1, &video_provider_);
  sender->SetSsrc(kVideoSsrc);

  // Expect that SetVideoSend will be called before the reference to the track
  // is released.
  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
      .Times(1)
      .WillOnce(InvokeWithoutArgs([&track] {
        EXPECT_LT(2, track->AddRef());
        track->Release();
      }));
  EXPECT_TRUE(sender->SetTrack(nullptr));

  // Make sure it's SetTrack that called methods on the provider, and not the
  // destructor.
  EXPECT_CALL(video_provider_, SetVideoSend(_, _, _, _)).Times(0);
}

TEST_F(RtpSenderReceiverTest, AudioSenderSsrcChanged) {
  AddVideoTrack();
  rtc::scoped_refptr<AudioTrackInterface> track =
      AudioTrack::Create(kAudioTrackId, nullptr);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, true, _, _));
  rtc::scoped_refptr<AudioRtpSender> sender =
      new AudioRtpSender(track, kStreamLabel1, &audio_provider_, nullptr);
  sender->SetSsrc(kAudioSsrc);

  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc, false, _, _)).Times(1);
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc2, true, _, _)).Times(1);
  sender->SetSsrc(kAudioSsrc2);

  // Calls expected from destructor.
  EXPECT_CALL(audio_provider_, SetAudioSend(kAudioSsrc2, false, _, _)).Times(1);
}

TEST_F(RtpSenderReceiverTest, VideoSenderSsrcChanged) {
  AddVideoTrack();
  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc, true, _, video_track_.get()));
  rtc::scoped_refptr<VideoRtpSender> sender =
      new VideoRtpSender(video_track_, kStreamLabel1, &video_provider_);
  sender->SetSsrc(kVideoSsrc);

  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc, false, _, nullptr))
      .Times(1);
  EXPECT_CALL(video_provider_,
              SetVideoSend(kVideoSsrc2, true, _, video_track_.get()))
      .Times(1);
  sender->SetSsrc(kVideoSsrc2);

  // Calls expected from destructor.
  EXPECT_CALL(video_provider_, SetVideoSend(kVideoSsrc2, false, _, nullptr))
      .Times(1);
}

TEST_F(RtpSenderReceiverTest, AudioSenderCanSetParameters) {
  CreateAudioRtpSender();

  EXPECT_CALL(audio_provider_, GetAudioRtpSendParameters(kAudioSsrc))
      .WillOnce(Return(RtpParameters()));
  EXPECT_CALL(audio_provider_, SetAudioRtpSendParameters(kAudioSsrc, _))
      .WillOnce(Return(true));
  RtpParameters params = audio_rtp_sender_->GetParameters();
  EXPECT_TRUE(audio_rtp_sender_->SetParameters(params));

  DestroyAudioRtpSender();
}

TEST_F(RtpSenderReceiverTest, VideoSenderCanSetParameters) {
  CreateVideoRtpSender();

  EXPECT_CALL(video_provider_, GetVideoRtpSendParameters(kVideoSsrc))
      .WillOnce(Return(RtpParameters()));
  EXPECT_CALL(video_provider_, SetVideoRtpSendParameters(kVideoSsrc, _))
      .WillOnce(Return(true));
  RtpParameters params = video_rtp_sender_->GetParameters();
  EXPECT_TRUE(video_rtp_sender_->SetParameters(params));

  DestroyVideoRtpSender();
}

TEST_F(RtpSenderReceiverTest, AudioReceiverCanSetParameters) {
  CreateAudioRtpReceiver();

  EXPECT_CALL(audio_provider_, GetAudioRtpReceiveParameters(kAudioSsrc))
      .WillOnce(Return(RtpParameters()));
  EXPECT_CALL(audio_provider_, SetAudioRtpReceiveParameters(kAudioSsrc, _))
      .WillOnce(Return(true));
  RtpParameters params = audio_rtp_receiver_->GetParameters();
  EXPECT_TRUE(audio_rtp_receiver_->SetParameters(params));

  DestroyAudioRtpReceiver();
}

TEST_F(RtpSenderReceiverTest, VideoReceiverCanSetParameters) {
  CreateVideoRtpReceiver();

  EXPECT_CALL(video_provider_, GetVideoRtpReceiveParameters(kVideoSsrc))
      .WillOnce(Return(RtpParameters()));
  EXPECT_CALL(video_provider_, SetVideoRtpReceiveParameters(kVideoSsrc, _))
      .WillOnce(Return(true));
  RtpParameters params = video_rtp_receiver_->GetParameters();
  EXPECT_TRUE(video_rtp_receiver_->SetParameters(params));

  DestroyVideoRtpReceiver();
}

}  // namespace webrtc
