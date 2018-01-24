/*
 *  Copyright 2009 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <algorithm>
#include <memory>

#include "webrtc/p2p/base/fakeportallocator.h"
#include "webrtc/p2p/base/p2ptransportchannel.h"
#include "webrtc/p2p/base/testrelayserver.h"
#include "webrtc/p2p/base/teststunserver.h"
#include "webrtc/p2p/base/testturnserver.h"
#include "webrtc/p2p/client/basicportallocator.h"
#include "webrtc/base/dscp.h"
#include "webrtc/base/fakenetwork.h"
#include "webrtc/base/firewallsocketserver.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/natserver.h"
#include "webrtc/base/natsocketfactory.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/proxyserver.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/virtualsocketserver.h"

using cricket::kDefaultPortAllocatorFlags;
using cricket::kMinimumStepDelay;
using cricket::kDefaultStepDelay;
using cricket::PORTALLOCATOR_ENABLE_SHARED_SOCKET;
using cricket::ServerAddresses;
using cricket::MIN_PINGS_AT_WEAK_PING_INTERVAL;
using rtc::SocketAddress;

static const int kDefaultTimeout = 1000;
static const int kOnlyLocalPorts = cricket::PORTALLOCATOR_DISABLE_STUN |
                                   cricket::PORTALLOCATOR_DISABLE_RELAY |
                                   cricket::PORTALLOCATOR_DISABLE_TCP;
// Addresses on the public internet.
static const SocketAddress kPublicAddrs[2] =
    { SocketAddress("11.11.11.11", 0), SocketAddress("22.22.22.22", 0) };
// IPv6 Addresses on the public internet.
static const SocketAddress kIPv6PublicAddrs[2] = {
    SocketAddress("2400:4030:1:2c00:be30:abcd:efab:cdef", 0),
    SocketAddress("2620:0:1000:1b03:2e41:38ff:fea6:f2a4", 0)
};
// For configuring multihomed clients.
static const SocketAddress kAlternateAddrs[2] =
    { SocketAddress("11.11.11.101", 0), SocketAddress("22.22.22.202", 0) };
// Addresses for HTTP proxy servers.
static const SocketAddress kHttpsProxyAddrs[2] =
    { SocketAddress("11.11.11.1", 443), SocketAddress("22.22.22.1", 443) };
// Addresses for SOCKS proxy servers.
static const SocketAddress kSocksProxyAddrs[2] =
    { SocketAddress("11.11.11.1", 1080), SocketAddress("22.22.22.1", 1080) };
// Internal addresses for NAT boxes.
static const SocketAddress kNatAddrs[2] =
    { SocketAddress("192.168.1.1", 0), SocketAddress("192.168.2.1", 0) };
// Private addresses inside the NAT private networks.
static const SocketAddress kPrivateAddrs[2] =
    { SocketAddress("192.168.1.11", 0), SocketAddress("192.168.2.22", 0) };
// For cascaded NATs, the internal addresses of the inner NAT boxes.
static const SocketAddress kCascadedNatAddrs[2] =
    { SocketAddress("192.168.10.1", 0), SocketAddress("192.168.20.1", 0) };
// For cascaded NATs, private addresses inside the inner private networks.
static const SocketAddress kCascadedPrivateAddrs[2] =
    { SocketAddress("192.168.10.11", 0), SocketAddress("192.168.20.22", 0) };
// The address of the public STUN server.
static const SocketAddress kStunAddr("99.99.99.1", cricket::STUN_SERVER_PORT);
// The addresses for the public relay server.
static const SocketAddress kRelayUdpIntAddr("99.99.99.2", 5000);
static const SocketAddress kRelayUdpExtAddr("99.99.99.3", 5001);
static const SocketAddress kRelayTcpIntAddr("99.99.99.2", 5002);
static const SocketAddress kRelayTcpExtAddr("99.99.99.3", 5003);
static const SocketAddress kRelaySslTcpIntAddr("99.99.99.2", 5004);
static const SocketAddress kRelaySslTcpExtAddr("99.99.99.3", 5005);
// The addresses for the public turn server.
static const SocketAddress kTurnUdpIntAddr("99.99.99.4",
                                           cricket::STUN_SERVER_PORT);
static const SocketAddress kTurnTcpIntAddr("99.99.99.4",
                                           cricket::STUN_SERVER_PORT + 1);
static const SocketAddress kTurnUdpExtAddr("99.99.99.5", 0);
static const cricket::RelayCredentials kRelayCredentials("test", "test");

// Based on ICE_UFRAG_LENGTH
static const char* kIceUfrag[4] = {"UF00", "UF01",
                                   "UF02", "UF03"};
// Based on ICE_PWD_LENGTH
static const char* kIcePwd[4] = {"TESTICEPWD00000000000000",
                                 "TESTICEPWD00000000000001",
                                 "TESTICEPWD00000000000002",
                                 "TESTICEPWD00000000000003"};

static const uint64_t kTiebreaker1 = 11111;
static const uint64_t kTiebreaker2 = 22222;

enum { MSG_ADD_CANDIDATES, MSG_REMOVE_CANDIDATES };

static cricket::IceConfig CreateIceConfig(int receiving_timeout,
                                          bool gather_continually,
                                          int backup_ping_interval = -1) {
  cricket::IceConfig config;
  config.receiving_timeout = receiving_timeout;
  config.gather_continually = gather_continually;
  config.backup_connection_ping_interval = backup_ping_interval;
  return config;
}

// This test simulates 2 P2P endpoints that want to establish connectivity
// with each other over various network topologies and conditions, which can be
// specified in each individial test.
// A virtual network (via VirtualSocketServer) along with virtual firewalls and
// NATs (via Firewall/NATSocketServer) are used to simulate the various network
// conditions. We can configure the IP addresses of the endpoints,
// block various types of connectivity, or add arbitrary levels of NAT.
// We also run a STUN server and a relay server on the virtual network to allow
// our typical P2P mechanisms to do their thing.
// For each case, we expect the P2P stack to eventually settle on a specific
// form of connectivity to the other side. The test checks that the P2P
// negotiation successfully establishes connectivity within a certain time,
// and that the result is what we expect.
// Note that this class is a base class for use by other tests, who will provide
// specialized test behavior.
class P2PTransportChannelTestBase : public testing::Test,
                                    public rtc::MessageHandler,
                                    public sigslot::has_slots<> {
 public:
  P2PTransportChannelTestBase()
      : main_(rtc::Thread::Current()),
        pss_(new rtc::PhysicalSocketServer),
        vss_(new rtc::VirtualSocketServer(pss_.get())),
        nss_(new rtc::NATSocketServer(vss_.get())),
        ss_(new rtc::FirewallSocketServer(nss_.get())),
        ss_scope_(ss_.get()),
        stun_server_(cricket::TestStunServer::Create(main_, kStunAddr)),
        turn_server_(main_, kTurnUdpIntAddr, kTurnUdpExtAddr),
        relay_server_(main_, kRelayUdpIntAddr, kRelayUdpExtAddr,
                      kRelayTcpIntAddr, kRelayTcpExtAddr,
                      kRelaySslTcpIntAddr, kRelaySslTcpExtAddr),
        socks_server1_(ss_.get(), kSocksProxyAddrs[0],
                       ss_.get(), kSocksProxyAddrs[0]),
        socks_server2_(ss_.get(), kSocksProxyAddrs[1],
                       ss_.get(), kSocksProxyAddrs[1]),
        clear_remote_candidates_ufrag_pwd_(false),
        force_relay_(false) {
    ep1_.role_ = cricket::ICEROLE_CONTROLLING;
    ep2_.role_ = cricket::ICEROLE_CONTROLLED;

    ServerAddresses stun_servers;
    stun_servers.insert(kStunAddr);
    ep1_.allocator_.reset(new cricket::BasicPortAllocator(
        &ep1_.network_manager_,
        stun_servers, kRelayUdpIntAddr, kRelayTcpIntAddr, kRelaySslTcpIntAddr));
    ep2_.allocator_.reset(new cricket::BasicPortAllocator(
        &ep2_.network_manager_,
        stun_servers, kRelayUdpIntAddr, kRelayTcpIntAddr, kRelaySslTcpIntAddr));
  }

 protected:
  enum Config {
    OPEN,                           // Open to the Internet
    NAT_FULL_CONE,                  // NAT, no filtering
    NAT_ADDR_RESTRICTED,            // NAT, must send to an addr to recv
    NAT_PORT_RESTRICTED,            // NAT, must send to an addr+port to recv
    NAT_SYMMETRIC,                  // NAT, endpoint-dependent bindings
    NAT_DOUBLE_CONE,                // Double NAT, both cone
    NAT_SYMMETRIC_THEN_CONE,        // Double NAT, symmetric outer, cone inner
    BLOCK_UDP,                      // Firewall, UDP in/out blocked
    BLOCK_UDP_AND_INCOMING_TCP,     // Firewall, UDP in/out and TCP in blocked
    BLOCK_ALL_BUT_OUTGOING_HTTP,    // Firewall, only TCP out on 80/443
    PROXY_HTTPS,                    // All traffic through HTTPS proxy
    PROXY_SOCKS,                    // All traffic through SOCKS proxy
    NUM_CONFIGS
  };

  struct Result {
    Result(const std::string& lt, const std::string& lp,
           const std::string& rt, const std::string& rp,
           const std::string& lt2, const std::string& lp2,
           const std::string& rt2, const std::string& rp2, int wait)
        : local_type(lt), local_proto(lp), remote_type(rt), remote_proto(rp),
          local_type2(lt2), local_proto2(lp2), remote_type2(rt2),
          remote_proto2(rp2), connect_wait(wait) {
    }

    std::string local_type;
    std::string local_proto;
    std::string remote_type;
    std::string remote_proto;
    std::string local_type2;
    std::string local_proto2;
    std::string remote_type2;
    std::string remote_proto2;
    int connect_wait;
  };

  struct ChannelData {
    bool CheckData(const char* data, int len) {
      bool ret = false;
      if (!ch_packets_.empty()) {
        std::string packet =  ch_packets_.front();
        ret = (packet == std::string(data, len));
        ch_packets_.pop_front();
      }
      return ret;
    }

    std::string name_;  // TODO - Currently not used.
    std::list<std::string> ch_packets_;
    std::unique_ptr<cricket::P2PTransportChannel> ch_;
  };

  struct CandidatesData : public rtc::MessageData {
    CandidatesData(cricket::TransportChannel* ch, const cricket::Candidate& c)
        : channel(ch), candidates(1, c) {}
    CandidatesData(cricket::TransportChannel* ch,
                   const std::vector<cricket::Candidate>& cc)
        : channel(ch), candidates(cc) {}
    cricket::TransportChannel* channel;
    cricket::Candidates candidates;
  };

  struct Endpoint {
    Endpoint()
        : role_(cricket::ICEROLE_UNKNOWN),
          tiebreaker_(0),
          role_conflict_(false),
          save_candidates_(false) {}
    bool HasChannel(cricket::TransportChannel* ch) {
      return (ch == cd1_.ch_.get() || ch == cd2_.ch_.get());
    }
    ChannelData* GetChannelData(cricket::TransportChannel* ch) {
      if (!HasChannel(ch)) return NULL;
      if (cd1_.ch_.get() == ch)
        return &cd1_;
      else
        return &cd2_;
    }

    void SetIceRole(cricket::IceRole role) { role_ = role; }
    cricket::IceRole ice_role() { return role_; }
    void SetIceTiebreaker(uint64_t tiebreaker) { tiebreaker_ = tiebreaker; }
    uint64_t GetIceTiebreaker() { return tiebreaker_; }
    void OnRoleConflict(bool role_conflict) { role_conflict_ = role_conflict; }
    bool role_conflict() { return role_conflict_; }
    void SetAllocationStepDelay(uint32_t delay) {
      allocator_->set_step_delay(delay);
    }
    void SetAllowTcpListen(bool allow_tcp_listen) {
      allocator_->set_allow_tcp_listen(allow_tcp_listen);
    }

    rtc::FakeNetworkManager network_manager_;
    std::unique_ptr<cricket::BasicPortAllocator> allocator_;
    ChannelData cd1_;
    ChannelData cd2_;
    cricket::IceRole role_;
    uint64_t tiebreaker_;
    bool role_conflict_;
    bool save_candidates_;
    std::vector<CandidatesData*> saved_candidates_;
  };

  ChannelData* GetChannelData(cricket::TransportChannel* channel) {
    if (ep1_.HasChannel(channel))
      return ep1_.GetChannelData(channel);
    else
      return ep2_.GetChannelData(channel);
  }

  void CreateChannels(int num) {
    std::string ice_ufrag_ep1_cd1_ch = kIceUfrag[0];
    std::string ice_pwd_ep1_cd1_ch = kIcePwd[0];
    std::string ice_ufrag_ep2_cd1_ch = kIceUfrag[1];
    std::string ice_pwd_ep2_cd1_ch = kIcePwd[1];
    ep1_.cd1_.ch_.reset(CreateChannel(
        0, cricket::ICE_CANDIDATE_COMPONENT_DEFAULT,
        ice_ufrag_ep1_cd1_ch, ice_pwd_ep1_cd1_ch,
        ice_ufrag_ep2_cd1_ch, ice_pwd_ep2_cd1_ch));
    ep2_.cd1_.ch_.reset(CreateChannel(
        1, cricket::ICE_CANDIDATE_COMPONENT_DEFAULT,
        ice_ufrag_ep2_cd1_ch, ice_pwd_ep2_cd1_ch,
        ice_ufrag_ep1_cd1_ch, ice_pwd_ep1_cd1_ch));
    ep1_.cd1_.ch_->MaybeStartGathering();
    ep2_.cd1_.ch_->MaybeStartGathering();
    if (num == 2) {
      std::string ice_ufrag_ep1_cd2_ch = kIceUfrag[2];
      std::string ice_pwd_ep1_cd2_ch = kIcePwd[2];
      std::string ice_ufrag_ep2_cd2_ch = kIceUfrag[3];
      std::string ice_pwd_ep2_cd2_ch = kIcePwd[3];
      ep1_.cd2_.ch_.reset(CreateChannel(
          0, cricket::ICE_CANDIDATE_COMPONENT_DEFAULT,
          ice_ufrag_ep1_cd2_ch, ice_pwd_ep1_cd2_ch,
          ice_ufrag_ep2_cd2_ch, ice_pwd_ep2_cd2_ch));
      ep2_.cd2_.ch_.reset(CreateChannel(
          1, cricket::ICE_CANDIDATE_COMPONENT_DEFAULT,
          ice_ufrag_ep2_cd2_ch, ice_pwd_ep2_cd2_ch,
          ice_ufrag_ep1_cd2_ch, ice_pwd_ep1_cd2_ch));
      ep1_.cd2_.ch_->MaybeStartGathering();
      ep2_.cd2_.ch_->MaybeStartGathering();
    }
  }
  cricket::P2PTransportChannel* CreateChannel(
      int endpoint,
      int component,
      const std::string& local_ice_ufrag,
      const std::string& local_ice_pwd,
      const std::string& remote_ice_ufrag,
      const std::string& remote_ice_pwd) {
    cricket::P2PTransportChannel* channel = new cricket::P2PTransportChannel(
        "test content name", component, GetAllocator(endpoint));
    channel->SignalCandidateGathered.connect(
        this, &P2PTransportChannelTestBase::OnCandidateGathered);
    channel->SignalCandidatesRemoved.connect(
        this, &P2PTransportChannelTestBase::OnCandidatesRemoved);
    channel->SignalReadPacket.connect(
        this, &P2PTransportChannelTestBase::OnReadPacket);
    channel->SignalRoleConflict.connect(
        this, &P2PTransportChannelTestBase::OnRoleConflict);
    channel->SetIceCredentials(local_ice_ufrag, local_ice_pwd);
    if (clear_remote_candidates_ufrag_pwd_) {
      // This only needs to be set if we're clearing them from the
      // candidates.  Some unit tests rely on this not being set.
      channel->SetRemoteIceCredentials(remote_ice_ufrag, remote_ice_pwd);
    }
    channel->SetIceRole(GetEndpoint(endpoint)->ice_role());
    channel->SetIceTiebreaker(GetEndpoint(endpoint)->GetIceTiebreaker());
    channel->Connect();
    return channel;
  }
  void DestroyChannels() {
    ep1_.cd1_.ch_.reset();
    ep2_.cd1_.ch_.reset();
    ep1_.cd2_.ch_.reset();
    ep2_.cd2_.ch_.reset();
  }
  cricket::P2PTransportChannel* ep1_ch1() { return ep1_.cd1_.ch_.get(); }
  cricket::P2PTransportChannel* ep1_ch2() { return ep1_.cd2_.ch_.get(); }
  cricket::P2PTransportChannel* ep2_ch1() { return ep2_.cd1_.ch_.get(); }
  cricket::P2PTransportChannel* ep2_ch2() { return ep2_.cd2_.ch_.get(); }

  // Common results.
  static const Result kLocalUdpToLocalUdp;
  static const Result kLocalUdpToStunUdp;
  static const Result kLocalUdpToPrflxUdp;
  static const Result kPrflxUdpToLocalUdp;
  static const Result kStunUdpToLocalUdp;
  static const Result kStunUdpToStunUdp;
  static const Result kPrflxUdpToStunUdp;
  static const Result kLocalUdpToRelayUdp;
  static const Result kPrflxUdpToRelayUdp;
  static const Result kLocalTcpToLocalTcp;
  static const Result kLocalTcpToPrflxTcp;
  static const Result kPrflxTcpToLocalTcp;

  rtc::NATSocketServer* nat() { return nss_.get(); }
  rtc::FirewallSocketServer* fw() { return ss_.get(); }

  Endpoint* GetEndpoint(int endpoint) {
    if (endpoint == 0) {
      return &ep1_;
    } else if (endpoint == 1) {
      return &ep2_;
    } else {
      return NULL;
    }
  }
  cricket::PortAllocator* GetAllocator(int endpoint) {
    return GetEndpoint(endpoint)->allocator_.get();
  }
  void AddAddress(int endpoint, const SocketAddress& addr) {
    GetEndpoint(endpoint)->network_manager_.AddInterface(addr);
  }
  void AddAddress(int endpoint,
                  const SocketAddress& addr,
                  const std::string& ifname,
                  rtc::AdapterType adapter_type) {
    GetEndpoint(endpoint)->network_manager_.AddInterface(addr, ifname,
                                                         adapter_type);
  }
  void RemoveAddress(int endpoint, const SocketAddress& addr) {
    GetEndpoint(endpoint)->network_manager_.RemoveInterface(addr);
  }
  void SetProxy(int endpoint, rtc::ProxyType type) {
    rtc::ProxyInfo info;
    info.type = type;
    info.address = (type == rtc::PROXY_HTTPS) ?
        kHttpsProxyAddrs[endpoint] : kSocksProxyAddrs[endpoint];
    GetAllocator(endpoint)->set_proxy("unittest/1.0", info);
  }
  void SetAllocatorFlags(int endpoint, int flags) {
    GetAllocator(endpoint)->set_flags(flags);
  }
  void SetIceRole(int endpoint, cricket::IceRole role) {
    GetEndpoint(endpoint)->SetIceRole(role);
  }
  void SetIceTiebreaker(int endpoint, uint64_t tiebreaker) {
    GetEndpoint(endpoint)->SetIceTiebreaker(tiebreaker);
  }
  bool GetRoleConflict(int endpoint) {
    return GetEndpoint(endpoint)->role_conflict();
  }
  void SetAllocationStepDelay(int endpoint, uint32_t delay) {
    return GetEndpoint(endpoint)->SetAllocationStepDelay(delay);
  }
  void SetAllowTcpListen(int endpoint, bool allow_tcp_listen) {
    return GetEndpoint(endpoint)->SetAllowTcpListen(allow_tcp_listen);
  }
  bool IsLocalToPrflxOrTheReverse(const Result& expected) {
    return (
        (expected.local_type == "local" && expected.remote_type == "prflx") ||
        (expected.local_type == "prflx" && expected.remote_type == "local"));
  }

  // Return true if the approprite parts of the expected Result, based
  // on the local and remote candidate of ep1_ch1, match.  This can be
  // used in an EXPECT_TRUE_WAIT.
  bool CheckCandidate1(const Result& expected) {
    const std::string& local_type = LocalCandidate(ep1_ch1())->type();
    const std::string& local_proto = LocalCandidate(ep1_ch1())->protocol();
    const std::string& remote_type = RemoteCandidate(ep1_ch1())->type();
    const std::string& remote_proto = RemoteCandidate(ep1_ch1())->protocol();
    return ((local_proto == expected.local_proto &&
             remote_proto == expected.remote_proto) &&
            ((local_type == expected.local_type &&
              remote_type == expected.remote_type) ||
             // Sometimes we expect local -> prflx or prflx -> local
             // and instead get prflx -> local or local -> prflx, and
             // that's OK.
             (IsLocalToPrflxOrTheReverse(expected) &&
              local_type == expected.remote_type &&
              remote_type == expected.local_type)));
  }

  // EXPECT_EQ on the approprite parts of the expected Result, based
  // on the local and remote candidate of ep1_ch1.  This is like
  // CheckCandidate1, except that it will provide more detail about
  // what didn't match.
  void ExpectCandidate1(const Result& expected) {
    if (CheckCandidate1(expected)) {
      return;
    }

    const std::string& local_type = LocalCandidate(ep1_ch1())->type();
    const std::string& local_proto = LocalCandidate(ep1_ch1())->protocol();
    const std::string& remote_type = RemoteCandidate(ep1_ch1())->type();
    const std::string& remote_proto = RemoteCandidate(ep1_ch1())->protocol();
    EXPECT_EQ(expected.local_type, local_type);
    EXPECT_EQ(expected.remote_type, remote_type);
    EXPECT_EQ(expected.local_proto, local_proto);
    EXPECT_EQ(expected.remote_proto, remote_proto);
  }

  // Return true if the approprite parts of the expected Result, based
  // on the local and remote candidate of ep2_ch1, match.  This can be
  // used in an EXPECT_TRUE_WAIT.
  bool CheckCandidate2(const Result& expected) {
    const std::string& local_type = LocalCandidate(ep2_ch1())->type();
    // const std::string& remote_type = RemoteCandidate(ep2_ch1())->type();
    const std::string& local_proto = LocalCandidate(ep2_ch1())->protocol();
    const std::string& remote_proto = RemoteCandidate(ep2_ch1())->protocol();
    // Removed remote_type comparision aginst best connection remote
    // candidate. This is done to handle remote type discrepancy from
    // local to stun based on the test type.
    // For example in case of Open -> NAT, ep2 channels will have LULU
    // and in other cases like NAT -> NAT it will be LUSU. To avoid these
    // mismatches and we are doing comparision in different way.
    // i.e. when don't match its remote type is either local or stun.
    // TODO(ronghuawu): Refine the test criteria.
    // https://code.google.com/p/webrtc/issues/detail?id=1953
    return ((local_proto == expected.local_proto2 &&
             remote_proto == expected.remote_proto2) &&
            (local_type == expected.local_type2 ||
             // Sometimes we expect local -> prflx or prflx -> local
             // and instead get prflx -> local or local -> prflx, and
             // that's OK.
             (IsLocalToPrflxOrTheReverse(expected) &&
              local_type == expected.remote_type2)));
  }

  // EXPECT_EQ on the approprite parts of the expected Result, based
  // on the local and remote candidate of ep2_ch1.  This is like
  // CheckCandidate2, except that it will provide more detail about
  // what didn't match.
  void ExpectCandidate2(const Result& expected) {
    if (CheckCandidate2(expected)) {
      return;
    }

    const std::string& local_type = LocalCandidate(ep2_ch1())->type();
    const std::string& local_proto = LocalCandidate(ep2_ch1())->protocol();
    const std::string& remote_type = RemoteCandidate(ep2_ch1())->type();
    const std::string& remote_proto = RemoteCandidate(ep2_ch1())->protocol();
    EXPECT_EQ(expected.local_type2, local_type);
    EXPECT_EQ(expected.remote_type2, remote_type);
    EXPECT_EQ(expected.local_proto2, local_proto);
    EXPECT_EQ(expected.remote_proto2, remote_proto);
  }

  void Test(const Result& expected) {
    int64_t connect_start = rtc::TimeMillis();
    int64_t connect_time;

    // Create the channels and wait for them to connect.
    CreateChannels(1);
    EXPECT_TRUE_WAIT_MARGIN(ep1_ch1() != NULL &&
                            ep2_ch1() != NULL &&
                            ep1_ch1()->receiving() &&
                            ep1_ch1()->writable() &&
                            ep2_ch1()->receiving() &&
                            ep2_ch1()->writable(),
                            expected.connect_wait,
                            1000);
    connect_time = rtc::TimeMillis() - connect_start;
    if (connect_time < expected.connect_wait) {
      LOG(LS_INFO) << "Connect time: " << connect_time << " ms";
    } else {
      LOG(LS_INFO) << "Connect time: " << "TIMEOUT ("
                   << expected.connect_wait << " ms)";
    }

    // Allow a few turns of the crank for the best connections to emerge.
    // This may take up to 2 seconds.
    if (ep1_ch1()->best_connection() &&
        ep2_ch1()->best_connection()) {
      int64_t converge_start = rtc::TimeMillis();
      int64_t converge_time;
      int64_t converge_wait = 2000;
      // Verifying local and remote channel best connection information. This is
      // done only for the RFC 5245 as controlled agent will use USE-CANDIDATE
      // from controlling (ep1) agent. We can easily predict from EP1 result
      // matrix.
      EXPECT_TRUE_WAIT_MARGIN(
          CheckCandidate1(expected) && CheckCandidate2(expected), converge_wait,
          converge_wait);
      // Also do EXPECT_EQ on each part so that failures are more verbose.
      ExpectCandidate1(expected);
      ExpectCandidate2(expected);

      converge_time = rtc::TimeMillis() - converge_start;
      if (converge_time < converge_wait) {
        LOG(LS_INFO) << "Converge time: " << converge_time << " ms";
      } else {
        LOG(LS_INFO) << "Converge time: " << "TIMEOUT ("
                     << converge_wait << " ms)";
      }
    }
    // Try sending some data to other end.
    TestSendRecv(1);

    // Destroy the channels, and wait for them to be fully cleaned up.
    DestroyChannels();
  }

  void TestSendRecv(int channels) {
    for (int i = 0; i < 10; ++i) {
      const char* data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
      int len = static_cast<int>(strlen(data));
      // local_channel1 <==> remote_channel1
      EXPECT_EQ_WAIT(len, SendData(ep1_ch1(), data, len), 1000);
      EXPECT_TRUE_WAIT(CheckDataOnChannel(ep2_ch1(), data, len), 1000);
      EXPECT_EQ_WAIT(len, SendData(ep2_ch1(), data, len), 1000);
      EXPECT_TRUE_WAIT(CheckDataOnChannel(ep1_ch1(), data, len), 1000);
      if (channels == 2 && ep1_ch2() && ep2_ch2()) {
        // local_channel2 <==> remote_channel2
        EXPECT_EQ_WAIT(len, SendData(ep1_ch2(), data, len), 1000);
        EXPECT_TRUE_WAIT(CheckDataOnChannel(ep2_ch2(), data, len), 1000);
        EXPECT_EQ_WAIT(len, SendData(ep2_ch2(), data, len), 1000);
        EXPECT_TRUE_WAIT(CheckDataOnChannel(ep1_ch2(), data, len), 1000);
      }
    }
  }

  // This test waits for the transport to become receiving and writable on both
  // end points. Once they are, the end points set new local ice credentials and
  // restart the ice gathering. Finally it waits for the transport to select a
  // new connection using the newly generated ice candidates.
  // Before calling this function the end points must be configured.
  void TestHandleIceUfragPasswordChanged() {
    ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[1], kIcePwd[1]);
    ep2_ch1()->SetRemoteIceCredentials(kIceUfrag[0], kIcePwd[0]);
    EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                            ep2_ch1()->receiving() && ep2_ch1()->writable(),
                            1000, 1000);

    const cricket::Candidate* old_local_candidate1 = LocalCandidate(ep1_ch1());
    const cricket::Candidate* old_local_candidate2 = LocalCandidate(ep2_ch1());
    const cricket::Candidate* old_remote_candidate1 =
        RemoteCandidate(ep1_ch1());
    const cricket::Candidate* old_remote_candidate2 =
        RemoteCandidate(ep2_ch1());

    ep1_ch1()->SetIceCredentials(kIceUfrag[2], kIcePwd[2]);
    ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[3], kIcePwd[3]);
    ep1_ch1()->MaybeStartGathering();
    ep2_ch1()->SetIceCredentials(kIceUfrag[3], kIcePwd[3]);
    ep2_ch1()->SetRemoteIceCredentials(kIceUfrag[2], kIcePwd[2]);
    ep2_ch1()->MaybeStartGathering();

    EXPECT_TRUE_WAIT_MARGIN(LocalCandidate(ep1_ch1())->generation() !=
                            old_local_candidate1->generation(),
                            1000, 1000);
    EXPECT_TRUE_WAIT_MARGIN(LocalCandidate(ep2_ch1())->generation() !=
                            old_local_candidate2->generation(),
                            1000, 1000);
    EXPECT_TRUE_WAIT_MARGIN(RemoteCandidate(ep1_ch1())->generation() !=
                            old_remote_candidate1->generation(),
                            1000, 1000);
    EXPECT_TRUE_WAIT_MARGIN(RemoteCandidate(ep2_ch1())->generation() !=
                            old_remote_candidate2->generation(),
                            1000, 1000);
    EXPECT_EQ(1u, RemoteCandidate(ep2_ch1())->generation());
    EXPECT_EQ(1u, RemoteCandidate(ep1_ch1())->generation());
  }

  void TestSignalRoleConflict() {
    SetIceTiebreaker(0, kTiebreaker1);  // Default EP1 is in controlling state.

    SetIceRole(1, cricket::ICEROLE_CONTROLLING);
    SetIceTiebreaker(1, kTiebreaker2);

    // Creating channels with both channels role set to CONTROLLING.
    CreateChannels(1);
    // Since both the channels initiated with controlling state and channel2
    // has higher tiebreaker value, channel1 should receive SignalRoleConflict.
    EXPECT_TRUE_WAIT(GetRoleConflict(0), 1000);
    EXPECT_FALSE(GetRoleConflict(1));

    EXPECT_TRUE_WAIT(ep1_ch1()->receiving() &&
                     ep1_ch1()->writable() &&
                     ep2_ch1()->receiving() &&
                     ep2_ch1()->writable(),
                     1000);

    EXPECT_TRUE(ep1_ch1()->best_connection() &&
                ep2_ch1()->best_connection());

    TestSendRecv(1);
  }

  // We pass the candidates directly to the other side.
  void OnCandidateGathered(cricket::TransportChannelImpl* ch,
                           const cricket::Candidate& c) {
    if (force_relay_ && c.type() != cricket::RELAY_PORT_TYPE)
      return;

    if (GetEndpoint(ch)->save_candidates_) {
      GetEndpoint(ch)->saved_candidates_.push_back(new CandidatesData(ch, c));
    } else {
      main_->Post(RTC_FROM_HERE, this, MSG_ADD_CANDIDATES,
                  new CandidatesData(ch, c));
    }
  }

  void PauseCandidates(int endpoint) {
    GetEndpoint(endpoint)->save_candidates_ = true;
  }

  void OnCandidatesRemoved(cricket::TransportChannelImpl* ch,
                           const std::vector<cricket::Candidate>& candidates) {
    // Candidate removals are not paused.
    CandidatesData* candidates_data = new CandidatesData(ch, candidates);
    main_->Post(RTC_FROM_HERE, this, MSG_REMOVE_CANDIDATES, candidates_data);
  }

  // Tcp candidate verification has to be done when they are generated.
  void VerifySavedTcpCandidates(int endpoint, const std::string& tcptype) {
    for (auto& data : GetEndpoint(endpoint)->saved_candidates_) {
      for (auto& candidate : data->candidates) {
        EXPECT_EQ(candidate.protocol(), cricket::TCP_PROTOCOL_NAME);
        EXPECT_EQ(candidate.tcptype(), tcptype);
        if (candidate.tcptype() == cricket::TCPTYPE_ACTIVE_STR) {
          EXPECT_EQ(candidate.address().port(), cricket::DISCARD_PORT);
        } else if (candidate.tcptype() == cricket::TCPTYPE_PASSIVE_STR) {
          EXPECT_NE(candidate.address().port(), cricket::DISCARD_PORT);
        } else {
          FAIL() << "Unknown tcptype: " << candidate.tcptype();
        }
      }
    }
  }

  void ResumeCandidates(int endpoint) {
    Endpoint* ed = GetEndpoint(endpoint);
    std::vector<CandidatesData*>::iterator it = ed->saved_candidates_.begin();
    for (; it != ed->saved_candidates_.end(); ++it) {
      main_->Post(RTC_FROM_HERE, this, MSG_ADD_CANDIDATES, *it);
    }
    ed->saved_candidates_.clear();
    ed->save_candidates_ = false;
  }

  void OnMessage(rtc::Message* msg) {
    switch (msg->message_id) {
      case MSG_ADD_CANDIDATES: {
        std::unique_ptr<CandidatesData> data(
            static_cast<CandidatesData*>(msg->pdata));
        cricket::P2PTransportChannel* rch = GetRemoteChannel(data->channel);
        for (auto& c : data->candidates) {
          if (clear_remote_candidates_ufrag_pwd_) {
            c.set_username("");
            c.set_password("");
          }
          LOG(LS_INFO) << "Candidate(" << data->channel->component() << "->"
                       << rch->component() << "): " << c.ToString();
          rch->AddRemoteCandidate(c);
        }
        break;
      }
      case MSG_REMOVE_CANDIDATES: {
        std::unique_ptr<CandidatesData> data(
            static_cast<CandidatesData*>(msg->pdata));
        cricket::P2PTransportChannel* rch = GetRemoteChannel(data->channel);
        for (cricket::Candidate& c : data->candidates) {
          LOG(LS_INFO) << "Removed remote candidate " << c.ToString();
          rch->RemoveRemoteCandidate(c);
        }
        break;
      }
    }
  }
  void OnReadPacket(cricket::TransportChannel* channel, const char* data,
                    size_t len, const rtc::PacketTime& packet_time,
                    int flags) {
    std::list<std::string>& packets = GetPacketList(channel);
    packets.push_front(std::string(data, len));
  }
  void OnRoleConflict(cricket::TransportChannelImpl* channel) {
    GetEndpoint(channel)->OnRoleConflict(true);
    cricket::IceRole new_role =
        GetEndpoint(channel)->ice_role() == cricket::ICEROLE_CONTROLLING ?
            cricket::ICEROLE_CONTROLLED : cricket::ICEROLE_CONTROLLING;
    channel->SetIceRole(new_role);
  }

  int SendData(cricket::TransportChannel* channel,
               const char* data, size_t len) {
    rtc::PacketOptions options;
    return channel->SendPacket(data, len, options, 0);
  }
  bool CheckDataOnChannel(cricket::TransportChannel* channel,
                          const char* data, int len) {
    return GetChannelData(channel)->CheckData(data, len);
  }
  static const cricket::Candidate* LocalCandidate(
      cricket::P2PTransportChannel* ch) {
    return (ch && ch->best_connection()) ?
        &ch->best_connection()->local_candidate() : NULL;
  }
  static const cricket::Candidate* RemoteCandidate(
      cricket::P2PTransportChannel* ch) {
    return (ch && ch->best_connection()) ?
        &ch->best_connection()->remote_candidate() : NULL;
  }
  Endpoint* GetEndpoint(cricket::TransportChannel* ch) {
    if (ep1_.HasChannel(ch)) {
      return &ep1_;
    } else if (ep2_.HasChannel(ch)) {
      return &ep2_;
    } else {
      return NULL;
    }
  }
  cricket::P2PTransportChannel* GetRemoteChannel(
      cricket::TransportChannel* ch) {
    if (ch == ep1_ch1())
      return ep2_ch1();
    else if (ch == ep1_ch2())
      return ep2_ch2();
    else if (ch == ep2_ch1())
      return ep1_ch1();
    else if (ch == ep2_ch2())
      return ep1_ch2();
    else
      return NULL;
  }
  std::list<std::string>& GetPacketList(cricket::TransportChannel* ch) {
    return GetChannelData(ch)->ch_packets_;
  }

  void set_clear_remote_candidates_ufrag_pwd(bool clear) {
    clear_remote_candidates_ufrag_pwd_ = clear;
  }

  void set_force_relay(bool relay) {
    force_relay_ = relay;
  }

 private:
  rtc::Thread* main_;
  std::unique_ptr<rtc::PhysicalSocketServer> pss_;
  std::unique_ptr<rtc::VirtualSocketServer> vss_;
  std::unique_ptr<rtc::NATSocketServer> nss_;
  std::unique_ptr<rtc::FirewallSocketServer> ss_;
  rtc::SocketServerScope ss_scope_;
  std::unique_ptr<cricket::TestStunServer> stun_server_;
  cricket::TestTurnServer turn_server_;
  cricket::TestRelayServer relay_server_;
  rtc::SocksProxyServer socks_server1_;
  rtc::SocksProxyServer socks_server2_;
  Endpoint ep1_;
  Endpoint ep2_;
  bool clear_remote_candidates_ufrag_pwd_;
  bool force_relay_;
};

// The tests have only a few outcomes, which we predefine.
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalUdpToLocalUdp("local", "udp", "local", "udp",
                        "local", "udp", "local", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalUdpToStunUdp("local", "udp", "stun", "udp",
                       "local", "udp", "stun", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalUdpToPrflxUdp("local", "udp", "prflx", "udp",
                        "prflx", "udp", "local", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kPrflxUdpToLocalUdp("prflx", "udp", "local", "udp",
                        "local", "udp", "prflx", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kStunUdpToLocalUdp("stun", "udp", "local", "udp",
                       "local", "udp", "stun", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kStunUdpToStunUdp("stun", "udp", "stun", "udp",
                      "stun", "udp", "stun", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kPrflxUdpToStunUdp("prflx", "udp", "stun", "udp",
                       "local", "udp", "prflx", "udp", 1000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalUdpToRelayUdp("local", "udp", "relay", "udp",
                        "relay", "udp", "local", "udp", 2000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kPrflxUdpToRelayUdp("prflx", "udp", "relay", "udp",
                        "relay", "udp", "prflx", "udp", 2000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalTcpToLocalTcp("local", "tcp", "local", "tcp",
                        "local", "tcp", "local", "tcp", 3000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kLocalTcpToPrflxTcp("local", "tcp", "prflx", "tcp",
                        "prflx", "tcp", "local", "tcp", 3000);
const P2PTransportChannelTestBase::Result P2PTransportChannelTestBase::
    kPrflxTcpToLocalTcp("prflx", "tcp", "local", "tcp",
                        "local", "tcp", "prflx", "tcp", 3000);

// Test the matrix of all the connectivity types we expect to see in the wild.
// Just test every combination of the configs in the Config enum.
class P2PTransportChannelTest : public P2PTransportChannelTestBase {
 protected:
  static const Result* kMatrix[NUM_CONFIGS][NUM_CONFIGS];
  void ConfigureEndpoints(Config config1,
                          Config config2,
                          int allocator_flags1,
                          int allocator_flags2) {
    ServerAddresses stun_servers;
    stun_servers.insert(kStunAddr);
    GetEndpoint(0)->allocator_.reset(
        new cricket::BasicPortAllocator(&(GetEndpoint(0)->network_manager_),
        stun_servers,
        rtc::SocketAddress(), rtc::SocketAddress(),
        rtc::SocketAddress()));
    GetEndpoint(1)->allocator_.reset(
        new cricket::BasicPortAllocator(&(GetEndpoint(1)->network_manager_),
        stun_servers,
        rtc::SocketAddress(), rtc::SocketAddress(),
        rtc::SocketAddress()));

    cricket::RelayServerConfig turn_server(cricket::RELAY_TURN);
    turn_server.credentials = kRelayCredentials;
    turn_server.ports.push_back(
        cricket::ProtocolAddress(kTurnUdpIntAddr, cricket::PROTO_UDP, false));
    GetEndpoint(0)->allocator_->AddTurnServer(turn_server);
    GetEndpoint(1)->allocator_->AddTurnServer(turn_server);

    int delay = kMinimumStepDelay;
    ConfigureEndpoint(0, config1);
    SetAllocatorFlags(0, allocator_flags1);
    SetAllocationStepDelay(0, delay);
    ConfigureEndpoint(1, config2);
    SetAllocatorFlags(1, allocator_flags2);
    SetAllocationStepDelay(1, delay);

    set_clear_remote_candidates_ufrag_pwd(true);
  }
  void ConfigureEndpoint(int endpoint, Config config) {
    switch (config) {
      case OPEN:
        AddAddress(endpoint, kPublicAddrs[endpoint]);
        break;
      case NAT_FULL_CONE:
      case NAT_ADDR_RESTRICTED:
      case NAT_PORT_RESTRICTED:
      case NAT_SYMMETRIC:
        AddAddress(endpoint, kPrivateAddrs[endpoint]);
        // Add a single NAT of the desired type
        nat()->AddTranslator(kPublicAddrs[endpoint], kNatAddrs[endpoint],
            static_cast<rtc::NATType>(config - NAT_FULL_CONE))->
            AddClient(kPrivateAddrs[endpoint]);
        break;
      case NAT_DOUBLE_CONE:
      case NAT_SYMMETRIC_THEN_CONE:
        AddAddress(endpoint, kCascadedPrivateAddrs[endpoint]);
        // Add a two cascaded NATs of the desired types
        nat()->AddTranslator(kPublicAddrs[endpoint], kNatAddrs[endpoint],
            (config == NAT_DOUBLE_CONE) ?
                rtc::NAT_OPEN_CONE : rtc::NAT_SYMMETRIC)->
            AddTranslator(kPrivateAddrs[endpoint], kCascadedNatAddrs[endpoint],
                rtc::NAT_OPEN_CONE)->
                AddClient(kCascadedPrivateAddrs[endpoint]);
        break;
      case BLOCK_UDP:
      case BLOCK_UDP_AND_INCOMING_TCP:
      case BLOCK_ALL_BUT_OUTGOING_HTTP:
      case PROXY_HTTPS:
      case PROXY_SOCKS:
        AddAddress(endpoint, kPublicAddrs[endpoint]);
        // Block all UDP
        fw()->AddRule(false, rtc::FP_UDP, rtc::FD_ANY,
                      kPublicAddrs[endpoint]);
        if (config == BLOCK_UDP_AND_INCOMING_TCP) {
          // Block TCP inbound to the endpoint
          fw()->AddRule(false, rtc::FP_TCP, SocketAddress(),
                        kPublicAddrs[endpoint]);
        } else if (config == BLOCK_ALL_BUT_OUTGOING_HTTP) {
          // Block all TCP to/from the endpoint except 80/443 out
          fw()->AddRule(true, rtc::FP_TCP, kPublicAddrs[endpoint],
                        SocketAddress(rtc::IPAddress(INADDR_ANY), 80));
          fw()->AddRule(true, rtc::FP_TCP, kPublicAddrs[endpoint],
                        SocketAddress(rtc::IPAddress(INADDR_ANY), 443));
          fw()->AddRule(false, rtc::FP_TCP, rtc::FD_ANY,
                        kPublicAddrs[endpoint]);
        } else if (config == PROXY_HTTPS) {
          // Block all TCP to/from the endpoint except to the proxy server
          fw()->AddRule(true, rtc::FP_TCP, kPublicAddrs[endpoint],
                        kHttpsProxyAddrs[endpoint]);
          fw()->AddRule(false, rtc::FP_TCP, rtc::FD_ANY,
                        kPublicAddrs[endpoint]);
          SetProxy(endpoint, rtc::PROXY_HTTPS);
        } else if (config == PROXY_SOCKS) {
          // Block all TCP to/from the endpoint except to the proxy server
          fw()->AddRule(true, rtc::FP_TCP, kPublicAddrs[endpoint],
                        kSocksProxyAddrs[endpoint]);
          fw()->AddRule(false, rtc::FP_TCP, rtc::FD_ANY,
                        kPublicAddrs[endpoint]);
          SetProxy(endpoint, rtc::PROXY_SOCKS5);
        }
        break;
      default:
        break;
    }
  }
};

// Shorthands for use in the test matrix.
#define LULU &kLocalUdpToLocalUdp
#define LUSU &kLocalUdpToStunUdp
#define LUPU &kLocalUdpToPrflxUdp
#define PULU &kPrflxUdpToLocalUdp
#define SULU &kStunUdpToLocalUdp
#define SUSU &kStunUdpToStunUdp
#define PUSU &kPrflxUdpToStunUdp
#define LURU &kLocalUdpToRelayUdp
#define PURU &kPrflxUdpToRelayUdp
#define LTLT &kLocalTcpToLocalTcp
#define LTPT &kLocalTcpToPrflxTcp
#define PTLT &kPrflxTcpToLocalTcp
// TODO: Enable these once TestRelayServer can accept external TCP.
#define LTRT NULL
#define LSRS NULL

// Test matrix. Originator behavior defined by rows, receiever by columns.

// TODO: Fix NULLs caused by lack of TCP support in NATSocket.
// TODO: Fix NULLs caused by no HTTP proxy support.
// TODO: Rearrange rows/columns from best to worst.
const P2PTransportChannelTest::Result* P2PTransportChannelTest::kMatrix[NUM_CONFIGS][NUM_CONFIGS] = {
    //      OPEN  CONE  ADDR  PORT  SYMM  2CON  SCON  !UDP  !TCP  HTTP  PRXH  PRXS
    /*OP*/ {LULU, LUSU, LUSU, LUSU, LUPU, LUSU, LUPU, PTLT, LTPT, LSRS, NULL, LTPT},
    /*CO*/ {LULU, LUSU, LUSU, LUSU, LUPU, LUSU, LUPU, NULL, NULL, LSRS, NULL, LTRT},
    /*AD*/ {LULU, LUSU, LUSU, LUSU, LUPU, LUSU, LUPU, NULL, NULL, LSRS, NULL, LTRT},
    /*PO*/ {LULU, LUSU, LUSU, LUSU, LURU, LUSU, LURU, NULL, NULL, LSRS, NULL, LTRT},
    /*SY*/ {PULU, PUSU, PUSU, PURU, PURU, PUSU, PURU, NULL, NULL, LSRS, NULL, LTRT},
    /*2C*/ {LULU, LUSU, LUSU, LUSU, LUPU, LUSU, LUPU, NULL, NULL, LSRS, NULL, LTRT},
    /*SC*/ {PULU, PUSU, PUSU, PURU, PURU, PUSU, PURU, NULL, NULL, LSRS, NULL, LTRT},
    /*!U*/ {PTLT, NULL, NULL, NULL, NULL, NULL, NULL, PTLT, LTPT, LSRS, NULL, LTRT},
    /*!T*/ {LTRT, NULL, NULL, NULL, NULL, NULL, NULL, PTLT, LTRT, LSRS, NULL, LTRT},
    /*HT*/ {LSRS, LSRS, LSRS, LSRS, LSRS, LSRS, LSRS, LSRS, LSRS, LSRS, NULL, LSRS},
    /*PR*/ {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    /*PR*/ {LTRT, LTRT, LTRT, LTRT, LTRT, LTRT, LTRT, LTRT, LTRT, LSRS, NULL, LTRT},
};

// The actual tests that exercise all the various configurations.
// Test names are of the form P2PTransportChannelTest_TestOPENToNAT_FULL_CONE
#define P2P_TEST_DECLARATION(x, y, z)                            \
  TEST_F(P2PTransportChannelTest, z##Test##x##To##y) {           \
    ConfigureEndpoints(x, y, PORTALLOCATOR_ENABLE_SHARED_SOCKET, \
                       PORTALLOCATOR_ENABLE_SHARED_SOCKET);      \
    if (kMatrix[x][y] != NULL)                                   \
      Test(*kMatrix[x][y]);                                      \
    else                                                         \
      LOG(LS_WARNING) << "Not yet implemented";                  \
  }

#define P2P_TEST(x, y) \
  P2P_TEST_DECLARATION(x, y,)

#define FLAKY_P2P_TEST(x, y) \
  P2P_TEST_DECLARATION(x, y, DISABLED_)

// TODO(holmer): Disabled due to randomly failing on webrtc buildbots.
// Issue: webrtc/2383
#define P2P_TEST_SET(x) \
  P2P_TEST(x, OPEN) \
  FLAKY_P2P_TEST(x, NAT_FULL_CONE) \
  FLAKY_P2P_TEST(x, NAT_ADDR_RESTRICTED) \
  FLAKY_P2P_TEST(x, NAT_PORT_RESTRICTED) \
  P2P_TEST(x, NAT_SYMMETRIC) \
  FLAKY_P2P_TEST(x, NAT_DOUBLE_CONE) \
  P2P_TEST(x, NAT_SYMMETRIC_THEN_CONE) \
  P2P_TEST(x, BLOCK_UDP) \
  P2P_TEST(x, BLOCK_UDP_AND_INCOMING_TCP) \
  P2P_TEST(x, BLOCK_ALL_BUT_OUTGOING_HTTP) \
  P2P_TEST(x, PROXY_HTTPS) \
  P2P_TEST(x, PROXY_SOCKS)

#define FLAKY_P2P_TEST_SET(x) \
  P2P_TEST(x, OPEN) \
  P2P_TEST(x, NAT_FULL_CONE) \
  P2P_TEST(x, NAT_ADDR_RESTRICTED) \
  P2P_TEST(x, NAT_PORT_RESTRICTED) \
  P2P_TEST(x, NAT_SYMMETRIC) \
  P2P_TEST(x, NAT_DOUBLE_CONE) \
  P2P_TEST(x, NAT_SYMMETRIC_THEN_CONE) \
  P2P_TEST(x, BLOCK_UDP) \
  P2P_TEST(x, BLOCK_UDP_AND_INCOMING_TCP) \
  P2P_TEST(x, BLOCK_ALL_BUT_OUTGOING_HTTP) \
  P2P_TEST(x, PROXY_HTTPS) \
  P2P_TEST(x, PROXY_SOCKS)

P2P_TEST_SET(OPEN)
P2P_TEST_SET(NAT_FULL_CONE)
P2P_TEST_SET(NAT_ADDR_RESTRICTED)
P2P_TEST_SET(NAT_PORT_RESTRICTED)
P2P_TEST_SET(NAT_SYMMETRIC)
P2P_TEST_SET(NAT_DOUBLE_CONE)
P2P_TEST_SET(NAT_SYMMETRIC_THEN_CONE)
P2P_TEST_SET(BLOCK_UDP)
P2P_TEST_SET(BLOCK_UDP_AND_INCOMING_TCP)
P2P_TEST_SET(BLOCK_ALL_BUT_OUTGOING_HTTP)
P2P_TEST_SET(PROXY_HTTPS)
P2P_TEST_SET(PROXY_SOCKS)

// Test that we restart candidate allocation when local ufrag&pwd changed.
// Standard Ice protocol is used.
TEST_F(P2PTransportChannelTest, HandleUfragPwdChange) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  CreateChannels(1);
  TestHandleIceUfragPasswordChanged();
  DestroyChannels();
}

// Test the operation of GetStats.
TEST_F(P2PTransportChannelTest, GetStats) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  CreateChannels(1);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                          ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  TestSendRecv(1);
  cricket::ConnectionInfos infos;
  ASSERT_TRUE(ep1_ch1()->GetStats(&infos));
  ASSERT_TRUE(infos.size() >= 1);
  cricket::ConnectionInfo* best_conn_info = nullptr;
  for (cricket::ConnectionInfo& info : infos) {
    if (info.best_connection) {
      best_conn_info = &info;
      break;
    }
  }
  ASSERT_TRUE(best_conn_info != nullptr);
  EXPECT_TRUE(best_conn_info->new_connection);
  EXPECT_TRUE(best_conn_info->receiving);
  EXPECT_TRUE(best_conn_info->writable);
  EXPECT_FALSE(best_conn_info->timeout);
  EXPECT_EQ(10U, best_conn_info->sent_total_packets);
  EXPECT_EQ(0U, best_conn_info->sent_discarded_packets);
  EXPECT_EQ(10 * 36U, best_conn_info->sent_total_bytes);
  EXPECT_EQ(10 * 36U, best_conn_info->recv_total_bytes);
  EXPECT_GT(best_conn_info->rtt, 0U);
  DestroyChannels();
}

// Test that we properly create a connection on a STUN ping from unknown address
// when the signaling is slow.
TEST_F(P2PTransportChannelTest, PeerReflexiveCandidateBeforeSignaling) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  // Emulate no remote credentials coming in.
  set_clear_remote_candidates_ufrag_pwd(false);
  CreateChannels(1);
  // Only have remote credentials come in for ep2, not ep1.
  ep2_ch1()->SetRemoteIceCredentials(kIceUfrag[0], kIcePwd[0]);

  // Pause sending ep2's candidates to ep1 until ep1 receives the peer reflexive
  // candidate.
  PauseCandidates(1);

  // The caller should have the best connection connected to the peer reflexive
  // candidate.
  const cricket::Connection* best_connection = NULL;
  WAIT((best_connection = ep1_ch1()->best_connection()) != NULL, 2000);
  EXPECT_EQ("prflx", ep1_ch1()->best_connection()->remote_candidate().type());

  // Because we don't have a remote pwd, we don't ping yet.
  EXPECT_EQ(kIceUfrag[1],
            ep1_ch1()->best_connection()->remote_candidate().username());
  EXPECT_EQ("", ep1_ch1()->best_connection()->remote_candidate().password());
  // Because we don't have ICE credentials yet, we don't know the generation.
  EXPECT_EQ(0u, ep1_ch1()->best_connection()->remote_candidate().generation());
  EXPECT_TRUE(nullptr == ep1_ch1()->FindNextPingableConnection());

  // Add two sets of remote ICE credentials, so that the ones used by the
  // candidate will be generation 1 instead of 0.
  ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[3], kIcePwd[3]);
  ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[1], kIcePwd[1]);
  // After setting the remote ICE credentials, the password and generation
  // of the peer reflexive candidate should be updated.
  EXPECT_EQ(kIcePwd[1],
            ep1_ch1()->best_connection()->remote_candidate().password());
  EXPECT_EQ(1u, ep1_ch1()->best_connection()->remote_candidate().generation());
  EXPECT_TRUE(nullptr != ep1_ch1()->FindNextPingableConnection());

  ResumeCandidates(1);

  WAIT(ep2_ch1()->best_connection() != NULL, 2000);
  // Verify ep1's best connection is updated to use the 'local' candidate.
  EXPECT_EQ_WAIT(
      "local",
      ep1_ch1()->best_connection()->remote_candidate().type(),
      2000);
  EXPECT_EQ(best_connection, ep1_ch1()->best_connection());
  DestroyChannels();
}

// Test that we properly create a connection on a STUN ping from unknown address
// when the signaling is slow and the end points are behind NAT.
TEST_F(P2PTransportChannelTest, PeerReflexiveCandidateBeforeSignalingWithNAT) {
  ConfigureEndpoints(OPEN, NAT_SYMMETRIC, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  // Emulate no remote credentials coming in.
  set_clear_remote_candidates_ufrag_pwd(false);
  CreateChannels(1);
  // Only have remote credentials come in for ep2, not ep1.
  ep2_ch1()->SetRemoteIceCredentials(kIceUfrag[0], kIcePwd[0]);
  // Pause sending ep2's candidates to ep1 until ep1 receives the peer reflexive
  // candidate.
  PauseCandidates(1);

  // The caller should have the best connection connected to the peer reflexive
  // candidate.
  WAIT(ep1_ch1()->best_connection() != NULL, 2000);
  EXPECT_EQ("prflx", ep1_ch1()->best_connection()->remote_candidate().type());

  // Because we don't have a remote pwd, we don't ping yet.
  EXPECT_EQ(kIceUfrag[1],
            ep1_ch1()->best_connection()->remote_candidate().username());
  EXPECT_EQ("", ep1_ch1()->best_connection()->remote_candidate().password());
  // Because we don't have ICE credentials yet, we don't know the generation.
  EXPECT_EQ(0u, ep1_ch1()->best_connection()->remote_candidate().generation());
  EXPECT_TRUE(nullptr == ep1_ch1()->FindNextPingableConnection());

  // Add two sets of remote ICE credentials, so that the ones used by the
  // candidate will be generation 1 instead of 0.
  ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[3], kIcePwd[3]);
  ep1_ch1()->SetRemoteIceCredentials(kIceUfrag[1], kIcePwd[1]);
  // After setting the remote ICE credentials, the password and generation
  // of the peer reflexive candidate should be updated.
  EXPECT_EQ(kIcePwd[1],
            ep1_ch1()->best_connection()->remote_candidate().password());
  EXPECT_EQ(1u, ep1_ch1()->best_connection()->remote_candidate().generation());

  ResumeCandidates(1);

  const cricket::Connection* best_connection = NULL;
  WAIT((best_connection = ep2_ch1()->best_connection()) != NULL, 2000);

  // Wait to verify the connection is not culled.
  WAIT(ep1_ch1()->writable(), 2000);
  EXPECT_EQ(ep2_ch1()->best_connection(), best_connection);
  EXPECT_EQ("prflx", ep1_ch1()->best_connection()->remote_candidate().type());
  DestroyChannels();
}

// Test that if remote candidates don't have ufrag and pwd, we still work.
TEST_F(P2PTransportChannelTest, RemoteCandidatesWithoutUfragPwd) {
  set_clear_remote_candidates_ufrag_pwd(true);
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  CreateChannels(1);
  const cricket::Connection* best_connection = NULL;
  // Wait until the callee's connections are created.
  WAIT((best_connection = ep2_ch1()->best_connection()) != NULL, 1000);
  // Wait to see if they get culled; they shouldn't.
  WAIT(ep2_ch1()->best_connection() != best_connection, 1000);
  EXPECT_TRUE(ep2_ch1()->best_connection() == best_connection);
  DestroyChannels();
}

// Test that a host behind NAT cannot be reached when incoming_only
// is set to true.
TEST_F(P2PTransportChannelTest, IncomingOnlyBlocked) {
  ConfigureEndpoints(NAT_FULL_CONE, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);

  SetAllocatorFlags(0, kOnlyLocalPorts);
  CreateChannels(1);
  ep1_ch1()->set_incoming_only(true);

  // Pump for 1 second and verify that the channels are not connected.
  rtc::Thread::Current()->ProcessMessages(1000);

  EXPECT_FALSE(ep1_ch1()->receiving());
  EXPECT_FALSE(ep1_ch1()->writable());
  EXPECT_FALSE(ep2_ch1()->receiving());
  EXPECT_FALSE(ep2_ch1()->writable());

  DestroyChannels();
}

// Test that a peer behind NAT can connect to a peer that has
// incoming_only flag set.
TEST_F(P2PTransportChannelTest, IncomingOnlyOpen) {
  ConfigureEndpoints(OPEN, NAT_FULL_CONE, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);

  SetAllocatorFlags(0, kOnlyLocalPorts);
  CreateChannels(1);
  ep1_ch1()->set_incoming_only(true);

  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1() != NULL && ep2_ch1() != NULL &&
                          ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                          ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);

  DestroyChannels();
}

TEST_F(P2PTransportChannelTest, TestTcpConnectionsFromActiveToPassive) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);

  SetAllocationStepDelay(0, kMinimumStepDelay);
  SetAllocationStepDelay(1, kMinimumStepDelay);

  int kOnlyLocalTcpPorts = cricket::PORTALLOCATOR_DISABLE_UDP |
                           cricket::PORTALLOCATOR_DISABLE_STUN |
                           cricket::PORTALLOCATOR_DISABLE_RELAY;
  // Disable all protocols except TCP.
  SetAllocatorFlags(0, kOnlyLocalTcpPorts);
  SetAllocatorFlags(1, kOnlyLocalTcpPorts);

  SetAllowTcpListen(0, true);   // actpass.
  SetAllowTcpListen(1, false);  // active.

  // Pause candidate so we could verify the candidate properties.
  PauseCandidates(0);
  PauseCandidates(1);
  CreateChannels(1);

  // Verify tcp candidates.
  VerifySavedTcpCandidates(0, cricket::TCPTYPE_PASSIVE_STR);
  VerifySavedTcpCandidates(1, cricket::TCPTYPE_ACTIVE_STR);

  // Resume candidates.
  ResumeCandidates(0);
  ResumeCandidates(1);

  EXPECT_TRUE_WAIT(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                   ep2_ch1()->receiving() && ep2_ch1()->writable(),
                   1000);
  EXPECT_TRUE(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[1]));

  TestSendRecv(1);
  DestroyChannels();
}

TEST_F(P2PTransportChannelTest, TestIceRoleConflict) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);
  TestSignalRoleConflict();
}

// Tests that the ice configs (protocol, tiebreaker and role) can be passed
// down to ports.
// Disable on Windows because it is flaky.
// https://bugs.chromium.org/p/webrtc/issues/detail?id=6019
#if defined(WEBRTC_WIN)
#define MAYBE_TestIceConfigWillPassDownToPort DISABLED_TestIceConfigWillPassDownToPort
#else
#define MAYBE_TestIceConfigWillPassDownToPort TestIceConfigWillPassDownToPort
#endif
TEST_F(P2PTransportChannelTest, MAYBE_TestIceConfigWillPassDownToPort) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);

  SetIceRole(0, cricket::ICEROLE_CONTROLLING);
  SetIceTiebreaker(0, kTiebreaker1);
  SetIceRole(1, cricket::ICEROLE_CONTROLLING);
  SetIceTiebreaker(1, kTiebreaker2);

  CreateChannels(1);

  EXPECT_EQ_WAIT(2u, ep1_ch1()->ports().size(), 1000);

  const std::vector<cricket::PortInterface *> ports_before = ep1_ch1()->ports();
  for (size_t i = 0; i < ports_before.size(); ++i) {
    EXPECT_EQ(cricket::ICEROLE_CONTROLLING, ports_before[i]->GetIceRole());
    EXPECT_EQ(kTiebreaker1, ports_before[i]->IceTiebreaker());
  }

  ep1_ch1()->SetIceRole(cricket::ICEROLE_CONTROLLED);
  ep1_ch1()->SetIceTiebreaker(kTiebreaker2);

  const std::vector<cricket::PortInterface *> ports_after = ep1_ch1()->ports();
  for (size_t i = 0; i < ports_after.size(); ++i) {
    EXPECT_EQ(cricket::ICEROLE_CONTROLLED, ports_before[i]->GetIceRole());
    // SetIceTiebreaker after Connect() has been called will fail. So expect the
    // original value.
    EXPECT_EQ(kTiebreaker1, ports_before[i]->IceTiebreaker());
  }

  EXPECT_TRUE_WAIT(ep1_ch1()->receiving() &&
                   ep1_ch1()->writable() &&
                   ep2_ch1()->receiving() &&
                   ep2_ch1()->writable(),
                   1000);

  EXPECT_TRUE(ep1_ch1()->best_connection() &&
              ep2_ch1()->best_connection());

  TestSendRecv(1);
  DestroyChannels();
}

// Verify that we can set DSCP value and retrieve properly from P2PTC.
TEST_F(P2PTransportChannelTest, TestDefaultDscpValue) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);

  CreateChannels(1);
  EXPECT_EQ(rtc::DSCP_NO_CHANGE,
            GetEndpoint(0)->cd1_.ch_->DefaultDscpValue());
  EXPECT_EQ(rtc::DSCP_NO_CHANGE,
            GetEndpoint(1)->cd1_.ch_->DefaultDscpValue());
  GetEndpoint(0)->cd1_.ch_->SetOption(
      rtc::Socket::OPT_DSCP, rtc::DSCP_CS6);
  GetEndpoint(1)->cd1_.ch_->SetOption(
      rtc::Socket::OPT_DSCP, rtc::DSCP_CS6);
  EXPECT_EQ(rtc::DSCP_CS6,
            GetEndpoint(0)->cd1_.ch_->DefaultDscpValue());
  EXPECT_EQ(rtc::DSCP_CS6,
            GetEndpoint(1)->cd1_.ch_->DefaultDscpValue());
  GetEndpoint(0)->cd1_.ch_->SetOption(
      rtc::Socket::OPT_DSCP, rtc::DSCP_AF41);
  GetEndpoint(1)->cd1_.ch_->SetOption(
      rtc::Socket::OPT_DSCP, rtc::DSCP_AF41);
  EXPECT_EQ(rtc::DSCP_AF41,
            GetEndpoint(0)->cd1_.ch_->DefaultDscpValue());
  EXPECT_EQ(rtc::DSCP_AF41,
            GetEndpoint(1)->cd1_.ch_->DefaultDscpValue());
}

// Verify IPv6 connection is preferred over IPv4.
TEST_F(P2PTransportChannelTest, TestIPv6Connections) {
  AddAddress(0, kIPv6PublicAddrs[0]);
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kIPv6PublicAddrs[1]);
  AddAddress(1, kPublicAddrs[1]);

  SetAllocationStepDelay(0, kMinimumStepDelay);
  SetAllocationStepDelay(1, kMinimumStepDelay);

  // Enable IPv6
  SetAllocatorFlags(0, cricket::PORTALLOCATOR_ENABLE_IPV6);
  SetAllocatorFlags(1, cricket::PORTALLOCATOR_ENABLE_IPV6);

  CreateChannels(1);

  EXPECT_TRUE_WAIT(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                   ep2_ch1()->receiving() && ep2_ch1()->writable(),
                   1000);
  EXPECT_TRUE(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kIPv6PublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kIPv6PublicAddrs[1]));

  TestSendRecv(1);
  DestroyChannels();
}

// Testing forceful TURN connections.
TEST_F(P2PTransportChannelTest, TestForceTurn) {
  ConfigureEndpoints(
      NAT_PORT_RESTRICTED, NAT_SYMMETRIC,
      kDefaultPortAllocatorFlags | cricket::PORTALLOCATOR_ENABLE_SHARED_SOCKET,
      kDefaultPortAllocatorFlags | cricket::PORTALLOCATOR_ENABLE_SHARED_SOCKET);
  set_force_relay(true);

  SetAllocationStepDelay(0, kMinimumStepDelay);
  SetAllocationStepDelay(1, kMinimumStepDelay);

  CreateChannels(1);

  EXPECT_TRUE_WAIT(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                       ep2_ch1()->receiving() && ep2_ch1()->writable(),
                   2000);

  EXPECT_TRUE(ep1_ch1()->best_connection() &&
              ep2_ch1()->best_connection());

  EXPECT_EQ("relay", RemoteCandidate(ep1_ch1())->type());
  EXPECT_EQ("relay", LocalCandidate(ep1_ch1())->type());
  EXPECT_EQ("relay", RemoteCandidate(ep2_ch1())->type());
  EXPECT_EQ("relay", LocalCandidate(ep2_ch1())->type());

  TestSendRecv(1);
  DestroyChannels();
}

// Test that if continual gathering is set to true, ICE gathering state will
// not change to "Complete", and vice versa.
TEST_F(P2PTransportChannelTest, TestContinualGathering) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  SetAllocationStepDelay(0, kDefaultStepDelay);
  SetAllocationStepDelay(1, kDefaultStepDelay);
  CreateChannels(1);
  cricket::IceConfig config = CreateIceConfig(1000, true);
  ep1_ch1()->SetIceConfig(config);
  // By default, ep2 does not gather continually.

  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1() != NULL && ep2_ch1() != NULL &&
                              ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  WAIT(cricket::IceGatheringState::kIceGatheringComplete ==
           ep1_ch1()->gathering_state(),
       1000);
  EXPECT_EQ(cricket::IceGatheringState::kIceGatheringGathering,
            ep1_ch1()->gathering_state());
  // By now, ep2 should have completed gathering.
  EXPECT_EQ(cricket::IceGatheringState::kIceGatheringComplete,
            ep2_ch1()->gathering_state());

  DestroyChannels();
}

// Test that a connection succeeds when the P2PTransportChannel uses a pooled
// PortAllocatorSession that has not yet finished gathering candidates.
TEST_F(P2PTransportChannelTest, TestUsingPooledSessionBeforeDoneGathering) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  // First create a pooled session for each endpoint.
  auto& allocator_1 = GetEndpoint(0)->allocator_;
  auto& allocator_2 = GetEndpoint(1)->allocator_;
  int pool_size = 1;
  allocator_1->SetConfiguration(allocator_1->stun_servers(),
                                allocator_1->turn_servers(), pool_size);
  allocator_2->SetConfiguration(allocator_2->stun_servers(),
                                allocator_2->turn_servers(), pool_size);
  const cricket::PortAllocatorSession* pooled_session_1 =
      allocator_1->GetPooledSession();
  const cricket::PortAllocatorSession* pooled_session_2 =
      allocator_2->GetPooledSession();
  ASSERT_NE(nullptr, pooled_session_1);
  ASSERT_NE(nullptr, pooled_session_2);
  // Sanity check that pooled sessions haven't gathered anything yet.
  EXPECT_TRUE(pooled_session_1->ReadyPorts().empty());
  EXPECT_TRUE(pooled_session_1->ReadyCandidates().empty());
  EXPECT_TRUE(pooled_session_2->ReadyPorts().empty());
  EXPECT_TRUE(pooled_session_2->ReadyCandidates().empty());
  // Now let the endpoints connect and try exchanging some data.
  CreateChannels(1);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1() != NULL && ep2_ch1() != NULL &&
                              ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  TestSendRecv(1);
  // Make sure the P2PTransportChannels are actually using ports from the
  // pooled sessions.
  auto pooled_ports_1 = pooled_session_1->ReadyPorts();
  auto pooled_ports_2 = pooled_session_2->ReadyPorts();
  EXPECT_NE(pooled_ports_1.end(),
            std::find(pooled_ports_1.begin(), pooled_ports_1.end(),
                      ep1_ch1()->best_connection()->port()));
  EXPECT_NE(pooled_ports_2.end(),
            std::find(pooled_ports_2.begin(), pooled_ports_2.end(),
                      ep2_ch1()->best_connection()->port()));
}

// Test that a connection succeeds when the P2PTransportChannel uses a pooled
// PortAllocatorSession that already finished gathering candidates.
TEST_F(P2PTransportChannelTest, TestUsingPooledSessionAfterDoneGathering) {
  ConfigureEndpoints(OPEN, OPEN, kDefaultPortAllocatorFlags,
                     kDefaultPortAllocatorFlags);
  // First create a pooled session for each endpoint.
  auto& allocator_1 = GetEndpoint(0)->allocator_;
  auto& allocator_2 = GetEndpoint(1)->allocator_;
  int pool_size = 1;
  allocator_1->SetConfiguration(allocator_1->stun_servers(),
                                allocator_1->turn_servers(), pool_size);
  allocator_2->SetConfiguration(allocator_2->stun_servers(),
                                allocator_2->turn_servers(), pool_size);
  const cricket::PortAllocatorSession* pooled_session_1 =
      allocator_1->GetPooledSession();
  const cricket::PortAllocatorSession* pooled_session_2 =
      allocator_2->GetPooledSession();
  ASSERT_NE(nullptr, pooled_session_1);
  ASSERT_NE(nullptr, pooled_session_2);
  // Wait for the pooled sessions to finish gathering before the
  // P2PTransportChannels try to use them.
  EXPECT_TRUE_WAIT(pooled_session_1->CandidatesAllocationDone() &&
                       pooled_session_2->CandidatesAllocationDone(),
                   kDefaultTimeout);
  // Now let the endpoints connect and try exchanging some data.
  CreateChannels(1);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1() != NULL && ep2_ch1() != NULL &&
                              ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  TestSendRecv(1);
  // Make sure the P2PTransportChannels are actually using ports from the
  // pooled sessions.
  auto pooled_ports_1 = pooled_session_1->ReadyPorts();
  auto pooled_ports_2 = pooled_session_2->ReadyPorts();
  EXPECT_NE(pooled_ports_1.end(),
            std::find(pooled_ports_1.begin(), pooled_ports_1.end(),
                      ep1_ch1()->best_connection()->port()));
  EXPECT_NE(pooled_ports_2.end(),
            std::find(pooled_ports_2.begin(), pooled_ports_2.end(),
                      ep2_ch1()->best_connection()->port()));
}

// Test what happens when we have 2 users behind the same NAT. This can lead
// to interesting behavior because the STUN server will only give out the
// address of the outermost NAT.
class P2PTransportChannelSameNatTest : public P2PTransportChannelTestBase {
 protected:
  void ConfigureEndpoints(Config nat_type, Config config1, Config config2) {
    ASSERT(nat_type >= NAT_FULL_CONE && nat_type <= NAT_SYMMETRIC);
    rtc::NATSocketServer::Translator* outer_nat =
        nat()->AddTranslator(kPublicAddrs[0], kNatAddrs[0],
            static_cast<rtc::NATType>(nat_type - NAT_FULL_CONE));
    ConfigureEndpoint(outer_nat, 0, config1);
    ConfigureEndpoint(outer_nat, 1, config2);
  }
  void ConfigureEndpoint(rtc::NATSocketServer::Translator* nat,
                         int endpoint, Config config) {
    ASSERT(config <= NAT_SYMMETRIC);
    if (config == OPEN) {
      AddAddress(endpoint, kPrivateAddrs[endpoint]);
      nat->AddClient(kPrivateAddrs[endpoint]);
    } else {
      AddAddress(endpoint, kCascadedPrivateAddrs[endpoint]);
      nat->AddTranslator(kPrivateAddrs[endpoint], kCascadedNatAddrs[endpoint],
          static_cast<rtc::NATType>(config - NAT_FULL_CONE))->AddClient(
              kCascadedPrivateAddrs[endpoint]);
    }
  }
};

TEST_F(P2PTransportChannelSameNatTest, TestConesBehindSameCone) {
  ConfigureEndpoints(NAT_FULL_CONE, NAT_FULL_CONE, NAT_FULL_CONE);
  Test(P2PTransportChannelTestBase::Result(
      "prflx", "udp", "stun", "udp", "stun", "udp", "prflx", "udp", 1000));
}

// Test what happens when we have multiple available pathways.
// In the future we will try different RTTs and configs for the different
// interfaces, so that we can simulate a user with Ethernet and VPN networks.
class P2PTransportChannelMultihomedTest : public P2PTransportChannelTestBase {
};

// Test that we can establish connectivity when both peers are multihomed.
TEST_F(P2PTransportChannelMultihomedTest, DISABLED_TestBasic) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(0, kAlternateAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);
  AddAddress(1, kAlternateAddrs[1]);
  Test(kLocalUdpToLocalUdp);
}

// Test that we can quickly switch links if an interface goes down.
// The controlled side has two interfaces and one will die.
TEST_F(P2PTransportChannelMultihomedTest, TestFailoverControlledSide) {
  AddAddress(0, kPublicAddrs[0]);
  // Adding alternate address will make sure |kPublicAddrs| has the higher
  // priority than others. This is due to FakeNetwork::AddInterface method.
  AddAddress(1, kAlternateAddrs[1]);
  AddAddress(1, kPublicAddrs[1]);

  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);

  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  EXPECT_TRUE(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[1]));

  // Make the receiving timeout shorter for testing.
  cricket::IceConfig config = CreateIceConfig(1000, false);
  ep1_ch1()->SetIceConfig(config);
  ep2_ch1()->SetIceConfig(config);

  // Blackhole any traffic to or from the public addrs.
  LOG(LS_INFO) << "Failing over...";
  fw()->AddRule(false, rtc::FP_ANY, rtc::FD_ANY, kPublicAddrs[1]);
  // The best connections will switch, so keep references to them.
  const cricket::Connection* best_connection1 = ep1_ch1()->best_connection();
  const cricket::Connection* best_connection2 = ep2_ch1()->best_connection();
  // We should detect loss of receiving within 1 second or so.
  EXPECT_TRUE_WAIT(
      !best_connection1->receiving() && !best_connection2->receiving(), 3000);

  // We should switch over to use the alternate addr immediately on both sides
  // when we are not receiving.
  EXPECT_TRUE_WAIT(
      ep1_ch1()->best_connection()->receiving() &&
      ep2_ch1()->best_connection()->receiving(), 1000);
  EXPECT_TRUE(LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]));
  EXPECT_TRUE(
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kAlternateAddrs[1]));
  EXPECT_TRUE(
      LocalCandidate(ep2_ch1())->address().EqualIPs(kAlternateAddrs[1]));

  DestroyChannels();
}

// Test that we can quickly switch links if an interface goes down.
// The controlling side has two interfaces and one will die.
TEST_F(P2PTransportChannelMultihomedTest, TestFailoverControllingSide) {
  // Adding alternate address will make sure |kPublicAddrs| has the higher
  // priority than others. This is due to FakeNetwork::AddInterface method.
  AddAddress(0, kAlternateAddrs[0]);
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);

  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  EXPECT_TRUE(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[1]));

  // Make the receiving timeout shorter for testing.
  cricket::IceConfig config = CreateIceConfig(1000, false);
  ep1_ch1()->SetIceConfig(config);
  ep2_ch1()->SetIceConfig(config);

  // Blackhole any traffic to or from the public addrs.
  LOG(LS_INFO) << "Failing over...";
  fw()->AddRule(false, rtc::FP_ANY, rtc::FD_ANY, kPublicAddrs[0]);
  // The best connections will switch, so keep references to them.
  const cricket::Connection* best_connection1 = ep1_ch1()->best_connection();
  const cricket::Connection* best_connection2 = ep2_ch1()->best_connection();
  // We should detect loss of receiving within 1 second or so.
  EXPECT_TRUE_WAIT(
      !best_connection1->receiving() && !best_connection2->receiving(), 3000);

  // We should switch over to use the alternate addr immediately on both sides
  // when we are not receiving.
  EXPECT_TRUE_WAIT(
      ep1_ch1()->best_connection()->receiving() &&
      ep2_ch1()->best_connection()->receiving(), 1000);
  EXPECT_TRUE(
    LocalCandidate(ep1_ch1())->address().EqualIPs(kAlternateAddrs[0]));
  EXPECT_TRUE(RemoteCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[1]));
  EXPECT_TRUE(
      RemoteCandidate(ep2_ch1())->address().EqualIPs(kAlternateAddrs[0]));

  DestroyChannels();
}

// Tests that a Wifi-Wifi connection has the highest precedence.
TEST_F(P2PTransportChannelMultihomedTest, TestPreferWifiToWifiConnection) {
  // The interface names are chosen so that |cellular| would have higher
  // candidate priority if it is not for the network type.
  auto& wifi = kAlternateAddrs;
  auto& cellular = kPublicAddrs;
  AddAddress(0, wifi[0], "test0", rtc::ADAPTER_TYPE_WIFI);
  AddAddress(0, cellular[0], "test1", rtc::ADAPTER_TYPE_CELLULAR);
  AddAddress(1, wifi[1], "test0", rtc::ADAPTER_TYPE_WIFI);
  AddAddress(1, cellular[1], "test1", rtc::ADAPTER_TYPE_CELLULAR);

  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);

  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  // Need to wait to make sure the connections on both networks are writable.
  EXPECT_TRUE_WAIT(ep1_ch1()->best_connection() &&
                       LocalCandidate(ep1_ch1())->address().EqualIPs(wifi[0]) &&
                       RemoteCandidate(ep1_ch1())->address().EqualIPs(wifi[1]),
                   1000);
  EXPECT_TRUE_WAIT(ep2_ch1()->best_connection() &&
                       LocalCandidate(ep2_ch1())->address().EqualIPs(wifi[1]) &&
                       RemoteCandidate(ep2_ch1())->address().EqualIPs(wifi[0]),
                   1000);
}

// Tests that a Wifi-Cellular connection has higher precedence than
// a Cellular-Cellular connection.
TEST_F(P2PTransportChannelMultihomedTest, TestPreferWifiOverCellularNetwork) {
  // The interface names are chosen so that |cellular| would have higher
  // candidate priority if it is not for the network type.
  auto& wifi = kAlternateAddrs;
  auto& cellular = kPublicAddrs;
  AddAddress(0, cellular[0], "test1", rtc::ADAPTER_TYPE_CELLULAR);
  AddAddress(1, wifi[1], "test0", rtc::ADAPTER_TYPE_WIFI);
  AddAddress(1, cellular[1], "test1", rtc::ADAPTER_TYPE_CELLULAR);

  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);

  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  // Need to wait to make sure the connections on both networks are writable.
  EXPECT_TRUE_WAIT(ep1_ch1()->best_connection() &&
                       RemoteCandidate(ep1_ch1())->address().EqualIPs(wifi[1]),
                   1000);
  EXPECT_TRUE_WAIT(ep2_ch1()->best_connection() &&
                       LocalCandidate(ep2_ch1())->address().EqualIPs(wifi[1]),
                   1000);
}

// Test that the backup connection is pinged at a rate no faster than
// what was configured.
TEST_F(P2PTransportChannelMultihomedTest, TestPingBackupConnectionRate) {
  AddAddress(0, kPublicAddrs[0]);
  // Adding alternate address will make sure |kPublicAddrs| has the higher
  // priority than others. This is due to FakeNetwork::AddInterface method.
  AddAddress(1, kAlternateAddrs[1]);
  AddAddress(1, kPublicAddrs[1]);

  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  int backup_ping_interval = 2000;
  ep2_ch1()->SetIceConfig(CreateIceConfig(2000, false, backup_ping_interval));
  // After the state becomes COMPLETED, the backup connection will be pinged
  // once every |backup_ping_interval| milliseconds.
  ASSERT_TRUE_WAIT(ep2_ch1()->GetState() == cricket::STATE_COMPLETED, 1000);
  const std::vector<cricket::Connection*>& connections =
      ep2_ch1()->connections();
  ASSERT_EQ(2U, connections.size());
  cricket::Connection* backup_conn = connections[1];
  EXPECT_TRUE_WAIT(backup_conn->writable(), 3000);
  int64_t last_ping_response_ms = backup_conn->last_ping_response_received();
  EXPECT_TRUE_WAIT(
      last_ping_response_ms < backup_conn->last_ping_response_received(), 5000);
  int time_elapsed =
      backup_conn->last_ping_response_received() - last_ping_response_ms;
  LOG(LS_INFO) << "Time elapsed: " << time_elapsed;
  EXPECT_GE(time_elapsed, backup_ping_interval);
}

TEST_F(P2PTransportChannelMultihomedTest, TestGetState) {
  AddAddress(0, kAlternateAddrs[0]);
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);
  // Create channels and let them go writable, as usual.
  CreateChannels(1);

  // Both transport channels will reach STATE_COMPLETED quickly.
  EXPECT_EQ_WAIT(cricket::TransportChannelState::STATE_COMPLETED,
                 ep1_ch1()->GetState(), 1000);
  EXPECT_EQ_WAIT(cricket::TransportChannelState::STATE_COMPLETED,
                 ep2_ch1()->GetState(), 1000);
}

// Tests that when a network interface becomes inactive, if and only if
// Continual Gathering is enabled, the ports associated with that network
// will be removed from the port list of the channel, and the respective
// remote candidates on the other participant will be removed eventually.
TEST_F(P2PTransportChannelMultihomedTest, TestNetworkBecomesInactive) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);
  // Create channels and let them go writable, as usual.
  CreateChannels(1);
  ep1_ch1()->SetIceConfig(CreateIceConfig(2000, true));
  ep2_ch1()->SetIceConfig(CreateIceConfig(2000, false));

  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);
  EXPECT_TRUE_WAIT_MARGIN(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                              ep2_ch1()->receiving() && ep2_ch1()->writable(),
                          1000, 1000);
  // More than one port has been created.
  EXPECT_LE(1U, ep1_ch1()->ports().size());
  // Endpoint 1 enabled continual gathering; the port will be removed
  // when the interface is removed.
  RemoveAddress(0, kPublicAddrs[0]);
  EXPECT_TRUE(ep1_ch1()->ports().empty());
  // The remote candidates will be removed eventually.
  EXPECT_TRUE_WAIT(ep2_ch1()->remote_candidates().empty(), 1000);

  size_t num_ports = ep2_ch1()->ports().size();
  EXPECT_LE(1U, num_ports);
  size_t num_remote_candidates = ep1_ch1()->remote_candidates().size();
  // Endpoint 2 did not enable continual gathering; the port will not be removed
  // when the interface is removed and neither the remote candidates on the
  // other participant.
  RemoveAddress(1, kPublicAddrs[1]);
  rtc::Thread::Current()->ProcessMessages(500);
  EXPECT_EQ(num_ports, ep2_ch1()->ports().size());
  EXPECT_EQ(num_remote_candidates, ep1_ch1()->remote_candidates().size());
}

/*

TODO(pthatcher): Once have a way to handle network interfaces changes
without signalling an ICE restart, put a test like this back.  In the
mean time, this test only worked for GICE.  With ICE, it's currently
not possible without an ICE restart.

// Test that we can switch links in a coordinated fashion.
TEST_F(P2PTransportChannelMultihomedTest, TestDrain) {
  AddAddress(0, kPublicAddrs[0]);
  AddAddress(1, kPublicAddrs[1]);
  // Use only local ports for simplicity.
  SetAllocatorFlags(0, kOnlyLocalPorts);
  SetAllocatorFlags(1, kOnlyLocalPorts);

  // Create channels and let them go writable, as usual.
  CreateChannels(1);
  EXPECT_TRUE_WAIT(ep1_ch1()->receiving() && ep1_ch1()->writable() &&
                   ep2_ch1()->receiving() && ep2_ch1()->writable(),
                   1000);
  EXPECT_TRUE(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[1]));


  // Remove the public interface, add the alternate interface, and allocate
  // a new generation of candidates for the new interface (via
  // MaybeStartGathering()).
  LOG(LS_INFO) << "Draining...";
  AddAddress(1, kAlternateAddrs[1]);
  RemoveAddress(1, kPublicAddrs[1]);
  ep2_ch1()->MaybeStartGathering();

  // We should switch over to use the alternate address after
  // an exchange of pings.
  EXPECT_TRUE_WAIT(
      ep1_ch1()->best_connection() && ep2_ch1()->best_connection() &&
      LocalCandidate(ep1_ch1())->address().EqualIPs(kPublicAddrs[0]) &&
      RemoteCandidate(ep1_ch1())->address().EqualIPs(kAlternateAddrs[1]),
      3000);

  DestroyChannels();
}

*/

// A collection of tests which tests a single P2PTransportChannel by sending
// pings.
class P2PTransportChannelPingTest : public testing::Test,
                                    public sigslot::has_slots<> {
 public:
  P2PTransportChannelPingTest()
      : pss_(new rtc::PhysicalSocketServer),
        vss_(new rtc::VirtualSocketServer(pss_.get())),
        ss_scope_(vss_.get()) {}

 protected:
  void PrepareChannel(cricket::P2PTransportChannel* ch) {
    ch->SetIceRole(cricket::ICEROLE_CONTROLLING);
    ch->SetIceCredentials(kIceUfrag[0], kIcePwd[0]);
    ch->SetRemoteIceCredentials(kIceUfrag[1], kIcePwd[1]);
    ch->SignalSelectedCandidatePairChanged.connect(
        this, &P2PTransportChannelPingTest::OnSelectedCandidatePairChanged);
    ch->SignalReadyToSend.connect(this,
                                  &P2PTransportChannelPingTest::OnReadyToSend);
    ch->SignalStateChanged.connect(
        this, &P2PTransportChannelPingTest::OnChannelStateChanged);
  }

  cricket::Candidate CreateHostCandidate(const std::string& ip,
                                         int port,
                                         int priority,
                                         const std::string& ufrag = "") {
    cricket::Candidate c;
    c.set_address(rtc::SocketAddress(ip, port));
    c.set_component(1);
    c.set_protocol(cricket::UDP_PROTOCOL_NAME);
    c.set_priority(priority);
    c.set_username(ufrag);
    c.set_type(cricket::LOCAL_PORT_TYPE);
    return c;
  }

  cricket::Connection* WaitForConnectionTo(cricket::P2PTransportChannel* ch,
                                           const std::string& ip,
                                           int port_num) {
    EXPECT_TRUE_WAIT(GetConnectionTo(ch, ip, port_num) != nullptr, 3000);
    return GetConnectionTo(ch, ip, port_num);
  }

  cricket::Port* GetPort(cricket::P2PTransportChannel* ch) {
    if (ch->ports().empty()) {
      return nullptr;
    }
    return static_cast<cricket::Port*>(ch->ports()[0]);
  }

  cricket::Connection* GetConnectionTo(cricket::P2PTransportChannel* ch,
                                       const std::string& ip,
                                       int port_num) {
    cricket::Port* port = GetPort(ch);
    if (!port) {
      return nullptr;
    }
    return port->GetConnection(rtc::SocketAddress(ip, port_num));
  }

  cricket::Connection* FindNextPingableConnectionAndPingIt(
      cricket::P2PTransportChannel* ch) {
    cricket::Connection* conn = ch->FindNextPingableConnection();
    if (conn) {
      ch->MarkConnectionPinged(conn);
    }
    return conn;
  }

  int SendData(cricket::TransportChannel& channel,
               const char* data,
               size_t len,
               int packet_id) {
    rtc::PacketOptions options;
    options.packet_id = packet_id;
    return channel.SendPacket(data, len, options, 0);
  }

  void OnSelectedCandidatePairChanged(
      cricket::TransportChannel* transport_channel,
      cricket::CandidatePairInterface* selected_candidate_pair,
      int last_sent_packet_id) {
    last_selected_candidate_pair_ = selected_candidate_pair;
    last_sent_packet_id_ = last_sent_packet_id;
  }

  void ReceivePingOnConnection(cricket::Connection* conn,
                               const std::string& remote_ufrag,
                               int priority) {
    cricket::IceMessage msg;
    msg.SetType(cricket::STUN_BINDING_REQUEST);
    msg.AddAttribute(new cricket::StunByteStringAttribute(
        cricket::STUN_ATTR_USERNAME,
        conn->local_candidate().username() + ":" + remote_ufrag));
    msg.AddAttribute(new cricket::StunUInt32Attribute(
        cricket::STUN_ATTR_PRIORITY, priority));
    msg.SetTransactionID(
        rtc::CreateRandomString(cricket::kStunTransactionIdLength));
    msg.AddMessageIntegrity(conn->local_candidate().password());
    msg.AddFingerprint();
    rtc::ByteBufferWriter buf;
    msg.Write(&buf);
    conn->OnReadPacket(buf.Data(), buf.Length(), rtc::CreatePacketTime(0));
  }

  void OnReadyToSend(cricket::TransportChannel* channel) {
    channel_ready_to_send_ = true;
  }
  void OnChannelStateChanged(cricket::TransportChannelImpl* channel) {
    channel_state_ = channel->GetState();
  }

  cricket::CandidatePairInterface* last_selected_candidate_pair() {
    return last_selected_candidate_pair_;
  }
  int last_sent_packet_id() { return last_sent_packet_id_; }
  bool channel_ready_to_send() { return channel_ready_to_send_; }
  void reset_channel_ready_to_send() { channel_ready_to_send_ = false; }
  cricket::TransportChannelState channel_state() { return channel_state_; }

 private:
  std::unique_ptr<rtc::PhysicalSocketServer> pss_;
  std::unique_ptr<rtc::VirtualSocketServer> vss_;
  rtc::SocketServerScope ss_scope_;
  cricket::CandidatePairInterface* last_selected_candidate_pair_ = nullptr;
  int last_sent_packet_id_ = -1;
  bool channel_ready_to_send_ = false;
  cricket::TransportChannelState channel_state_ = cricket::STATE_INIT;
};

TEST_F(P2PTransportChannelPingTest, TestTriggeredChecks) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("trigger checks", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));

  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn1 != nullptr);
  ASSERT_TRUE(conn2 != nullptr);

  // Before a triggered check, the first connection to ping is the
  // highest priority one.
  EXPECT_EQ(conn2, FindNextPingableConnectionAndPingIt(&ch));

  // Receiving a ping causes a triggered check which should make conn1
  // be pinged first instead of conn2, even though conn2 has a higher
  // priority.
  conn1->ReceivedPing();
  EXPECT_EQ(conn1, FindNextPingableConnectionAndPingIt(&ch));
}

TEST_F(P2PTransportChannelPingTest, TestAllConnectionsPingedSufficiently) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("ping sufficiently", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));

  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn1 != nullptr);
  ASSERT_TRUE(conn2 != nullptr);

  // Low-priority connection becomes writable so that the other connection
  // is not pruned.
  conn1->ReceivedPingResponse();
  EXPECT_TRUE_WAIT(
      conn1->num_pings_sent() >= MIN_PINGS_AT_WEAK_PING_INTERVAL &&
          conn2->num_pings_sent() >= MIN_PINGS_AT_WEAK_PING_INTERVAL,
      kDefaultTimeout);
}

TEST_F(P2PTransportChannelPingTest, TestNoTriggeredChecksWhenWritable) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("trigger checks", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));

  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn1 != nullptr);
  ASSERT_TRUE(conn2 != nullptr);

  EXPECT_EQ(conn2, FindNextPingableConnectionAndPingIt(&ch));
  EXPECT_EQ(conn1, FindNextPingableConnectionAndPingIt(&ch));
  conn1->ReceivedPingResponse();
  ASSERT_TRUE(conn1->writable());
  conn1->ReceivedPing();

  // Ping received, but the connection is already writable, so no
  // "triggered check" and conn2 is pinged before conn1 because it has
  // a higher priority.
  EXPECT_EQ(conn2, FindNextPingableConnectionAndPingIt(&ch));
}

TEST_F(P2PTransportChannelPingTest, TestSignalStateChanged) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("state change", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  // Pruning the connection reduces the set of active connections and changes
  // the channel state.
  conn1->Prune();
  EXPECT_EQ_WAIT(cricket::STATE_FAILED, channel_state(), kDefaultTimeout);
}

// Test adding remote candidates with different ufrags. If a remote candidate
// is added with an old ufrag, it will be discarded. If it is added with a
// ufrag that was not seen before, it will be used to create connections
// although the ICE pwd in the remote candidate will be set when the ICE
// credentials arrive. If a remote candidate is added with the current ICE
// ufrag, its pwd and generation will be set properly.
TEST_F(P2PTransportChannelPingTest, TestAddRemoteCandidateWithVariousUfrags) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("add candidate", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  // Add a candidate with a future ufrag.
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1, kIceUfrag[2]));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  const cricket::Candidate& candidate = conn1->remote_candidate();
  EXPECT_EQ(kIceUfrag[2], candidate.username());
  EXPECT_TRUE(candidate.password().empty());
  EXPECT_TRUE(FindNextPingableConnectionAndPingIt(&ch) == nullptr);

  // Set the remote credentials with the "future" ufrag.
  // This should set the ICE pwd in the remote candidate of |conn1|, making
  // it pingable.
  ch.SetRemoteIceCredentials(kIceUfrag[2], kIcePwd[2]);
  EXPECT_EQ(kIceUfrag[2], candidate.username());
  EXPECT_EQ(kIcePwd[2], candidate.password());
  EXPECT_EQ(conn1, FindNextPingableConnectionAndPingIt(&ch));

  // Add a candidate with an old ufrag. No connection will be created.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2, kIceUfrag[1]));
  rtc::Thread::Current()->ProcessMessages(500);
  EXPECT_TRUE(GetConnectionTo(&ch, "2.2.2.2", 2) == nullptr);

  // Add a candidate with the current ufrag, its pwd and generation will be
  // assigned, even if the generation is not set.
  ch.AddRemoteCandidate(CreateHostCandidate("3.3.3.3", 3, 0, kIceUfrag[2]));
  cricket::Connection* conn3 = nullptr;
  ASSERT_TRUE_WAIT((conn3 = GetConnectionTo(&ch, "3.3.3.3", 3)) != nullptr,
                   3000);
  const cricket::Candidate& new_candidate = conn3->remote_candidate();
  EXPECT_EQ(kIcePwd[2], new_candidate.password());
  EXPECT_EQ(1U, new_candidate.generation());

  // Check that the pwd of all remote candidates are properly assigned.
  for (const cricket::RemoteCandidate& candidate : ch.remote_candidates()) {
    EXPECT_TRUE(candidate.username() == kIceUfrag[1] ||
                candidate.username() == kIceUfrag[2]);
    if (candidate.username() == kIceUfrag[1]) {
      EXPECT_EQ(kIcePwd[1], candidate.password());
    } else if (candidate.username() == kIceUfrag[2]) {
      EXPECT_EQ(kIcePwd[2], candidate.password());
    }
  }
}

TEST_F(P2PTransportChannelPingTest, ConnectionResurrection) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("connection resurrection", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();

  // Create conn1 and keep track of original candidate priority.
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  uint32_t remote_priority = conn1->remote_candidate().priority();

  // Create a higher priority candidate and make the connection
  // receiving/writable. This will prune conn1.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  conn2->ReceivedPing();
  conn2->ReceivedPingResponse();

  // Wait for conn1 to be pruned.
  EXPECT_TRUE_WAIT(conn1->pruned(), 3000);
  // Destroy the connection to test SignalUnknownAddress.
  conn1->Destroy();
  EXPECT_TRUE_WAIT(GetConnectionTo(&ch, "1.1.1.1", 1) == nullptr, 1000);

  // Create a minimal STUN message with prflx priority.
  cricket::IceMessage request;
  request.SetType(cricket::STUN_BINDING_REQUEST);
  request.AddAttribute(new cricket::StunByteStringAttribute(
      cricket::STUN_ATTR_USERNAME, kIceUfrag[1]));
  uint32_t prflx_priority = cricket::ICE_TYPE_PREFERENCE_PRFLX << 24;
  request.AddAttribute(new cricket::StunUInt32Attribute(
      cricket::STUN_ATTR_PRIORITY, prflx_priority));
  EXPECT_NE(prflx_priority, remote_priority);

  cricket::Port* port = GetPort(&ch);
  // conn1 should be resurrected with original priority.
  port->SignalUnknownAddress(port, rtc::SocketAddress("1.1.1.1", 1),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(conn1->remote_candidate().priority(), remote_priority);

  // conn3, a real prflx connection, should have prflx priority.
  port->SignalUnknownAddress(port, rtc::SocketAddress("3.3.3.3", 1),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 1);
  ASSERT_TRUE(conn3 != nullptr);
  EXPECT_EQ(conn3->remote_candidate().priority(), prflx_priority);
}

TEST_F(P2PTransportChannelPingTest, TestReceivingStateChange) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("receiving state change", 1, &pa);
  PrepareChannel(&ch);
  // Default receiving timeout and checking receiving interval should not be too
  // small.
  EXPECT_LE(1000, ch.receiving_timeout());
  EXPECT_LE(200, ch.check_receiving_interval());
  ch.SetIceConfig(CreateIceConfig(500, false));
  EXPECT_EQ(500, ch.receiving_timeout());
  EXPECT_EQ(50, ch.check_receiving_interval());
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);

  conn1->ReceivedPing();
  conn1->OnReadPacket("ABC", 3, rtc::CreatePacketTime(0));
  EXPECT_TRUE_WAIT(ch.best_connection() != nullptr, 1000);
  EXPECT_TRUE_WAIT(ch.receiving(), 1000);
  EXPECT_TRUE_WAIT(!ch.receiving(), 1000);
}

// The controlled side will select a connection as the "best connection" based
// on priority until the controlling side nominates a connection, at which
// point the controlled side will select that connection as the
// "best connection". Plus, SignalSelectedCandidatePair will be fired if the
// best connection changes and SignalReadyToSend will be fired if the new best
// connection is writable.
TEST_F(P2PTransportChannelPingTest, TestSelectConnectionBeforeNomination) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("receiving state change", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceRole(cricket::ICEROLE_CONTROLLED);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(conn1, ch.best_connection());
  EXPECT_EQ(conn1, last_selected_candidate_pair());
  EXPECT_EQ(-1, last_sent_packet_id());
  // Channel is not ready to send because it is not writable.
  EXPECT_FALSE(channel_ready_to_send());

  int last_packet_id = 0;
  const char* data = "ABCDEFGH";
  int len = static_cast<int>(strlen(data));
  SendData(ch, data, len, ++last_packet_id);
  // When a higher priority candidate comes in, the new connection is chosen
  // as the best connection.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 10));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  EXPECT_EQ(conn2, ch.best_connection());
  EXPECT_EQ(conn2, last_selected_candidate_pair());
  EXPECT_EQ(last_packet_id, last_sent_packet_id());
  EXPECT_FALSE(channel_ready_to_send());

  // If a stun request with use-candidate attribute arrives, the receiving
  // connection will be set as the best connection, even though
  // its priority is lower.
  SendData(ch, data, len, ++last_packet_id);
  ch.AddRemoteCandidate(CreateHostCandidate("3.3.3.3", 3, 1));
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 3);
  ASSERT_TRUE(conn3 != nullptr);
  // Because it has a lower priority, the best connection is still conn2.
  EXPECT_EQ(conn2, ch.best_connection());
  conn3->ReceivedPingResponse();  // Become writable.
  // But if it is nominated via use_candidate, it is chosen as the best
  // connection.
  conn3->set_nominated(true);
  conn3->SignalNominated(conn3);
  EXPECT_EQ(conn3, ch.best_connection());
  EXPECT_EQ(conn3, last_selected_candidate_pair());
  EXPECT_EQ(last_packet_id, last_sent_packet_id());
  EXPECT_TRUE(channel_ready_to_send());

  // Even if another higher priority candidate arrives,
  // it will not be set as the best connection because the best connection
  // is nominated by the controlling side.
  SendData(ch, data, len, ++last_packet_id);
  ch.AddRemoteCandidate(CreateHostCandidate("4.4.4.4", 4, 100));
  cricket::Connection* conn4 = WaitForConnectionTo(&ch, "4.4.4.4", 4);
  ASSERT_TRUE(conn4 != nullptr);
  EXPECT_EQ(conn3, ch.best_connection());
  // But if it is nominated via use_candidate and writable, it will be set as
  // the best connection.
  conn4->set_nominated(true);
  conn4->SignalNominated(conn4);
  // Not switched yet because conn4 is not writable.
  EXPECT_EQ(conn3, ch.best_connection());
  reset_channel_ready_to_send();
  // The best connection switches after conn4 becomes writable.
  conn4->ReceivedPingResponse();
  EXPECT_EQ(conn4, ch.best_connection());
  EXPECT_EQ(conn4, last_selected_candidate_pair());
  EXPECT_EQ(last_packet_id, last_sent_packet_id());
  // SignalReadyToSend is fired again because conn4 is writable.
  EXPECT_TRUE(channel_ready_to_send());
}

// The controlled side will select a connection as the "best connection" based
// on requests from an unknown address before the controlling side nominates
// a connection, and will nominate a connection from an unknown address if the
// request contains the use_candidate attribute. Plus, it will also sends back
// a ping response and set the ICE pwd in the remote candidate appropriately.
TEST_F(P2PTransportChannelPingTest, TestSelectConnectionFromUnknownAddress) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("receiving state change", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceRole(cricket::ICEROLE_CONTROLLED);
  ch.Connect();
  ch.MaybeStartGathering();
  // A minimal STUN message with prflx priority.
  cricket::IceMessage request;
  request.SetType(cricket::STUN_BINDING_REQUEST);
  request.AddAttribute(new cricket::StunByteStringAttribute(
      cricket::STUN_ATTR_USERNAME, kIceUfrag[1]));
  uint32_t prflx_priority = cricket::ICE_TYPE_PREFERENCE_PRFLX << 24;
  request.AddAttribute(new cricket::StunUInt32Attribute(
      cricket::STUN_ATTR_PRIORITY, prflx_priority));
  cricket::TestUDPPort* port = static_cast<cricket::TestUDPPort*>(GetPort(&ch));
  port->SignalUnknownAddress(port, rtc::SocketAddress("1.1.1.1", 1),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_TRUE(port->sent_binding_response());
  EXPECT_EQ(conn1, ch.best_connection());
  conn1->ReceivedPingResponse();
  EXPECT_EQ(conn1, ch.best_connection());
  port->set_sent_binding_response(false);

  // Another connection is nominated via use_candidate.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 1));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  // Because it has a lower priority, the best connection is still conn1.
  EXPECT_EQ(conn1, ch.best_connection());
  // When it is nominated via use_candidate and writable, it is chosen as the
  // best connection.
  conn2->ReceivedPingResponse();  // Become writable.
  conn2->set_nominated(true);
  conn2->SignalNominated(conn2);
  EXPECT_EQ(conn2, ch.best_connection());

  // Another request with unknown address, it will not be set as the best
  // connection because the best connection was nominated by the controlling
  // side.
  port->SignalUnknownAddress(port, rtc::SocketAddress("3.3.3.3", 3),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 3);
  ASSERT_TRUE(conn3 != nullptr);
  EXPECT_TRUE(port->sent_binding_response());
  conn3->ReceivedPingResponse();  // Become writable.
  EXPECT_EQ(conn2, ch.best_connection());
  port->set_sent_binding_response(false);

  // However if the request contains use_candidate attribute, it will be
  // selected as the best connection.
  request.AddAttribute(
      new cricket::StunByteStringAttribute(cricket::STUN_ATTR_USE_CANDIDATE));
  port->SignalUnknownAddress(port, rtc::SocketAddress("4.4.4.4", 4),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  cricket::Connection* conn4 = WaitForConnectionTo(&ch, "4.4.4.4", 4);
  ASSERT_TRUE(conn4 != nullptr);
  EXPECT_TRUE(port->sent_binding_response());
  // conn4 is not the best connection yet because it is not writable.
  EXPECT_EQ(conn2, ch.best_connection());
  conn4->ReceivedPingResponse();  // Become writable.
  EXPECT_EQ(conn4, ch.best_connection());

  // Test that the request from an unknown address contains a ufrag from an old
  // generation.
  port->set_sent_binding_response(false);
  ch.SetRemoteIceCredentials(kIceUfrag[2], kIcePwd[2]);
  ch.SetRemoteIceCredentials(kIceUfrag[3], kIcePwd[3]);
  port->SignalUnknownAddress(port, rtc::SocketAddress("5.5.5.5", 5),
                             cricket::PROTO_UDP, &request, kIceUfrag[2], false);
  cricket::Connection* conn5 = WaitForConnectionTo(&ch, "5.5.5.5", 5);
  ASSERT_TRUE(conn5 != nullptr);
  EXPECT_TRUE(port->sent_binding_response());
  EXPECT_EQ(kIcePwd[2], conn5->remote_candidate().password());
}

// The controlled side will select a connection as the "best connection"
// based on media received until the controlling side nominates a connection,
// at which point the controlled side will select that connection as
// the "best connection".
TEST_F(P2PTransportChannelPingTest, TestSelectConnectionBasedOnMediaReceived) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("receiving state change", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceRole(cricket::ICEROLE_CONTROLLED);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 10));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(conn1, ch.best_connection());

  // If a data packet is received on conn2, the best connection should
  // switch to conn2 because the controlled side must mirror the media path
  // chosen by the controlling side.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 1));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  conn2->ReceivedPing();  // Start receiving.
  // Do not switch because it is not writable.
  conn2->OnReadPacket("ABC", 3, rtc::CreatePacketTime(0));
  EXPECT_EQ(conn1, ch.best_connection());

  conn2->ReceivedPingResponse();  // Become writable.
  // Switch because it is writable.
  conn2->OnReadPacket("DEF", 3, rtc::CreatePacketTime(0));
  EXPECT_EQ(conn2, ch.best_connection());

  // Now another STUN message with an unknown address and use_candidate will
  // nominate the best connection.
  cricket::IceMessage request;
  request.SetType(cricket::STUN_BINDING_REQUEST);
  request.AddAttribute(new cricket::StunByteStringAttribute(
      cricket::STUN_ATTR_USERNAME, kIceUfrag[1]));
  uint32_t prflx_priority = cricket::ICE_TYPE_PREFERENCE_PRFLX << 24;
  request.AddAttribute(new cricket::StunUInt32Attribute(
      cricket::STUN_ATTR_PRIORITY, prflx_priority));
  request.AddAttribute(
      new cricket::StunByteStringAttribute(cricket::STUN_ATTR_USE_CANDIDATE));
  cricket::Port* port = GetPort(&ch);
  port->SignalUnknownAddress(port, rtc::SocketAddress("3.3.3.3", 3),
                             cricket::PROTO_UDP, &request, kIceUfrag[1], false);
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 3);
  ASSERT_TRUE(conn3 != nullptr);
  EXPECT_EQ(conn2, ch.best_connection());  // Not writable yet.
  conn3->ReceivedPingResponse();           // Become writable.
  EXPECT_EQ(conn3, ch.best_connection());

  // Now another data packet will not switch the best connection because the
  // best connection was nominated by the controlling side.
  conn2->ReceivedPing();
  conn2->ReceivedPingResponse();
  conn2->OnReadPacket("XYZ", 3, rtc::CreatePacketTime(0));
  EXPECT_EQ(conn3, ch.best_connection());
}

// Test that if a new remote candidate has the same address and port with
// an old one, it will be used to create a new connection.
TEST_F(P2PTransportChannelPingTest, TestAddRemoteCandidateWithAddressReuse) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("candidate reuse", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  const std::string host_address = "1.1.1.1";
  const int port_num = 1;

  // kIceUfrag[1] is the current generation ufrag.
  cricket::Candidate candidate =
      CreateHostCandidate(host_address, port_num, 1, kIceUfrag[1]);
  ch.AddRemoteCandidate(candidate);
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, host_address, port_num);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(0u, conn1->remote_candidate().generation());

  // Simply adding the same candidate again won't create a new connection.
  ch.AddRemoteCandidate(candidate);
  cricket::Connection* conn2 = GetConnectionTo(&ch, host_address, port_num);
  EXPECT_EQ(conn1, conn2);

  // Update the ufrag of the candidate and add it again.
  candidate.set_username(kIceUfrag[2]);
  ch.AddRemoteCandidate(candidate);
  conn2 = GetConnectionTo(&ch, host_address, port_num);
  EXPECT_NE(conn1, conn2);
  EXPECT_EQ(kIceUfrag[2], conn2->remote_candidate().username());
  EXPECT_EQ(1u, conn2->remote_candidate().generation());

  // Verify that a ping with the new ufrag can be received on the new
  // connection.
  EXPECT_EQ(0, conn2->last_ping_received());
  ReceivePingOnConnection(conn2, kIceUfrag[2], 1 /* priority */);
  EXPECT_TRUE(conn2->last_ping_received() > 0);
}

// When the current best connection is strong, lower-priority connections will
// be pruned. Otherwise, lower-priority connections are kept.
TEST_F(P2PTransportChannelPingTest, TestDontPruneWhenWeak) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("test channel", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceRole(cricket::ICEROLE_CONTROLLED);
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(conn1, ch.best_connection());
  conn1->ReceivedPingResponse();  // Becomes writable and receiving

  // When a higher-priority, nominated candidate comes in, the connections with
  // lower-priority are pruned.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 10));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  conn2->ReceivedPingResponse();  // Becomes writable and receiving
  conn2->set_nominated(true);
  conn2->SignalNominated(conn2);
  EXPECT_TRUE_WAIT(conn1->pruned(), 3000);

  ch.SetIceConfig(CreateIceConfig(500, false));
  // Wait until conn2 becomes not receiving.
  EXPECT_TRUE_WAIT(!conn2->receiving(), 3000);

  ch.AddRemoteCandidate(CreateHostCandidate("3.3.3.3", 3, 1));
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 3);
  ASSERT_TRUE(conn3 != nullptr);
  // The best connection should still be conn2. Even through conn3 has lower
  // priority and is not receiving/writable, it is not pruned because the best
  // connection is not receiving.
  WAIT(conn3->pruned(), 1000);
  EXPECT_FALSE(conn3->pruned());
}

// Test that GetState returns the state correctly.
TEST_F(P2PTransportChannelPingTest, TestGetState) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("test channel", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  EXPECT_EQ(cricket::TransportChannelState::STATE_INIT, ch.GetState());
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 100));
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 1));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn1 != nullptr);
  ASSERT_TRUE(conn2 != nullptr);
  // Now there are two connections, so the transport channel is connecting.
  EXPECT_EQ(cricket::TransportChannelState::STATE_CONNECTING, ch.GetState());
  // |conn1| becomes writable and receiving; it then should prune |conn2|.
  conn1->ReceivedPingResponse();
  EXPECT_TRUE_WAIT(conn2->pruned(), 1000);
  EXPECT_EQ(cricket::TransportChannelState::STATE_COMPLETED, ch.GetState());
  conn1->Prune();  // All connections are pruned.
  // Need to wait until the channel state is updated.
  EXPECT_EQ_WAIT(cricket::TransportChannelState::STATE_FAILED, ch.GetState(),
                 1000);
}

// Test that when a low-priority connection is pruned, it is not deleted
// right away, and it can become active and be pruned again.
TEST_F(P2PTransportChannelPingTest, TestConnectionPrunedAgain) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("test channel", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceConfig(CreateIceConfig(1000, false));
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 100));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  EXPECT_EQ(conn1, ch.best_connection());
  conn1->ReceivedPingResponse();  // Becomes writable and receiving

  // Add a low-priority connection |conn2|, which will be pruned, but it will
  // not be deleted right away. Once the current best connection becomes not
  // receiving, |conn2| will start to ping and upon receiving the ping response,
  // it will become the best connection.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 1));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  EXPECT_TRUE_WAIT(!conn2->active(), 1000);
  // |conn2| should not send a ping yet.
  EXPECT_EQ(cricket::Connection::STATE_WAITING, conn2->state());
  EXPECT_EQ(cricket::TransportChannelState::STATE_COMPLETED, ch.GetState());
  // Wait for |conn1| becoming not receiving.
  EXPECT_TRUE_WAIT(!conn1->receiving(), 3000);
  // Make sure conn2 is not deleted.
  conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  EXPECT_EQ_WAIT(cricket::Connection::STATE_INPROGRESS, conn2->state(), 1000);
  conn2->ReceivedPingResponse();
  EXPECT_EQ_WAIT(conn2, ch.best_connection(), 1000);
  EXPECT_EQ(cricket::TransportChannelState::STATE_CONNECTING, ch.GetState());

  // When |conn1| comes back again, |conn2| will be pruned again.
  conn1->ReceivedPingResponse();
  EXPECT_EQ_WAIT(conn1, ch.best_connection(), 1000);
  EXPECT_TRUE_WAIT(!conn2->active(), 1000);
  EXPECT_EQ(cricket::TransportChannelState::STATE_COMPLETED, ch.GetState());
}

// Test that if all connections in a channel has timed out on writing, they
// will all be deleted. We use Prune to simulate write_time_out.
TEST_F(P2PTransportChannelPingTest, TestDeleteConnectionsIfAllWriteTimedout) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("test channel", 1, &pa);
  PrepareChannel(&ch);
  ch.Connect();
  ch.MaybeStartGathering();
  // Have one connection only but later becomes write-time-out.
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 100));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  conn1->ReceivedPing();  // Becomes receiving
  conn1->Prune();
  EXPECT_TRUE_WAIT(ch.connections().empty(), 1000);

  // Have two connections but both become write-time-out later.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 1));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  conn2->ReceivedPing();  // Becomes receiving
  ch.AddRemoteCandidate(CreateHostCandidate("3.3.3.3", 3, 2));
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "3.3.3.3", 3);
  ASSERT_TRUE(conn3 != nullptr);
  conn3->ReceivedPing();  // Becomes receiving
  // Now prune both conn2 and conn3; they will be deleted soon.
  conn2->Prune();
  conn3->Prune();
  EXPECT_TRUE_WAIT(ch.connections().empty(), 1000);
}

// Tests that after a port allocator session is started, it will be stopped
// when a new connection becomes writable and receiving. Also tests that if a
// connection belonging to an old session becomes writable, it won't stop
// the current port allocator session.
TEST_F(P2PTransportChannelPingTest, TestStopPortAllocatorSessions) {
  cricket::FakePortAllocator pa(rtc::Thread::Current(), nullptr);
  cricket::P2PTransportChannel ch("test channel", 1, &pa);
  PrepareChannel(&ch);
  ch.SetIceConfig(CreateIceConfig(2000, false));
  ch.Connect();
  ch.MaybeStartGathering();
  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 100));
  cricket::Connection* conn1 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  ASSERT_TRUE(conn1 != nullptr);
  conn1->ReceivedPingResponse();  // Becomes writable and receiving
  EXPECT_TRUE(!ch.allocator_session()->IsGettingPorts());

  // Start a new session. Even though conn1, which belongs to an older
  // session, becomes unwritable and writable again, it should not stop the
  // current session.
  ch.SetIceCredentials(kIceUfrag[1], kIcePwd[1]);
  ch.MaybeStartGathering();
  conn1->Prune();
  conn1->ReceivedPingResponse();
  EXPECT_TRUE(ch.allocator_session()->IsGettingPorts());

  // But if a new connection created from the new session becomes writable,
  // it will stop the current session.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 100));
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  ASSERT_TRUE(conn2 != nullptr);
  conn2->ReceivedPingResponse();  // Becomes writable and receiving
  EXPECT_TRUE(!ch.allocator_session()->IsGettingPorts());
}

class P2PTransportChannelMostLikelyToWorkFirstTest
    : public P2PTransportChannelPingTest {
 public:
  P2PTransportChannelMostLikelyToWorkFirstTest()
      : turn_server_(rtc::Thread::Current(), kTurnUdpIntAddr, kTurnUdpExtAddr) {
    network_manager_.AddInterface(kPublicAddrs[0]);
    allocator_.reset(new cricket::BasicPortAllocator(
        &network_manager_, ServerAddresses(), rtc::SocketAddress(),
        rtc::SocketAddress(), rtc::SocketAddress()));
    allocator_->set_flags(allocator_->flags() |
                          cricket::PORTALLOCATOR_DISABLE_STUN |
                          cricket::PORTALLOCATOR_DISABLE_TCP);
    cricket::RelayServerConfig config(cricket::RELAY_TURN);
    config.credentials = kRelayCredentials;
    config.ports.push_back(
        cricket::ProtocolAddress(kTurnUdpIntAddr, cricket::PROTO_UDP, false));
    allocator_->AddTurnServer(config);
    allocator_->set_step_delay(kMinimumStepDelay);
  }

  cricket::P2PTransportChannel& StartTransportChannel(
      bool prioritize_most_likely_to_work,
      int max_strong_interval) {
    channel_.reset(
        new cricket::P2PTransportChannel("checks", 1, nullptr, allocator()));
    cricket::IceConfig config = channel_->config();
    config.prioritize_most_likely_candidate_pairs =
        prioritize_most_likely_to_work;
    config.max_strong_interval = max_strong_interval;
    channel_->SetIceConfig(config);
    PrepareChannel(channel_.get());
    channel_->Connect();
    channel_->MaybeStartGathering();
    return *channel_.get();
  }

  cricket::BasicPortAllocator* allocator() { return allocator_.get(); }
  cricket::TestTurnServer* turn_server() { return &turn_server_; }

  // This verifies the next pingable connection has the expected candidates'
  // types and, for relay local candidate, the expected relay protocol and ping
  // it.
  void VerifyNextPingableConnection(
      const std::string& local_candidate_type,
      const std::string& remote_candidate_type,
      const std::string& relay_protocol_type = cricket::UDP_PROTOCOL_NAME) {
    cricket::Connection* conn =
        FindNextPingableConnectionAndPingIt(channel_.get());
    EXPECT_EQ(conn->local_candidate().type(), local_candidate_type);
    if (conn->local_candidate().type() == cricket::RELAY_PORT_TYPE) {
      EXPECT_EQ(conn->local_candidate().relay_protocol(), relay_protocol_type);
    }
    EXPECT_EQ(conn->remote_candidate().type(), remote_candidate_type);
  }

  cricket::Candidate CreateRelayCandidate(const std::string& ip,
                                          int port,
                                          int priority,
                                          const std::string& ufrag = "") {
    cricket::Candidate c = CreateHostCandidate(ip, port, priority, ufrag);
    c.set_type(cricket::RELAY_PORT_TYPE);
    return c;
  }

 private:
  std::unique_ptr<cricket::BasicPortAllocator> allocator_;
  rtc::FakeNetworkManager network_manager_;
  cricket::TestTurnServer turn_server_;
  std::unique_ptr<cricket::P2PTransportChannel> channel_;
};

// Test that Relay/Relay connections will be pinged first when no other
// connections have been pinged yet, unless we need to ping a trigger check or
// we have a best connection.
TEST_F(P2PTransportChannelMostLikelyToWorkFirstTest,
       TestRelayRelayFirstWhenNothingPingedYet) {
  const int max_strong_interval = 100;
  cricket::P2PTransportChannel& ch =
      StartTransportChannel(true, max_strong_interval);
  EXPECT_TRUE_WAIT(ch.ports().size() == 2, 5000);
  EXPECT_EQ(ch.ports()[0]->Type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(ch.ports()[1]->Type(), cricket::RELAY_PORT_TYPE);

  ch.AddRemoteCandidate(CreateRelayCandidate("1.1.1.1", 1, 1));
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));

  EXPECT_TRUE_WAIT(ch.connections().size() == 4, 5000);

  // Relay/Relay should be the first pingable connection.
  cricket::Connection* conn = FindNextPingableConnectionAndPingIt(&ch);
  EXPECT_EQ(conn->local_candidate().type(), cricket::RELAY_PORT_TYPE);
  EXPECT_EQ(conn->remote_candidate().type(), cricket::RELAY_PORT_TYPE);

  // Unless that we have a trigger check waiting to be pinged.
  cricket::Connection* conn2 = WaitForConnectionTo(&ch, "2.2.2.2", 2);
  EXPECT_EQ(conn2->local_candidate().type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(conn2->remote_candidate().type(), cricket::LOCAL_PORT_TYPE);
  conn2->ReceivedPing();
  EXPECT_EQ(conn2, FindNextPingableConnectionAndPingIt(&ch));

  // Make conn3 the best connection.
  cricket::Connection* conn3 = WaitForConnectionTo(&ch, "1.1.1.1", 1);
  EXPECT_EQ(conn3->local_candidate().type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(conn3->remote_candidate().type(), cricket::RELAY_PORT_TYPE);
  conn3->ReceivedPingResponse();
  ASSERT_TRUE(conn3->writable());
  conn3->ReceivedPing();

  /*

  TODO(honghaiz): Re-enable this once we use fake clock for this test to fix
  the flakiness. The following test becomes flaky because we now ping the
  connections with fast rates until every connection is pinged at least three
  times. The best connection may have been pinged before |max_strong_interval|,
  so it may not be the next connection to be pinged as expected in the test.

  // Verify that conn3 will be the "best connection" since it is readable and
  // writable. After |MAX_CURRENT_STRONG_INTERVAL|, it should be the next
  // pingable connection.
  EXPECT_TRUE_WAIT(conn3 == ch.best_connection(), 5000);
  WAIT(false, max_strong_interval + 100);
  conn3->ReceivedPingResponse();
  ASSERT_TRUE(conn3->writable());
  EXPECT_EQ(conn3, FindNextPingableConnectionAndPingIt(&ch));

  */
}

// Test that Relay/Relay connections will be pinged first when everything has
// been pinged even if the Relay/Relay connection wasn't the first to be pinged
// in the first round.
TEST_F(P2PTransportChannelMostLikelyToWorkFirstTest,
       TestRelayRelayFirstWhenEverythingPinged) {
  cricket::P2PTransportChannel& ch = StartTransportChannel(true, 100);
  EXPECT_TRUE_WAIT(ch.ports().size() == 2, 5000);
  EXPECT_EQ(ch.ports()[0]->Type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(ch.ports()[1]->Type(), cricket::RELAY_PORT_TYPE);

  ch.AddRemoteCandidate(CreateHostCandidate("1.1.1.1", 1, 1));
  EXPECT_TRUE_WAIT(ch.connections().size() == 2, 5000);

  // Initially, only have Local/Local and Local/Relay.
  VerifyNextPingableConnection(cricket::LOCAL_PORT_TYPE,
                               cricket::LOCAL_PORT_TYPE);
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::LOCAL_PORT_TYPE);

  // Remote Relay candidate arrives.
  ch.AddRemoteCandidate(CreateRelayCandidate("2.2.2.2", 2, 2));
  EXPECT_TRUE_WAIT(ch.connections().size() == 4, 5000);

  // Relay/Relay should be the first since it hasn't been pinged before.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);

  // Local/Relay is the final one.
  VerifyNextPingableConnection(cricket::LOCAL_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);

  // Now, every connection has been pinged once. The next one should be
  // Relay/Relay.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);
}

// Test that when we receive a new remote candidate, they will be tried first
// before we re-ping Relay/Relay connections again.
TEST_F(P2PTransportChannelMostLikelyToWorkFirstTest,
       TestNoStarvationOnNonRelayConnection) {
  cricket::P2PTransportChannel& ch = StartTransportChannel(true, 100);
  EXPECT_TRUE_WAIT(ch.ports().size() == 2, 5000);
  EXPECT_EQ(ch.ports()[0]->Type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(ch.ports()[1]->Type(), cricket::RELAY_PORT_TYPE);

  ch.AddRemoteCandidate(CreateRelayCandidate("1.1.1.1", 1, 1));
  EXPECT_TRUE_WAIT(ch.connections().size() == 2, 5000);

  // Initially, only have Relay/Relay and Local/Relay. Ping Relay/Relay first.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);

  // Next, ping Local/Relay.
  VerifyNextPingableConnection(cricket::LOCAL_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);

  // Remote Local candidate arrives.
  ch.AddRemoteCandidate(CreateHostCandidate("2.2.2.2", 2, 2));
  EXPECT_TRUE_WAIT(ch.connections().size() == 4, 5000);

  // Local/Local should be the first since it hasn't been pinged before.
  VerifyNextPingableConnection(cricket::LOCAL_PORT_TYPE,
                               cricket::LOCAL_PORT_TYPE);

  // Relay/Local is the final one.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::LOCAL_PORT_TYPE);

  // Now, every connection has been pinged once. The next one should be
  // Relay/Relay.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);
}

// Test the ping sequence is UDP Relay/Relay followed by TCP Relay/Relay,
// followed by the rest.
TEST_F(P2PTransportChannelMostLikelyToWorkFirstTest, TestTcpTurn) {
  // Add a Tcp Turn server.
  turn_server()->AddInternalSocket(kTurnTcpIntAddr, cricket::PROTO_TCP);
  cricket::RelayServerConfig config(cricket::RELAY_TURN);
  config.credentials = kRelayCredentials;
  config.ports.push_back(
      cricket::ProtocolAddress(kTurnTcpIntAddr, cricket::PROTO_TCP, false));
  allocator()->AddTurnServer(config);

  cricket::P2PTransportChannel& ch = StartTransportChannel(true, 100);
  EXPECT_TRUE_WAIT(ch.ports().size() == 3, 5000);
  EXPECT_EQ(ch.ports()[0]->Type(), cricket::LOCAL_PORT_TYPE);
  EXPECT_EQ(ch.ports()[1]->Type(), cricket::RELAY_PORT_TYPE);
  EXPECT_EQ(ch.ports()[2]->Type(), cricket::RELAY_PORT_TYPE);

  // Remote Relay candidate arrives.
  ch.AddRemoteCandidate(CreateRelayCandidate("1.1.1.1", 1, 1));
  EXPECT_TRUE_WAIT(ch.connections().size() == 3, 5000);

  // UDP Relay/Relay should be pinged first.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);

  // TCP Relay/Relay is the next.
  VerifyNextPingableConnection(cricket::RELAY_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE,
                               cricket::TCP_PROTOCOL_NAME);

  // Finally, Local/Relay will be pinged.
  VerifyNextPingableConnection(cricket::LOCAL_PORT_TYPE,
                               cricket::RELAY_PORT_TYPE);
}
