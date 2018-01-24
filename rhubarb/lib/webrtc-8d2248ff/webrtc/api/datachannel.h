/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_DATACHANNEL_H_
#define WEBRTC_API_DATACHANNEL_H_

#include <deque>
#include <set>
#include <string>

#include "webrtc/api/datachannelinterface.h"
#include "webrtc/api/proxy.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/media/base/mediachannel.h"
#include "webrtc/pc/channel.h"

namespace webrtc {

class DataChannel;

class DataChannelProviderInterface {
 public:
  // Sends the data to the transport.
  virtual bool SendData(const cricket::SendDataParams& params,
                        const rtc::CopyOnWriteBuffer& payload,
                        cricket::SendDataResult* result) = 0;
  // Connects to the transport signals.
  virtual bool ConnectDataChannel(DataChannel* data_channel) = 0;
  // Disconnects from the transport signals.
  virtual void DisconnectDataChannel(DataChannel* data_channel) = 0;
  // Adds the data channel SID to the transport for SCTP.
  virtual void AddSctpDataStream(int sid) = 0;
  // Removes the data channel SID from the transport for SCTP.
  virtual void RemoveSctpDataStream(int sid) = 0;
  // Returns true if the transport channel is ready to send data.
  virtual bool ReadyToSendData() const = 0;

 protected:
  virtual ~DataChannelProviderInterface() {}
};

struct InternalDataChannelInit : public DataChannelInit {
  enum OpenHandshakeRole {
    kOpener,
    kAcker,
    kNone
  };
  // The default role is kOpener because the default |negotiated| is false.
  InternalDataChannelInit() : open_handshake_role(kOpener) {}
  explicit InternalDataChannelInit(const DataChannelInit& base)
      : DataChannelInit(base), open_handshake_role(kOpener) {
    // If the channel is externally negotiated, do not send the OPEN message.
    if (base.negotiated) {
      open_handshake_role = kNone;
    }
  }

  OpenHandshakeRole open_handshake_role;
};

// Helper class to allocate unique IDs for SCTP DataChannels
class SctpSidAllocator {
 public:
  // Gets the first unused odd/even id based on the DTLS role. If |role| is
  // SSL_CLIENT, the allocated id starts from 0 and takes even numbers;
  // otherwise, the id starts from 1 and takes odd numbers.
  // Returns false if no id can be allocated.
  bool AllocateSid(rtc::SSLRole role, int* sid);

  // Attempts to reserve a specific sid. Returns false if it's unavailable.
  bool ReserveSid(int sid);

  // Indicates that |sid| isn't in use any more, and is thus available again.
  void ReleaseSid(int sid);

 private:
  // Checks if |sid| is available to be assigned to a new SCTP data channel.
  bool IsSidAvailable(int sid) const;

  std::set<int> used_sids_;
};

// DataChannel is a an implementation of the DataChannelInterface based on
// libjingle's data engine. It provides an implementation of unreliable or
// reliabledata channels. Currently this class is specifically designed to use
// both RtpDataEngine and SctpDataEngine.

// DataChannel states:
// kConnecting: The channel has been created the transport might not yet be
//              ready.
// kOpen: The channel have a local SSRC set by a call to UpdateSendSsrc
//        and a remote SSRC set by call to UpdateReceiveSsrc and the transport
//        has been writable once.
// kClosing: DataChannelInterface::Close has been called or UpdateReceiveSsrc
//           has been called with SSRC==0
// kClosed: Both UpdateReceiveSsrc and UpdateSendSsrc has been called with
//          SSRC==0.
class DataChannel : public DataChannelInterface,
                    public sigslot::has_slots<>,
                    public rtc::MessageHandler {
 public:
  static rtc::scoped_refptr<DataChannel> Create(
      DataChannelProviderInterface* provider,
      cricket::DataChannelType dct,
      const std::string& label,
      const InternalDataChannelInit& config);

  virtual void RegisterObserver(DataChannelObserver* observer);
  virtual void UnregisterObserver();

  virtual std::string label() const { return label_; }
  virtual bool reliable() const;
  virtual bool ordered() const { return config_.ordered; }
  virtual uint16_t maxRetransmitTime() const {
    return config_.maxRetransmitTime;
  }
  virtual uint16_t maxRetransmits() const { return config_.maxRetransmits; }
  virtual std::string protocol() const { return config_.protocol; }
  virtual bool negotiated() const { return config_.negotiated; }
  virtual int id() const { return config_.id; }
  virtual uint64_t buffered_amount() const;
  virtual void Close();
  virtual DataState state() const { return state_; }
  virtual bool Send(const DataBuffer& buffer);

  // rtc::MessageHandler override.
  virtual void OnMessage(rtc::Message* msg);

  // Called when the channel's ready to use.  That can happen when the
  // underlying DataMediaChannel becomes ready, or when this channel is a new
  // stream on an existing DataMediaChannel, and we've finished negotiation.
  void OnChannelReady(bool writable);

  // Sigslots from cricket::DataChannel
  void OnDataReceived(cricket::DataChannel* channel,
                      const cricket::ReceiveDataParams& params,
                      const rtc::CopyOnWriteBuffer& payload);
  void OnStreamClosedRemotely(uint32_t sid);

  // The remote peer request that this channel should be closed.
  void RemotePeerRequestClose();

  // The following methods are for SCTP only.

  // Sets the SCTP sid and adds to transport layer if not set yet. Should only
  // be called once.
  void SetSctpSid(int sid);
  // Called when the transport channel is created.
  // Only needs to be called for SCTP data channels.
  void OnTransportChannelCreated();
  // Called when the transport channel is destroyed.
  void OnTransportChannelDestroyed();

  // The following methods are for RTP only.

  // Set the SSRC this channel should use to send data on the
  // underlying data engine. |send_ssrc| == 0 means that the channel is no
  // longer part of the session negotiation.
  void SetSendSsrc(uint32_t send_ssrc);
  // Set the SSRC this channel should use to receive data from the
  // underlying data engine.
  void SetReceiveSsrc(uint32_t receive_ssrc);

  cricket::DataChannelType data_channel_type() const {
    return data_channel_type_;
  }

  // Emitted when state transitions to kClosed.
  // In the case of SCTP channels, this signal can be used to tell when the
  // channel's sid is free.
  sigslot::signal1<DataChannel*> SignalClosed;

 protected:
  DataChannel(DataChannelProviderInterface* client,
              cricket::DataChannelType dct,
              const std::string& label);
  virtual ~DataChannel();

 private:
  // A packet queue which tracks the total queued bytes. Queued packets are
  // owned by this class.
  class PacketQueue {
   public:
    PacketQueue();
    ~PacketQueue();

    size_t byte_count() const {
      return byte_count_;
    }

    bool Empty() const;

    DataBuffer* Front();

    void Pop();

    void Push(DataBuffer* packet);

    void Clear();

    void Swap(PacketQueue* other);

   private:
    std::deque<DataBuffer*> packets_;
    size_t byte_count_;
  };

  // The OPEN(_ACK) signaling state.
  enum HandshakeState {
    kHandshakeInit,
    kHandshakeShouldSendOpen,
    kHandshakeShouldSendAck,
    kHandshakeWaitingForAck,
    kHandshakeReady
  };

  bool Init(const InternalDataChannelInit& config);
  void DoClose();
  void UpdateState();
  void SetState(DataState state);
  void DisconnectFromProvider();

  void DeliverQueuedReceivedData();

  void SendQueuedDataMessages();
  bool SendDataMessage(const DataBuffer& buffer, bool queue_if_blocked);
  bool QueueSendDataMessage(const DataBuffer& buffer);

  void SendQueuedControlMessages();
  void QueueControlMessage(const rtc::CopyOnWriteBuffer& buffer);
  bool SendControlMessage(const rtc::CopyOnWriteBuffer& buffer);

  std::string label_;
  InternalDataChannelInit config_;
  DataChannelObserver* observer_;
  DataState state_;
  cricket::DataChannelType data_channel_type_;
  DataChannelProviderInterface* provider_;
  HandshakeState handshake_state_;
  bool connected_to_provider_;
  bool send_ssrc_set_;
  bool receive_ssrc_set_;
  bool writable_;
  uint32_t send_ssrc_;
  uint32_t receive_ssrc_;
  // Control messages that always have to get sent out before any queued
  // data.
  PacketQueue queued_control_data_;
  PacketQueue queued_received_data_;
  PacketQueue queued_send_data_;
};

// Define proxy for DataChannelInterface.
BEGIN_SIGNALING_PROXY_MAP(DataChannel)
  PROXY_METHOD1(void, RegisterObserver, DataChannelObserver*)
  PROXY_METHOD0(void, UnregisterObserver)
  PROXY_CONSTMETHOD0(std::string, label)
  PROXY_CONSTMETHOD0(bool, reliable)
  PROXY_CONSTMETHOD0(bool, ordered)
  PROXY_CONSTMETHOD0(uint16_t, maxRetransmitTime)
  PROXY_CONSTMETHOD0(uint16_t, maxRetransmits)
  PROXY_CONSTMETHOD0(std::string, protocol)
  PROXY_CONSTMETHOD0(bool, negotiated)
  PROXY_CONSTMETHOD0(int, id)
  PROXY_CONSTMETHOD0(DataState, state)
  PROXY_CONSTMETHOD0(uint64_t, buffered_amount)
  PROXY_METHOD0(void, Close)
  PROXY_METHOD1(bool, Send, const DataBuffer&)
END_SIGNALING_PROXY()

}  // namespace webrtc

#endif  // WEBRTC_API_DATACHANNEL_H_
