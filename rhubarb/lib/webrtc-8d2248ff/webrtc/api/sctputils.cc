/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/sctputils.h"

#include "webrtc/base/bytebuffer.h"
#include "webrtc/base/copyonwritebuffer.h"
#include "webrtc/base/logging.h"

namespace webrtc {

// Format defined at
// http://tools.ietf.org/html/draft-ietf-rtcweb-data-protocol-01#section

static const uint8_t DATA_CHANNEL_OPEN_MESSAGE_TYPE = 0x03;
static const uint8_t DATA_CHANNEL_OPEN_ACK_MESSAGE_TYPE = 0x02;

enum DataChannelOpenMessageChannelType {
  DCOMCT_ORDERED_RELIABLE = 0x00,
  DCOMCT_ORDERED_PARTIAL_RTXS = 0x01,
  DCOMCT_ORDERED_PARTIAL_TIME = 0x02,
  DCOMCT_UNORDERED_RELIABLE = 0x80,
  DCOMCT_UNORDERED_PARTIAL_RTXS = 0x81,
  DCOMCT_UNORDERED_PARTIAL_TIME = 0x82,
};

bool IsOpenMessage(const rtc::CopyOnWriteBuffer& payload) {
  // Format defined at
  // http://tools.ietf.org/html/draft-jesup-rtcweb-data-protocol-04
  if (payload.size() < 1) {
    LOG(LS_WARNING) << "Could not read OPEN message type.";
    return false;
  }

  uint8_t message_type = payload[0];
  return message_type == DATA_CHANNEL_OPEN_MESSAGE_TYPE;
}

bool ParseDataChannelOpenMessage(const rtc::CopyOnWriteBuffer& payload,
                                 std::string* label,
                                 DataChannelInit* config) {
  // Format defined at
  // http://tools.ietf.org/html/draft-jesup-rtcweb-data-protocol-04

  rtc::ByteBufferReader buffer(payload.data<char>(), payload.size());
  uint8_t message_type;
  if (!buffer.ReadUInt8(&message_type)) {
    LOG(LS_WARNING) << "Could not read OPEN message type.";
    return false;
  }
  if (message_type != DATA_CHANNEL_OPEN_MESSAGE_TYPE) {
    LOG(LS_WARNING) << "Data Channel OPEN message of unexpected type: "
                    << message_type;
    return false;
  }

  uint8_t channel_type;
  if (!buffer.ReadUInt8(&channel_type)) {
    LOG(LS_WARNING) << "Could not read OPEN message channel type.";
    return false;
  }

  uint16_t priority;
  if (!buffer.ReadUInt16(&priority)) {
    LOG(LS_WARNING) << "Could not read OPEN message reliabilility prioirty.";
    return false;
  }
  uint32_t reliability_param;
  if (!buffer.ReadUInt32(&reliability_param)) {
    LOG(LS_WARNING) << "Could not read OPEN message reliabilility param.";
    return false;
  }
  uint16_t label_length;
  if (!buffer.ReadUInt16(&label_length)) {
    LOG(LS_WARNING) << "Could not read OPEN message label length.";
    return false;
  }
  uint16_t protocol_length;
  if (!buffer.ReadUInt16(&protocol_length)) {
    LOG(LS_WARNING) << "Could not read OPEN message protocol length.";
    return false;
  }
  if (!buffer.ReadString(label, (size_t) label_length)) {
    LOG(LS_WARNING) << "Could not read OPEN message label";
    return false;
  }
  if (!buffer.ReadString(&config->protocol, protocol_length)) {
    LOG(LS_WARNING) << "Could not read OPEN message protocol.";
    return false;
  }

  config->ordered = true;
  switch (channel_type) {
    case DCOMCT_UNORDERED_RELIABLE:
    case DCOMCT_UNORDERED_PARTIAL_RTXS:
    case DCOMCT_UNORDERED_PARTIAL_TIME:
      config->ordered = false;
  }

  config->maxRetransmits = -1;
  config->maxRetransmitTime = -1;
  switch (channel_type) {
    case DCOMCT_ORDERED_PARTIAL_RTXS:
    case DCOMCT_UNORDERED_PARTIAL_RTXS:
      config->maxRetransmits = reliability_param;
      break;
    case DCOMCT_ORDERED_PARTIAL_TIME:
    case DCOMCT_UNORDERED_PARTIAL_TIME:
      config->maxRetransmitTime = reliability_param;
      break;
  }
  return true;
}

bool ParseDataChannelOpenAckMessage(const rtc::CopyOnWriteBuffer& payload) {
  if (payload.size() < 1) {
    LOG(LS_WARNING) << "Could not read OPEN_ACK message type.";
    return false;
  }

  uint8_t message_type = payload[0];
  if (message_type != DATA_CHANNEL_OPEN_ACK_MESSAGE_TYPE) {
    LOG(LS_WARNING) << "Data Channel OPEN_ACK message of unexpected type: "
                    << message_type;
    return false;
  }
  return true;
}

bool WriteDataChannelOpenMessage(const std::string& label,
                                 const DataChannelInit& config,
                                 rtc::CopyOnWriteBuffer* payload) {
  // Format defined at
  // http://tools.ietf.org/html/draft-ietf-rtcweb-data-protocol-00#section-6.1
  uint8_t channel_type = 0;
  uint32_t reliability_param = 0;
  uint16_t priority = 0;
  if (config.ordered) {
    if (config.maxRetransmits > -1) {
      channel_type = DCOMCT_ORDERED_PARTIAL_RTXS;
      reliability_param = config.maxRetransmits;
    } else if (config.maxRetransmitTime > -1) {
      channel_type = DCOMCT_ORDERED_PARTIAL_TIME;
      reliability_param = config.maxRetransmitTime;
    } else {
      channel_type = DCOMCT_ORDERED_RELIABLE;
    }
  } else {
    if (config.maxRetransmits > -1) {
      channel_type = DCOMCT_UNORDERED_PARTIAL_RTXS;
      reliability_param = config.maxRetransmits;
    } else if (config.maxRetransmitTime > -1) {
      channel_type = DCOMCT_UNORDERED_PARTIAL_TIME;
      reliability_param = config.maxRetransmitTime;
    } else {
      channel_type = DCOMCT_UNORDERED_RELIABLE;
    }
  }

  rtc::ByteBufferWriter buffer(
      NULL, 20 + label.length() + config.protocol.length(),
      rtc::ByteBuffer::ORDER_NETWORK);
  // TODO(tommi): Add error handling and check resulting length.
  buffer.WriteUInt8(DATA_CHANNEL_OPEN_MESSAGE_TYPE);
  buffer.WriteUInt8(channel_type);
  buffer.WriteUInt16(priority);
  buffer.WriteUInt32(reliability_param);
  buffer.WriteUInt16(static_cast<uint16_t>(label.length()));
  buffer.WriteUInt16(static_cast<uint16_t>(config.protocol.length()));
  buffer.WriteString(label);
  buffer.WriteString(config.protocol);
  payload->SetData(buffer.Data(), buffer.Length());
  return true;
}

void WriteDataChannelOpenAckMessage(rtc::CopyOnWriteBuffer* payload) {
  uint8_t data = DATA_CHANNEL_OPEN_ACK_MESSAGE_TYPE;
  payload->SetData(&data, sizeof(data));
}

}  // namespace webrtc
