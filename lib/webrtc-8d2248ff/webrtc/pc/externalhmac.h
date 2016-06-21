/*
 *  Copyright 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_PC_EXTERNALHMAC_H_
#define WEBRTC_PC_EXTERNALHMAC_H_

// External libsrtp HMAC auth module which implements methods defined in
// auth_type_t.
// The default auth module will be replaced only when the ENABLE_EXTERNAL_AUTH
// flag is enabled. This allows us to access to authentication keys,
// as the default auth implementation doesn't provide access and avoids
// hashing each packet twice.

// How will libsrtp select this module?
// Libsrtp defines authentication function types identified by an unsigned
// integer, e.g. HMAC_SHA1 is 3. Using authentication ids, the application
// can plug any desired authentication modules into libsrtp.
// libsrtp also provides a mechanism to select different auth functions for
// individual streams. This can be done by setting the right value in
// the auth_type of srtp_policy_t. The application must first register auth
// functions and the corresponding authentication id using
// crypto_kernel_replace_auth_type function.
#if defined(HAVE_SRTP) && defined(ENABLE_EXTERNAL_AUTH)

#include "webrtc/base/basictypes.h"
extern "C" {
#ifdef SRTP_RELATIVE_PATH
#include "auth.h"  // NOLINT
#else
#include "third_party/libsrtp/srtp/crypto/include/auth.h"
#endif  // SRTP_RELATIVE_PATH
}

#define EXTERNAL_HMAC_SHA1 HMAC_SHA1 + 1
#define HMAC_KEY_LENGTH 20

// The HMAC context structure used to store authentication keys.
// The pointer to the key  will be allocated in the external_hmac_init function.
// This pointer is owned by srtp_t in a template context.
typedef struct {
  uint8_t key[HMAC_KEY_LENGTH];
  int key_length;
} ExternalHmacContext;

err_status_t external_hmac_alloc(auth_t** a, int key_len, int out_len);

err_status_t external_hmac_dealloc(auth_t* a);

err_status_t external_hmac_init(ExternalHmacContext* state,
                                const uint8_t* key,
                                int key_len);

err_status_t external_hmac_start(ExternalHmacContext* state);

err_status_t external_hmac_update(ExternalHmacContext* state,
                                  const uint8_t* message,
                                  int msg_octets);

err_status_t external_hmac_compute(ExternalHmacContext* state,
                                   const void* message,
                                   int msg_octets,
                                   int tag_len,
                                   uint8_t* result);

err_status_t external_crypto_init();

#endif  // defined(HAVE_SRTP) && defined(ENABLE_EXTERNAL_AUTH)
#endif  // WEBRTC_PC_EXTERNALHMAC_H_
