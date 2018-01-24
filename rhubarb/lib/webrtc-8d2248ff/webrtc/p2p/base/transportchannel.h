/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_P2P_BASE_TRANSPORTCHANNEL_H_
#define WEBRTC_P2P_BASE_TRANSPORTCHANNEL_H_

#include <memory>
#include <string>
#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/p2p/base/candidate.h"
#include "webrtc/p2p/base/candidatepairinterface.h"
#include "webrtc/p2p/base/transport.h"
#include "webrtc/p2p/base/transportdescription.h"
#include "webrtc/base/asyncpacketsocket.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/base/dscp.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socket.h"
#include "webrtc/base/sslidentity.h"
#include "webrtc/base/sslstreamadapter.h"

namespace cricket {

class Candidate;

// Flags for SendPacket/SignalReadPacket.
enum PacketFlags {
  PF_NORMAL       = 0x00,  // A normal packet.
  PF_SRTP_BYPASS  = 0x01,  // An encrypted SRTP packet; bypass any additional
                           // crypto provided by the transport (e.g. DTLS)
};

// Used to indicate channel's connection state.
enum TransportChannelState {
  STATE_INIT,
  STATE_CONNECTING,  // Will enter this state once a connection is created
  STATE_COMPLETED,
  STATE_FAILED
};

// A TransportChannel represents one logical stream of packets that are sent
// between the two sides of a session.
// TODO(deadbeef): This interface currently represents the unity of an ICE
// transport and a DTLS transport. They need to be separated apart.
class TransportChannel : public sigslot::has_slots<> {
 public:
  TransportChannel(const std::string& transport_name, int component)
      : transport_name_(transport_name),
        component_(component),
        writable_(false),
        receiving_(false) {}
  virtual ~TransportChannel() {}

  // TODO(guoweis) - Make this pure virtual once all subclasses of
  // TransportChannel have this defined.
  virtual TransportChannelState GetState() const {
    return TransportChannelState::STATE_CONNECTING;
  }

  // TODO(mallinath) - Remove this API, as it's no longer useful.
  // Returns the session id of this channel.
  virtual const std::string SessionId() const { return std::string(); }

  const std::string& transport_name() const { return transport_name_; }
  int component() const { return component_; }

  // Returns the states of this channel.  Each time one of these states changes,
  // a signal is raised.  These states are aggregated by the TransportManager.
  bool writable() const { return writable_; }
  bool receiving() const { return receiving_; }
  DtlsTransportState dtls_state() const { return dtls_state_; }
  sigslot::signal1<TransportChannel*> SignalWritableState;
  // Emitted when the TransportChannel's ability to send has changed.
  sigslot::signal1<TransportChannel*> SignalReadyToSend;
  sigslot::signal1<TransportChannel*> SignalReceivingState;
  // Emitted whenever DTLS-SRTP is setup which will require setting up a new
  // SRTP context.
  sigslot::signal2<TransportChannel*, DtlsTransportState> SignalDtlsState;

  // Attempts to send the given packet.  The return value is < 0 on failure.
  // TODO: Remove the default argument once channel code is updated.
  virtual int SendPacket(const char* data, size_t len,
                         const rtc::PacketOptions& options,
                         int flags = 0) = 0;

  // Sets a socket option on this channel.  Note that not all options are
  // supported by all transport types.
  virtual int SetOption(rtc::Socket::Option opt, int value) = 0;
  // TODO(pthatcher): Once Chrome's MockTransportChannel implments
  // this, remove the default implementation.
  virtual bool GetOption(rtc::Socket::Option opt, int* value) { return false; }

  // Returns the most recent error that occurred on this channel.
  virtual int GetError() = 0;

  // Returns the current stats for this connection.
  virtual bool GetStats(ConnectionInfos* infos) = 0;

  // Is DTLS active?
  virtual bool IsDtlsActive() const = 0;

  // Default implementation.
  virtual bool GetSslRole(rtc::SSLRole* role) const = 0;

  // Sets up the ciphers to use for DTLS-SRTP. TODO(guoweis): Make this pure
  // virtual once all dependencies have implementation.
  virtual bool SetSrtpCryptoSuites(const std::vector<int>& ciphers);

  // Keep the original one for backward compatibility until all dependencies
  // move away. TODO(guoweis): Remove this function.
  virtual bool SetSrtpCiphers(const std::vector<std::string>& ciphers);

  // Finds out which DTLS-SRTP cipher was negotiated.
  // TODO(guoweis): Remove this once all dependencies implement this.
  virtual bool GetSrtpCryptoSuite(int* cipher) { return false; }

  // Finds out which DTLS cipher was negotiated.
  // TODO(guoweis): Remove this once all dependencies implement this.
  virtual bool GetSslCipherSuite(int* cipher) { return false; }

  // Gets the local RTCCertificate used for DTLS.
  virtual rtc::scoped_refptr<rtc::RTCCertificate>
  GetLocalCertificate() const = 0;

  // Gets a copy of the remote side's SSL certificate.
  virtual std::unique_ptr<rtc::SSLCertificate> GetRemoteSSLCertificate()
      const = 0;

  // Allows key material to be extracted for external encryption.
  virtual bool ExportKeyingMaterial(const std::string& label,
                                    const uint8_t* context,
                                    size_t context_len,
                                    bool use_context,
                                    uint8_t* result,
                                    size_t result_len) = 0;

  // Signalled each time a packet is received on this channel.
  sigslot::signal5<TransportChannel*, const char*,
                   size_t, const rtc::PacketTime&, int> SignalReadPacket;

  // Signalled each time a packet is sent on this channel.
  sigslot::signal2<TransportChannel*, const rtc::SentPacket&> SignalSentPacket;

  // Deprecated by SignalSelectedCandidatePairChanged
  // This signal occurs when there is a change in the way that packets are
  // being routed, i.e. to a different remote location. The candidate
  // indicates where and how we are currently sending media.
  sigslot::signal2<TransportChannel*, const Candidate&> SignalRouteChange;

  // Signalled when the current selected candidate pair has changed.
  // The first parameter is the transport channel that signals the event.
  // The second parameter is the new selected candidate pair. The third
  // parameter is the last packet id sent on the previous candidate pair.
  sigslot::signal3<TransportChannel*, CandidatePairInterface*, int>
      SignalSelectedCandidatePairChanged;

  // Invoked when the channel is being destroyed.
  sigslot::signal1<TransportChannel*> SignalDestroyed;

  // Debugging description of this transport channel.
  std::string ToString() const;

 protected:
  // Sets the writable state, signaling if necessary.
  void set_writable(bool writable);

  // Sets the receiving state, signaling if necessary.
  void set_receiving(bool receiving);

  // Sets the DTLS state, signaling if necessary.
  void set_dtls_state(DtlsTransportState state);

 private:
  // Used mostly for debugging.
  std::string transport_name_;
  int component_;
  bool writable_;
  bool receiving_;
  DtlsTransportState dtls_state_ = DTLS_TRANSPORT_NEW;

  RTC_DISALLOW_COPY_AND_ASSIGN(TransportChannel);
};

}  // namespace cricket

#endif  // WEBRTC_P2P_BASE_TRANSPORTCHANNEL_H_
