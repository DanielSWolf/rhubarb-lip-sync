/*
 *  Copyright 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(HAVE_SRTP) && defined(ENABLE_EXTERNAL_AUTH)

#include "webrtc/pc/externalhmac.h"

#include <stdlib.h>  // For malloc/free.

extern "C" {
#ifdef SRTP_RELATIVE_PATH
#include "crypto_kernel.h"  // NOLINT
#include "srtp.h"  // NOLINT
#else
#include "third_party/libsrtp/srtp/crypto/include/crypto_kernel.h"
#include "third_party/libsrtp/srtp/include/srtp.h"
#endif  // SRTP_RELATIVE_PATH
}

#include "webrtc/base/logging.h"

// Begin test case 0 */
static const uint8_t kExternalHmacTestCase0Key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b
};

static const uint8_t kExternalHmacTestCase0Data[8] = {
  0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65   // "Hi There"
};

static const uint8_t kExternalHmacFakeTag[10] = {
  0xba, 0xdd, 0xba, 0xdd, 0xba, 0xdd, 0xba, 0xdd, 0xba, 0xdd
};

static const auth_test_case_t kExternalHmacTestCase0 = {
  20,                                                    // Octets in key
  const_cast<uint8_t*>(kExternalHmacTestCase0Key),   // Key
  8,                                                     // Octets in data
  const_cast<uint8_t*>(kExternalHmacTestCase0Data),  // Data
  10,                                                    // Octets in tag
  const_cast<uint8_t*>(kExternalHmacFakeTag),          // Tag
  NULL                                                   // Pointer to next
                                                         // testcase
};

static const char kExternalHmacDescription[] =
    "external hmac sha-1 authentication";

// auth_type_t external_hmac is the hmac metaobject

static const auth_type_t external_hmac  = {
  external_hmac_alloc,
  external_hmac_dealloc,
  (auth_init_func)    external_hmac_init,
  (auth_compute_func) external_hmac_compute,
  (auth_update_func)  external_hmac_update,
  (auth_start_func)   external_hmac_start,
  const_cast<char*>(kExternalHmacDescription),
  0,     // Instance count.
  const_cast<auth_test_case_t*>(&kExternalHmacTestCase0),
  NULL,  // No debugging module.
  EXTERNAL_HMAC_SHA1
};


err_status_t external_hmac_alloc(auth_t** a, int key_len, int out_len) {
  uint8_t* pointer;

  // Check key length - note that we don't support keys larger
  // than 20 bytes yet
  if (key_len > 20)
    return err_status_bad_param;

  // Check output length - should be less than 20 bytes/
  if (out_len > 20)
    return err_status_bad_param;

  // Allocate memory for auth and hmac_ctx_t structures.
  pointer = new uint8_t[(sizeof(ExternalHmacContext) + sizeof(auth_t))];
  if (pointer == NULL)
    return err_status_alloc_fail;

  // Set pointers
  *a = (auth_t *)pointer;
  // |external_hmac| is const and libsrtp expects |type| to be non-const.
  // const conversion is required. |external_hmac| is constant because we don't
  // want to increase global count in Chrome.
  (*a)->type = const_cast<auth_type_t*>(&external_hmac);
  (*a)->state = pointer + sizeof(auth_t);
  (*a)->out_len = out_len;
  (*a)->key_len = key_len;
  (*a)->prefix_len = 0;

  return err_status_ok;
}

err_status_t external_hmac_dealloc(auth_t* a) {
  // Zeroize entire state
  memset((uint8_t *)a, 0, sizeof(ExternalHmacContext) + sizeof(auth_t));

  // Free memory
  delete[] a;

  return err_status_ok;
}

err_status_t external_hmac_init(ExternalHmacContext* state,
                                const uint8_t* key,
                                int key_len) {
  if (key_len > HMAC_KEY_LENGTH)
    return err_status_bad_param;

  memset(state->key, 0, key_len);
  memcpy(state->key, key, key_len);
  state->key_length = key_len;
  return err_status_ok;
}

err_status_t external_hmac_start(ExternalHmacContext* state) {
  return err_status_ok;
}

err_status_t external_hmac_update(ExternalHmacContext* state,
                                  const uint8_t* message,
                                  int msg_octets) {
  return err_status_ok;
}

err_status_t external_hmac_compute(ExternalHmacContext* state,
                                   const void* message,
                                   int msg_octets,
                                   int tag_len,
                                   uint8_t* result) {
  memcpy(result, kExternalHmacFakeTag, tag_len);
  return err_status_ok;
}

err_status_t external_crypto_init() {
  // |external_hmac| is const. const_cast is required as libsrtp expects
  // non-const.
  err_status_t status = crypto_kernel_replace_auth_type(
      const_cast<auth_type_t*>(&external_hmac), EXTERNAL_HMAC_SHA1);
  if (status) {
    LOG(LS_ERROR) << "Error in replacing default auth module, error: "
                  << status;
    return err_status_fail;
  }
  return err_status_ok;
}

#endif  // defined(HAVE_SRTP) && defined(ENABLE_EXTERNAL_AUTH)
