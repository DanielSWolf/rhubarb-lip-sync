/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/test/vcm_capturer.h"

#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {
namespace test {

VcmCapturer::VcmCapturer(webrtc::VideoCaptureInput* input)
    : VideoCapturer(input), started_(false), vcm_(NULL) {
}

bool VcmCapturer::Init(size_t width, size_t height, size_t target_fps) {
  VideoCaptureModule::DeviceInfo* device_info =
      VideoCaptureFactory::CreateDeviceInfo(42);  // Any ID (42) will do.

  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(0, device_name, sizeof(device_name),
                                 unique_name, sizeof(unique_name)) !=
      0) {
    Destroy();
    return false;
  }

  vcm_ = webrtc::VideoCaptureFactory::Create(0, unique_name);
  vcm_->RegisterCaptureDataCallback(*this);

  device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);
  delete device_info;

  capability_.width = static_cast<int32_t>(width);
  capability_.height = static_cast<int32_t>(height);
  capability_.maxFPS = static_cast<int32_t>(target_fps);
  capability_.rawType = kVideoI420;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  assert(vcm_->CaptureStarted());

  return true;
}

VcmCapturer* VcmCapturer::Create(VideoCaptureInput* input,
                                 size_t width,
                                 size_t height,
                                 size_t target_fps) {
  VcmCapturer* vcm_capturer = new VcmCapturer(input);
  if (!vcm_capturer->Init(width, height, target_fps)) {
    // TODO(pbos): Log a warning that this failed.
    delete vcm_capturer;
    return NULL;
  }
  return vcm_capturer;
}


void VcmCapturer::Start() {
  rtc::CritScope lock(&crit_);
  started_ = true;
}

void VcmCapturer::Stop() {
  rtc::CritScope lock(&crit_);
  started_ = false;
}

void VcmCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  // Release reference to VCM.
  vcm_ = nullptr;
}

VcmCapturer::~VcmCapturer() { Destroy(); }

void VcmCapturer::OnIncomingCapturedFrame(const int32_t id,
                                          const VideoFrame& frame) {
  rtc::CritScope lock(&crit_);
  if (started_)
    input_->IncomingCapturedFrame(frame);
}

void VcmCapturer::OnCaptureDelayChanged(const int32_t id, const int32_t delay) {
}
}  // test
}  // webrtc
