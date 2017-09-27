/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// P2PTransportChannel wraps up the state management of the connection between
// two P2P clients.  Clients have candidate ports for connecting, and
// connections which are combinations of candidates from each end (Alice and
// Bob each have candidates, one candidate from Alice and one candidate from
// Bob are used to make a connection, repeat to make many connections).
//
// When all of the available connections become invalid (non-writable), we
// kick off a process of determining more candidates and more connections.
//
#ifndef WEBRTC_P2P_BASE_P2PTRANSPORTCHANNEL_H_
#define WEBRTC_P2P_BASE_P2PTRANSPORTCHANNEL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/p2p/base/candidate.h"
#include "webrtc/p2p/base/candidatepairinterface.h"
#include "webrtc/p2p/base/p2ptransport.h"
#include "webrtc/p2p/base/portallocator.h"
#include "webrtc/p2p/base/portinterface.h"
#include "webrtc/p2p/base/transportchannelimpl.h"
#include "webrtc/base/asyncpacketsocket.h"
#include "webrtc/base/sigslot.h"

namespace cricket {

extern const int WEAK_PING_INTERVAL;
static const int MIN_PINGS_AT_WEAK_PING_INTERVAL = 3;

struct IceParameters {
  std::string ufrag;
  std::string pwd;
  IceParameters(const std::string& ice_ufrag, const std::string& ice_pwd)
      : ufrag(ice_ufrag), pwd(ice_pwd) {}

  bool operator==(const IceParameters& other) {
    return ufrag == other.ufrag && pwd == other.pwd;
  }
  bool operator!=(const IceParameters& other) { return !(*this == other); }
};

// Adds the port on which the candidate originated.
class RemoteCandidate : public Candidate {
 public:
  RemoteCandidate(const Candidate& c, PortInterface* origin_port)
      : Candidate(c), origin_port_(origin_port) {}

  PortInterface* origin_port() { return origin_port_; }

 private:
  PortInterface* origin_port_;
};

// P2PTransportChannel manages the candidates and connection process to keep
// two P2P clients connected to each other.
class P2PTransportChannel : public TransportChannelImpl,
                            public rtc::MessageHandler {
 public:
  P2PTransportChannel(const std::string& transport_name,
                      int component,
                      PortAllocator* allocator);
  // TODO(mikescarlett): Deprecated. Remove when Chromium's
  // IceTransportChannel does not depend on this.
  P2PTransportChannel(const std::string& transport_name,
                      int component,
                      P2PTransport* transport,
                      PortAllocator* allocator);
  virtual ~P2PTransportChannel();

  // From TransportChannelImpl:
  TransportChannelState GetState() const override;
  void SetIceRole(IceRole role) override;
  IceRole GetIceRole() const override { return ice_role_; }
  void SetIceTiebreaker(uint64_t tiebreaker) override;
  void SetIceCredentials(const std::string& ice_ufrag,
                         const std::string& ice_pwd) override;
  void SetRemoteIceCredentials(const std::string& ice_ufrag,
                               const std::string& ice_pwd) override;
  void SetRemoteIceMode(IceMode mode) override;
  void Connect() override;
  void MaybeStartGathering() override;
  IceGatheringState gathering_state() const override {
    return gathering_state_;
  }
  void AddRemoteCandidate(const Candidate& candidate) override;
  void RemoveRemoteCandidate(const Candidate& candidate) override;
  // Sets the parameters in IceConfig. We do not set them blindly. Instead, we
  // only update the parameter if it is considered set in |config|. For example,
  // a negative value of receiving_timeout will be considered "not set" and we
  // will not use it to update the respective parameter in |config_|.
  void SetIceConfig(const IceConfig& config) override;
  const IceConfig& config() const;

  // From TransportChannel:
  int SendPacket(const char* data,
                 size_t len,
                 const rtc::PacketOptions& options,
                 int flags) override;
  int SetOption(rtc::Socket::Option opt, int value) override;
  bool GetOption(rtc::Socket::Option opt, int* value) override;
  int GetError() override { return error_; }
  bool GetStats(std::vector<ConnectionInfo>* stats) override;

  const Connection* best_connection() const { return best_connection_; }
  void set_incoming_only(bool value) { incoming_only_ = value; }

  // Note: This is only for testing purpose.
  // |ports_| should not be changed from outside.
  const std::vector<PortInterface*>& ports() { return ports_; }

  IceMode remote_ice_mode() const { return remote_ice_mode_; }

  // DTLS methods.
  bool IsDtlsActive() const override { return false; }

  // Default implementation.
  bool GetSslRole(rtc::SSLRole* role) const override { return false; }

  bool SetSslRole(rtc::SSLRole role) override { return false; }

  // Set up the ciphers to use for DTLS-SRTP.
  bool SetSrtpCryptoSuites(const std::vector<int>& ciphers) override {
    return false;
  }

  // Find out which DTLS-SRTP cipher was negotiated.
  bool GetSrtpCryptoSuite(int* cipher) override { return false; }

  // Find out which DTLS cipher was negotiated.
  bool GetSslCipherSuite(int* cipher) override { return false; }

  // Returns null because the channel is not encrypted by default.
  rtc::scoped_refptr<rtc::RTCCertificate> GetLocalCertificate() const override {
    return nullptr;
  }

  std::unique_ptr<rtc::SSLCertificate> GetRemoteSSLCertificate()
      const override {
    return nullptr;
  }

  // Allows key material to be extracted for external encryption.
  bool ExportKeyingMaterial(const std::string& label,
                            const uint8_t* context,
                            size_t context_len,
                            bool use_context,
                            uint8_t* result,
                            size_t result_len) override {
    return false;
  }

  bool SetLocalCertificate(
      const rtc::scoped_refptr<rtc::RTCCertificate>& certificate) override {
    return false;
  }

  // Set DTLS Remote fingerprint. Must be after local identity set.
  bool SetRemoteFingerprint(const std::string& digest_alg,
                            const uint8_t* digest,
                            size_t digest_len) override {
    return false;
  }

  int receiving_timeout() const { return config_.receiving_timeout; }
  int check_receiving_interval() const { return check_receiving_interval_; }

  // Helper method used only in unittest.
  rtc::DiffServCodePoint DefaultDscpValue() const;

  // Public for unit tests.
  Connection* FindNextPingableConnection();
  void MarkConnectionPinged(Connection* conn);

  // Public for unit tests.
  const std::vector<Connection*>& connections() const { return connections_; }

  // Public for unit tests.
  PortAllocatorSession* allocator_session() {
    return allocator_sessions_.back().get();
  }

  // Public for unit tests.
  const std::vector<RemoteCandidate>& remote_candidates() const {
    return remote_candidates_;
  }

 private:
  rtc::Thread* thread() { return worker_thread_; }
  bool IsGettingPorts() { return allocator_session()->IsGettingPorts(); }

  // A transport channel is weak if the current best connection is either
  // not receiving or not writable, or if there is no best connection at all.
  bool weak() const;
  void UpdateConnectionStates();
  void RequestSort();
  void SortConnections();
  void SwitchBestConnectionTo(Connection* conn);
  void UpdateState();
  void HandleAllTimedOut();
  void MaybeStopPortAllocatorSessions();
  TransportChannelState ComputeState() const;

  Connection* GetBestConnectionOnNetwork(rtc::Network* network) const;
  bool CreateConnections(const Candidate& remote_candidate,
                         PortInterface* origin_port);
  bool CreateConnection(PortInterface* port,
                        const Candidate& remote_candidate,
                        PortInterface* origin_port);
  bool FindConnection(cricket::Connection* connection) const;

  uint32_t GetRemoteCandidateGeneration(const Candidate& candidate);
  bool IsDuplicateRemoteCandidate(const Candidate& candidate);
  void RememberRemoteCandidate(const Candidate& remote_candidate,
                               PortInterface* origin_port);
  bool IsPingable(Connection* conn, int64_t now);
  void PingConnection(Connection* conn);
  void AddAllocatorSession(std::unique_ptr<PortAllocatorSession> session);
  void AddConnection(Connection* connection);

  void OnPortReady(PortAllocatorSession *session, PortInterface* port);
  void OnCandidatesReady(PortAllocatorSession *session,
                         const std::vector<Candidate>& candidates);
  void OnCandidatesAllocationDone(PortAllocatorSession* session);
  void OnUnknownAddress(PortInterface* port,
                        const rtc::SocketAddress& addr,
                        ProtocolType proto,
                        IceMessage* stun_msg,
                        const std::string& remote_username,
                        bool port_muxed);
  void OnPortDestroyed(PortInterface* port);
  void OnPortNetworkInactive(PortInterface* port);
  void OnRoleConflict(PortInterface* port);

  void OnConnectionStateChange(Connection* connection);
  void OnReadPacket(Connection *connection, const char *data, size_t len,
                    const rtc::PacketTime& packet_time);
  void OnSentPacket(const rtc::SentPacket& sent_packet);
  void OnReadyToSend(Connection* connection);
  void OnConnectionDestroyed(Connection *connection);

  void OnNominated(Connection* conn);

  void OnMessage(rtc::Message* pmsg) override;
  void OnSort();
  void OnCheckAndPing();

  void PruneConnections();
  Connection* best_nominated_connection() const;
  bool IsBackupConnection(Connection* conn) const;

  Connection* FindConnectionToPing(int64_t now);
  Connection* FindOldestConnectionNeedingTriggeredCheck(int64_t now);
  // Between |conn1| and |conn2|, this function returns the one which should
  // be pinged first.
  Connection* SelectMostPingableConnection(Connection* conn1,
                                           Connection* conn2);
  // Select the connection which is Relay/Relay. If both of them are,
  // UDP relay protocol takes precedence.
  Connection* MostLikelyToWork(Connection* conn1, Connection* conn2);
  // Compare the last_ping_sent time and return the one least recently pinged.
  Connection* LeastRecentlyPinged(Connection* conn1, Connection* conn2);

  // Returns the latest remote ICE parameters or nullptr if there are no remote
  // ICE parameters yet.
  IceParameters* remote_ice() {
    return remote_ice_parameters_.empty() ? nullptr
                                          : &remote_ice_parameters_.back();
  }
  // Returns the remote IceParameters and generation that match |ufrag|
  // if found, and returns nullptr otherwise.
  const IceParameters* FindRemoteIceFromUfrag(const std::string& ufrag,
                                              uint32_t* generation);
  // Returns the index of the latest remote ICE parameters, or 0 if no remote
  // ICE parameters have been received.
  uint32_t remote_ice_generation() {
    return remote_ice_parameters_.empty()
               ? 0
               : static_cast<uint32_t>(remote_ice_parameters_.size() - 1);
  }

  PortAllocator* allocator_;
  rtc::Thread* worker_thread_;
  bool incoming_only_;
  int error_;
  std::vector<std::unique_ptr<PortAllocatorSession>> allocator_sessions_;
  std::vector<PortInterface *> ports_;

  // |connections_| is a sorted list with the first one always be the
  // |best_connection_| when it's not nullptr. The combination of
  // |pinged_connections_| and |unpinged_connections_| has the same
  // connections as |connections_|. These 2 sets maintain whether a
  // connection should be pinged next or not.
  std::vector<Connection *> connections_;
  std::set<Connection*> pinged_connections_;
  std::set<Connection*> unpinged_connections_;

  Connection* best_connection_;

  // Connection selected by the controlling agent. This should be used only
  // at controlled side when protocol type is RFC5245.
  Connection* pending_best_connection_;
  std::vector<RemoteCandidate> remote_candidates_;
  bool sort_dirty_;  // indicates whether another sort is needed right now
  bool had_connection_ = false;  // if connections_ has ever been nonempty
  typedef std::map<rtc::Socket::Option, int> OptionMap;
  OptionMap options_;
  std::string ice_ufrag_;
  std::string ice_pwd_;
  std::vector<IceParameters> remote_ice_parameters_;
  IceMode remote_ice_mode_;
  IceRole ice_role_;
  uint64_t tiebreaker_;
  IceGatheringState gathering_state_;

  int check_receiving_interval_;
  int64_t last_ping_sent_ms_ = 0;
  int weak_ping_interval_ = WEAK_PING_INTERVAL;
  TransportChannelState state_ = TransportChannelState::STATE_INIT;
  IceConfig config_;
  int last_sent_packet_id_ = -1;  // -1 indicates no packet was sent before.

  RTC_DISALLOW_COPY_AND_ASSIGN(P2PTransportChannel);
};

}  // namespace cricket

#endif  // WEBRTC_P2P_BASE_P2PTRANSPORTCHANNEL_H_
