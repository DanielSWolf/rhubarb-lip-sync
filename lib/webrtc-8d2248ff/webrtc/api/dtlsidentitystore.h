/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_DTLSIDENTITYSTORE_H_
#define WEBRTC_API_DTLSIDENTITYSTORE_H_

#include <memory>
#include <queue>
#include <string>
#include <utility>

#include "webrtc/base/messagehandler.h"
#include "webrtc/base/messagequeue.h"
#include "webrtc/base/optional.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/rtccertificategenerator.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/sslidentity.h"
#include "webrtc/base/thread.h"

namespace webrtc {

class SSLIdentity;
class Thread;

// Used to receive callbacks of DTLS identity requests.
class DtlsIdentityRequestObserver : public rtc::RefCountInterface {
 public:
  virtual void OnFailure(int error) = 0;
  // TODO(hbos): Unify the OnSuccess method once Chrome code is updated.
  virtual void OnSuccess(const std::string& der_cert,
                         const std::string& der_private_key) = 0;
  // |identity| is a unique_ptr because rtc::SSLIdentity is not copyable and the
  // client has to get the ownership of the object to make use of it.
  virtual void OnSuccess(std::unique_ptr<rtc::SSLIdentity> identity) = 0;

 protected:
  virtual ~DtlsIdentityRequestObserver() {}
};

// This interface defines an in-memory DTLS identity store, which generates DTLS
// identities.
// APIs calls must be made on the signaling thread and the callbacks are also
// called on the signaling thread.
class DtlsIdentityStoreInterface {
 public:
  virtual ~DtlsIdentityStoreInterface() { }

  // The |observer| will be called when the requested identity is ready, or when
  // identity generation fails.
  virtual void RequestIdentity(
      const rtc::KeyParams& key_params,
      const rtc::Optional<uint64_t>& expires_ms,
      const rtc::scoped_refptr<DtlsIdentityRequestObserver>& observer) = 0;
};

}  // namespace webrtc

#endif  // WEBRTC_API_DTLSIDENTITYSTORE_H_
