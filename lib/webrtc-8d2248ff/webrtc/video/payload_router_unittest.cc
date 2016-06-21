/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/mocks/mock_rtp_rtcp.h"
#include "webrtc/modules/video_coding/include/video_codec_interface.h"
#include "webrtc/video/payload_router.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace webrtc {

TEST(PayloadRouterTest, SendOnOneModule) {
  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules(1, &rtp);
  std::vector<VideoStream> streams(1);

  uint8_t payload = 'a';
  int8_t payload_type = 96;
  EncodedImage encoded_image;
  encoded_image._timeStamp = 1;
  encoded_image.capture_time_ms_ = 2;
  encoded_image._frameType = kVideoFrameKey;
  encoded_image._buffer = &payload;
  encoded_image._length = 1;

  PayloadRouter payload_router(modules, payload_type);
  payload_router.SetSendStreams(streams);

  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, payload_type,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _))
      .Times(0);
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, nullptr, nullptr));

  payload_router.set_active(true);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, payload_type,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _))
      .Times(1);
  EXPECT_EQ(0, payload_router.Encoded(encoded_image, nullptr, nullptr));

  payload_router.set_active(false);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, payload_type,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _))
      .Times(0);
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, nullptr, nullptr));

  payload_router.set_active(true);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, payload_type,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _))
      .Times(1);
  EXPECT_EQ(0, payload_router.Encoded(encoded_image, nullptr, nullptr));

  streams.clear();
  payload_router.SetSendStreams(streams);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, payload_type,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _))
      .Times(0);
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, nullptr, nullptr));
}

TEST(PayloadRouterTest, SendSimulcast) {
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules;
  modules.push_back(&rtp_1);
  modules.push_back(&rtp_2);
  std::vector<VideoStream> streams(2);

  int8_t payload_type = 96;
  uint8_t payload = 'a';
  EncodedImage encoded_image;
  encoded_image._timeStamp = 1;
  encoded_image.capture_time_ms_ = 2;
  encoded_image._frameType = kVideoFrameKey;
  encoded_image._buffer = &payload;
  encoded_image._length = 1;

  PayloadRouter payload_router(modules, payload_type);
  payload_router.SetSendStreams(streams);

  CodecSpecificInfo codec_info_1;
  memset(&codec_info_1, 0, sizeof(CodecSpecificInfo));
  codec_info_1.codecType = kVideoCodecVP8;
  codec_info_1.codecSpecific.VP8.simulcastIdx = 0;

  payload_router.set_active(true);
  EXPECT_CALL(rtp_1, SendOutgoingData(encoded_image._frameType, payload_type,
                                      encoded_image._timeStamp,
                                      encoded_image.capture_time_ms_, &payload,
                                      encoded_image._length, nullptr, _))
      .Times(1);
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_EQ(0, payload_router.Encoded(encoded_image, &codec_info_1, nullptr));

  CodecSpecificInfo codec_info_2;
  memset(&codec_info_2, 0, sizeof(CodecSpecificInfo));
  codec_info_2.codecType = kVideoCodecVP8;
  codec_info_2.codecSpecific.VP8.simulcastIdx = 1;

  EXPECT_CALL(rtp_2, SendOutgoingData(encoded_image._frameType, payload_type,
                                      encoded_image._timeStamp,
                                      encoded_image.capture_time_ms_, &payload,
                                      encoded_image._length, nullptr, _))
      .Times(1);
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_EQ(0, payload_router.Encoded(encoded_image, &codec_info_2, nullptr));

  // Inactive.
  payload_router.set_active(false);
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, &codec_info_1, nullptr));
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, &codec_info_2, nullptr));

  // Invalid simulcast index.
  streams.pop_back();  // Remove a stream.
  payload_router.SetSendStreams(streams);
  payload_router.set_active(true);
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _))
      .Times(0);
  codec_info_2.codecSpecific.VP8.simulcastIdx = 1;
  EXPECT_EQ(-1, payload_router.Encoded(encoded_image, &codec_info_2, nullptr));
}

TEST(PayloadRouterTest, MaxPayloadLength) {
  // Without any limitations from the modules, verify we get the max payload
  // length for IP/UDP/SRTP with a MTU of 150 bytes.
  const size_t kDefaultMaxLength = 1500 - 20 - 8 - 12 - 4;
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules;
  modules.push_back(&rtp_1);
  modules.push_back(&rtp_2);
  PayloadRouter payload_router(modules, 42);

  EXPECT_EQ(kDefaultMaxLength, PayloadRouter::DefaultMaxPayloadLength());
  std::vector<VideoStream> streams(2);
  payload_router.SetSendStreams(streams);

  // Modules return a higher length than the default value.
  EXPECT_CALL(rtp_1, MaxDataPayloadLength())
      .Times(1)
      .WillOnce(Return(kDefaultMaxLength + 10));
  EXPECT_CALL(rtp_2, MaxDataPayloadLength())
      .Times(1)
      .WillOnce(Return(kDefaultMaxLength + 10));
  EXPECT_EQ(kDefaultMaxLength, payload_router.MaxPayloadLength());

  // The modules return a value lower than default.
  const size_t kTestMinPayloadLength = 1001;
  EXPECT_CALL(rtp_1, MaxDataPayloadLength())
      .Times(1)
      .WillOnce(Return(kTestMinPayloadLength + 10));
  EXPECT_CALL(rtp_2, MaxDataPayloadLength())
      .Times(1)
      .WillOnce(Return(kTestMinPayloadLength));
  EXPECT_EQ(kTestMinPayloadLength, payload_router.MaxPayloadLength());
}

TEST(PayloadRouterTest, SetTargetSendBitrates) {
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules;
  modules.push_back(&rtp_1);
  modules.push_back(&rtp_2);
  PayloadRouter payload_router(modules, 42);
  std::vector<VideoStream> streams(2);
  streams[0].max_bitrate_bps = 10000;
  streams[1].max_bitrate_bps = 100000;
  payload_router.SetSendStreams(streams);

  const uint32_t bitrate_1 = 10000;
  const uint32_t bitrate_2 = 76543;
  EXPECT_CALL(rtp_1, SetTargetSendBitrate(bitrate_1))
      .Times(1);
  EXPECT_CALL(rtp_2, SetTargetSendBitrate(bitrate_2))
      .Times(1);
  payload_router.SetTargetSendBitrate(bitrate_1 + bitrate_2);
}
}  // namespace webrtc
