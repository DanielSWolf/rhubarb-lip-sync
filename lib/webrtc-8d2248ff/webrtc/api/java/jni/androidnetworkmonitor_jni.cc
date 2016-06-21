/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/java/jni/androidnetworkmonitor_jni.h"

#include <dlfcn.h>

#include "webrtc/api/java/jni/classreferenceholder.h"
#include "webrtc/api/java/jni/jni_helpers.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/common.h"
#include "webrtc/base/ipaddress.h"

namespace webrtc_jni {

jobject AndroidNetworkMonitor::application_context_ = nullptr;

static NetworkType GetNetworkTypeFromJava(JNIEnv* jni, jobject j_network_type) {
  std::string enum_name =
      GetJavaEnumName(jni, "org/webrtc/NetworkMonitorAutoDetect$ConnectionType",
                      j_network_type);
  if (enum_name == "CONNECTION_UNKNOWN") {
    return NetworkType::NETWORK_UNKNOWN;
  }
  if (enum_name == "CONNECTION_ETHERNET") {
    return NetworkType::NETWORK_ETHERNET;
  }
  if (enum_name == "CONNECTION_WIFI") {
    return NetworkType::NETWORK_WIFI;
  }
  if (enum_name == "CONNECTION_4G") {
    return NetworkType::NETWORK_4G;
  }
  if (enum_name == "CONNECTION_3G") {
    return NetworkType::NETWORK_3G;
  }
  if (enum_name == "CONNECTION_2G") {
    return NetworkType::NETWORK_2G;
  }
  if (enum_name == "CONNECTION_BLUETOOTH") {
    return NetworkType::NETWORK_BLUETOOTH;
  }
  if (enum_name == "CONNECTION_NONE") {
    return NetworkType::NETWORK_NONE;
  }
  ASSERT(false);
  return NetworkType::NETWORK_UNKNOWN;
}

static rtc::AdapterType AdapterTypeFromNetworkType(NetworkType network_type) {
  switch (network_type) {
    case NETWORK_UNKNOWN:
      RTC_DCHECK(false) << "Unknown network type";
      return rtc::ADAPTER_TYPE_UNKNOWN;
    case NETWORK_ETHERNET:
      return rtc::ADAPTER_TYPE_ETHERNET;
    case NETWORK_WIFI:
      return rtc::ADAPTER_TYPE_WIFI;
    case NETWORK_4G:
    case NETWORK_3G:
    case NETWORK_2G:
      return rtc::ADAPTER_TYPE_CELLULAR;
    case NETWORK_BLUETOOTH:
      // There is no corresponding mapping for bluetooth networks.
      // Map it to VPN for now.
      return rtc::ADAPTER_TYPE_VPN;
    default:
      RTC_DCHECK(false) << "Invalid network type " << network_type;
      return rtc::ADAPTER_TYPE_UNKNOWN;
  }
}

static rtc::IPAddress GetIPAddressFromJava(JNIEnv* jni, jobject j_ip_address) {
  jclass j_ip_address_class = GetObjectClass(jni, j_ip_address);
  jfieldID j_address_id = GetFieldID(jni, j_ip_address_class, "address", "[B");
  jbyteArray j_addresses =
      static_cast<jbyteArray>(GetObjectField(jni, j_ip_address, j_address_id));
  size_t address_length = jni->GetArrayLength(j_addresses);
  jbyte* addr_array = jni->GetByteArrayElements(j_addresses, nullptr);
  CHECK_EXCEPTION(jni) << "Error during GetIPAddressFromJava";
  if (address_length == 4) {
    // IP4
    struct in_addr ip4_addr;
    memcpy(&ip4_addr.s_addr, addr_array, 4);
    jni->ReleaseByteArrayElements(j_addresses, addr_array, JNI_ABORT);
    return rtc::IPAddress(ip4_addr);
  }
  // IP6
  RTC_CHECK(address_length == 16);
  struct in6_addr ip6_addr;
  memcpy(ip6_addr.s6_addr, addr_array, address_length);
  jni->ReleaseByteArrayElements(j_addresses, addr_array, JNI_ABORT);
  return rtc::IPAddress(ip6_addr);
}

static void GetIPAddressesFromJava(JNIEnv* jni,
                                   jobjectArray j_ip_addresses,
                                   std::vector<rtc::IPAddress>* ip_addresses) {
  ip_addresses->clear();
  size_t num_addresses = jni->GetArrayLength(j_ip_addresses);
  CHECK_EXCEPTION(jni) << "Error during GetArrayLength";
  for (size_t i = 0; i < num_addresses; ++i) {
    jobject j_ip_address = jni->GetObjectArrayElement(j_ip_addresses, i);
    CHECK_EXCEPTION(jni) << "Error during GetObjectArrayElement";
    rtc::IPAddress ip = GetIPAddressFromJava(jni, j_ip_address);
    ip_addresses->push_back(ip);
  }
}

static NetworkInformation GetNetworkInformationFromJava(
    JNIEnv* jni,
    jobject j_network_info) {
  jclass j_network_info_class = GetObjectClass(jni, j_network_info);
  jfieldID j_interface_name_id =
      GetFieldID(jni, j_network_info_class, "name", "Ljava/lang/String;");
  jfieldID j_handle_id = GetFieldID(jni, j_network_info_class, "handle", "I");
  jfieldID j_type_id =
      GetFieldID(jni, j_network_info_class, "type",
                 "Lorg/webrtc/NetworkMonitorAutoDetect$ConnectionType;");
  jfieldID j_ip_addresses_id =
      GetFieldID(jni, j_network_info_class, "ipAddresses",
                 "[Lorg/webrtc/NetworkMonitorAutoDetect$IPAddress;");

  NetworkInformation network_info;
  network_info.interface_name = JavaToStdString(
      jni, GetStringField(jni, j_network_info, j_interface_name_id));
  network_info.handle =
      static_cast<NetworkHandle>(GetIntField(jni, j_network_info, j_handle_id));
  network_info.type = GetNetworkTypeFromJava(
      jni, GetObjectField(jni, j_network_info, j_type_id));
  jobjectArray j_ip_addresses = static_cast<jobjectArray>(
      GetObjectField(jni, j_network_info, j_ip_addresses_id));
  GetIPAddressesFromJava(jni, j_ip_addresses, &network_info.ip_addresses);
  return network_info;
}

std::string NetworkInformation::ToString() const {
  std::stringstream ss;
  ss << "NetInfo[name " << interface_name << "; handle " << handle << "; type "
     << type << "; address";
  for (const rtc::IPAddress address : ip_addresses) {
    ss << " " << address.ToString();
  }
  ss << "]";
  return ss.str();
}

// static
void AndroidNetworkMonitor::SetAndroidContext(JNIEnv* jni, jobject context) {
  if (application_context_) {
    jni->DeleteGlobalRef(application_context_);
  }
  application_context_ = NewGlobalRef(jni, context);
}

AndroidNetworkMonitor::AndroidNetworkMonitor()
    : j_network_monitor_class_(jni(),
                               FindClass(jni(), "org/webrtc/NetworkMonitor")),
      j_network_monitor_(
          jni(),
          jni()->CallStaticObjectMethod(
              *j_network_monitor_class_,
              GetStaticMethodID(
                  jni(),
                  *j_network_monitor_class_,
                  "init",
                  "(Landroid/content/Context;)Lorg/webrtc/NetworkMonitor;"),
              application_context_)) {
  ASSERT(application_context_ != nullptr);
  CHECK_EXCEPTION(jni()) << "Error during NetworkMonitor.init";
}

void AndroidNetworkMonitor::Start() {
  RTC_CHECK(thread_checker_.CalledOnValidThread());
  if (started_) {
    return;
  }
  started_ = true;

  // This is kind of magic behavior, but doing this allows the SocketServer to
  // use this as a NetworkBinder to bind sockets on a particular network when
  // it creates sockets.
  worker_thread()->socketserver()->set_network_binder(this);

  jmethodID m =
      GetMethodID(jni(), *j_network_monitor_class_, "startMonitoring", "(J)V");
  jni()->CallVoidMethod(*j_network_monitor_, m, jlongFromPointer(this));
  CHECK_EXCEPTION(jni()) << "Error during CallVoidMethod";
}

void AndroidNetworkMonitor::Stop() {
  RTC_CHECK(thread_checker_.CalledOnValidThread());
  if (!started_) {
    return;
  }
  started_ = false;

  // Once the network monitor stops, it will clear all network information and
  // it won't find the network handle to bind anyway.
  if (worker_thread()->socketserver()->network_binder() == this) {
    worker_thread()->socketserver()->set_network_binder(nullptr);
  }

  jmethodID m =
      GetMethodID(jni(), *j_network_monitor_class_, "stopMonitoring", "(J)V");
  jni()->CallVoidMethod(*j_network_monitor_, m, jlongFromPointer(this));
  CHECK_EXCEPTION(jni()) << "Error during NetworkMonitor.stopMonitoring";

  network_handle_by_address_.clear();
  network_info_by_handle_.clear();
}

int AndroidNetworkMonitor::BindSocketToNetwork(int socket_fd,
                                               const rtc::IPAddress& address) {
  RTC_CHECK(thread_checker_.CalledOnValidThread());
  // Android prior to Lollipop didn't have support for binding sockets to
  // networks. However, in that case it should not have reached here because
  // |network_handle_by_address_| should only be populated in Android Lollipop
  // and above.
  // TODO(honghaiz): Add a check for Android version here so that it won't try
  // to look for handle if the Android version is before Lollipop.
  auto iter = network_handle_by_address_.find(address);
  if (iter == network_handle_by_address_.end()) {
    return rtc::NETWORK_BIND_ADDRESS_NOT_FOUND;
  }
  NetworkHandle network_handle = iter->second;

  // NOTE: This does rely on Android implementation details, but
  // these details are unlikely to change.
  typedef int (*SetNetworkForSocket)(unsigned netId, int socketFd);
  static SetNetworkForSocket setNetworkForSocket;
  // This is not threadsafe, but we are running this only on the worker thread.
  if (setNetworkForSocket == nullptr) {
    // Android's netd client library should always be loaded in our address
    // space as it shims libc functions like connect().
    const std::string net_library_path = "libnetd_client.so";
    void* lib = dlopen(net_library_path.c_str(), RTLD_LAZY);
    if (lib == nullptr) {
      LOG(LS_ERROR) << "Library " << net_library_path << " not found!";
      return rtc::NETWORK_BIND_NOT_IMPLEMENTED;
    }
    setNetworkForSocket = reinterpret_cast<SetNetworkForSocket>(
        dlsym(lib, "setNetworkForSocket"));
  }
  if (setNetworkForSocket == nullptr) {
    LOG(LS_ERROR) << "Symbol setNetworkForSocket not found ";
    return rtc::NETWORK_BIND_NOT_IMPLEMENTED;
  }
  int rv = setNetworkForSocket(network_handle, socket_fd);
  // If |network| has since disconnected, |rv| will be ENONET.  Surface this as
  // ERR_NETWORK_CHANGED, rather than MapSystemError(ENONET) which gives back
  // the less descriptive ERR_FAILED.
  if (rv == 0) {
    return rtc::NETWORK_BIND_SUCCESS;
  }
  if (rv == ENONET) {
    return rtc::NETWORK_BIND_NETWORK_CHANGED;
  }
  return rtc::NETWORK_BIND_FAILURE;
}

void AndroidNetworkMonitor::OnNetworkConnected(
    const NetworkInformation& network_info) {
  worker_thread()->Invoke<void>(
      RTC_FROM_HERE, rtc::Bind(&AndroidNetworkMonitor::OnNetworkConnected_w,
                               this, network_info));
  // Fire SignalNetworksChanged to update the list of networks.
  OnNetworksChanged();
}

void AndroidNetworkMonitor::OnNetworkConnected_w(
    const NetworkInformation& network_info) {
  LOG(LS_INFO) << "Network connected: " << network_info.ToString();
  adapter_type_by_name_[network_info.interface_name] =
      AdapterTypeFromNetworkType(network_info.type);
  network_info_by_handle_[network_info.handle] = network_info;
  for (const rtc::IPAddress& address : network_info.ip_addresses) {
    network_handle_by_address_[address] = network_info.handle;
  }
}

void AndroidNetworkMonitor::OnNetworkDisconnected(NetworkHandle handle) {
  LOG(LS_INFO) << "Network disconnected for handle " << handle;
  worker_thread()->Invoke<void>(
      RTC_FROM_HERE,
      rtc::Bind(&AndroidNetworkMonitor::OnNetworkDisconnected_w, this, handle));
}

void AndroidNetworkMonitor::OnNetworkDisconnected_w(NetworkHandle handle) {
  auto iter = network_info_by_handle_.find(handle);
  if (iter != network_info_by_handle_.end()) {
    for (const rtc::IPAddress& address : iter->second.ip_addresses) {
      network_handle_by_address_.erase(address);
    }
    network_info_by_handle_.erase(iter);
  }
}

void AndroidNetworkMonitor::SetNetworkInfos(
    const std::vector<NetworkInformation>& network_infos) {
  RTC_CHECK(thread_checker_.CalledOnValidThread());
  network_handle_by_address_.clear();
  network_info_by_handle_.clear();
  LOG(LS_INFO) << "Android network monitor found " << network_infos.size()
               << " networks";
  for (NetworkInformation network : network_infos) {
    OnNetworkConnected_w(network);
  }
}

rtc::AdapterType AndroidNetworkMonitor::GetAdapterType(
    const std::string& if_name) {
  auto iter = adapter_type_by_name_.find(if_name);
  rtc::AdapterType type = (iter == adapter_type_by_name_.end())
                              ? rtc::ADAPTER_TYPE_UNKNOWN
                              : iter->second;
  if (type == rtc::ADAPTER_TYPE_UNKNOWN) {
    LOG(LS_WARNING) << "Get an unknown type for the interface " << if_name;
  }
  return type;
}

rtc::NetworkMonitorInterface*
AndroidNetworkMonitorFactory::CreateNetworkMonitor() {
  return new AndroidNetworkMonitor();
}

JOW(void, NetworkMonitor_nativeNotifyConnectionTypeChanged)(
    JNIEnv* jni, jobject j_monitor, jlong j_native_monitor) {
  rtc::NetworkMonitorInterface* network_monitor =
      reinterpret_cast<rtc::NetworkMonitorInterface*>(j_native_monitor);
  network_monitor->OnNetworksChanged();
}

JOW(void, NetworkMonitor_nativeNotifyOfActiveNetworkList)(
    JNIEnv* jni, jobject j_monitor, jlong j_native_monitor,
    jobjectArray j_network_infos) {
  AndroidNetworkMonitor* network_monitor =
      reinterpret_cast<AndroidNetworkMonitor*>(j_native_monitor);
  std::vector<NetworkInformation> network_infos;
  size_t num_networks = jni->GetArrayLength(j_network_infos);
  for (size_t i = 0; i < num_networks; ++i) {
    jobject j_network_info = jni->GetObjectArrayElement(j_network_infos, i);
    CHECK_EXCEPTION(jni) << "Error during GetObjectArrayElement";
    network_infos.push_back(GetNetworkInformationFromJava(jni, j_network_info));
  }
  network_monitor->SetNetworkInfos(network_infos);
}

JOW(void, NetworkMonitor_nativeNotifyOfNetworkConnect)(
    JNIEnv* jni, jobject j_monitor, jlong j_native_monitor,
    jobject j_network_info) {
  AndroidNetworkMonitor* network_monitor =
      reinterpret_cast<AndroidNetworkMonitor*>(j_native_monitor);
  NetworkInformation network_info =
      GetNetworkInformationFromJava(jni, j_network_info);
  network_monitor->OnNetworkConnected(network_info);
}

JOW(void, NetworkMonitor_nativeNotifyOfNetworkDisconnect)(
    JNIEnv* jni, jobject j_monitor, jlong j_native_monitor,
    jint network_handle) {
  AndroidNetworkMonitor* network_monitor =
      reinterpret_cast<AndroidNetworkMonitor*>(j_native_monitor);
  network_monitor->OnNetworkDisconnected(
      static_cast<NetworkHandle>(network_handle));
}

}  // namespace webrtc_jni
