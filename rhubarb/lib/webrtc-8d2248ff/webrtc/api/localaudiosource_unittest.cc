/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/localaudiosource.h"

#include <string>
#include <vector>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/base/gunit.h"
#include "webrtc/media/base/fakemediaengine.h"
#include "webrtc/media/base/fakevideorenderer.h"

using webrtc::LocalAudioSource;
using webrtc::MediaConstraintsInterface;
using webrtc::MediaSourceInterface;
using webrtc::PeerConnectionFactoryInterface;

TEST(LocalAudioSourceTest, SetValidOptions) {
  webrtc::FakeConstraints constraints;
  constraints.AddMandatory(
      MediaConstraintsInterface::kGoogEchoCancellation, false);
  constraints.AddOptional(
      MediaConstraintsInterface::kExtendedFilterEchoCancellation, true);
  constraints.AddOptional(MediaConstraintsInterface::kDAEchoCancellation, true);
  constraints.AddOptional(MediaConstraintsInterface::kAutoGainControl, true);
  constraints.AddOptional(
      MediaConstraintsInterface::kExperimentalAutoGainControl, true);
  constraints.AddMandatory(MediaConstraintsInterface::kNoiseSuppression, false);
  constraints.AddOptional(MediaConstraintsInterface::kHighpassFilter, true);

  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               &constraints);

  EXPECT_EQ(rtc::Optional<bool>(false), source->options().echo_cancellation);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().extended_filter_aec);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().delay_agnostic_aec);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().auto_gain_control);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().experimental_agc);
  EXPECT_EQ(rtc::Optional<bool>(false), source->options().noise_suppression);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().highpass_filter);
}

TEST(LocalAudioSourceTest, OptionNotSet) {
  webrtc::FakeConstraints constraints;
  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               &constraints);
  EXPECT_EQ(rtc::Optional<bool>(), source->options().highpass_filter);
}

TEST(LocalAudioSourceTest, MandatoryOverridesOptional) {
  webrtc::FakeConstraints constraints;
  constraints.AddMandatory(
      MediaConstraintsInterface::kGoogEchoCancellation, false);
  constraints.AddOptional(
      MediaConstraintsInterface::kGoogEchoCancellation, true);

  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               &constraints);

  EXPECT_EQ(rtc::Optional<bool>(false), source->options().echo_cancellation);
}

TEST(LocalAudioSourceTest, InvalidOptional) {
  webrtc::FakeConstraints constraints;
  constraints.AddOptional(MediaConstraintsInterface::kHighpassFilter, false);
  constraints.AddOptional("invalidKey", false);

  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               &constraints);

  EXPECT_EQ(MediaSourceInterface::kLive, source->state());
  EXPECT_EQ(rtc::Optional<bool>(false), source->options().highpass_filter);
}

TEST(LocalAudioSourceTest, InvalidMandatory) {
  webrtc::FakeConstraints constraints;
  constraints.AddMandatory(MediaConstraintsInterface::kHighpassFilter, false);
  constraints.AddMandatory("invalidKey", false);

  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               &constraints);

  EXPECT_EQ(MediaSourceInterface::kLive, source->state());
  EXPECT_EQ(rtc::Optional<bool>(false), source->options().highpass_filter);
}

TEST(LocalAudioSourceTest, InitWithAudioOptions) {
  cricket::AudioOptions audio_options;
  audio_options.highpass_filter = rtc::Optional<bool>(true);
  rtc::scoped_refptr<LocalAudioSource> source = LocalAudioSource::Create(
      PeerConnectionFactoryInterface::Options(), &audio_options);
  EXPECT_EQ(rtc::Optional<bool>(true), source->options().highpass_filter);
}

TEST(LocalAudioSourceTest, InitWithNoOptions) {
  rtc::scoped_refptr<LocalAudioSource> source =
      LocalAudioSource::Create(PeerConnectionFactoryInterface::Options(),
                               (cricket::AudioOptions*)nullptr);
  EXPECT_EQ(rtc::Optional<bool>(), source->options().highpass_filter);
}
