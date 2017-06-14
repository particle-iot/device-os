/**
  ******************************************************************************
  * @file    handshake.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   HANDSHAKE
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */
#include <string.h>
#include <stdint.h>

#include "protocol_selector.h"

#ifdef USE_MBEDTLS
#include "mbedtls/rsa.h"
#define rsa_context mbedtls_rsa_context
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
#  include "wiced_security.h"
#  include "crypto_open/bignum.h"
# else
#  include "tropicssl/rsa.h"
#  include "tropicssl/sha1.h"
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ciphertext_from_nonce_and_id(const unsigned char *nonce,
                                 const unsigned char *id,
                                 const unsigned char *pubkey,
                                 unsigned char *ciphertext);

int decipher_aes_credentials(const unsigned char *private_key,
                             const unsigned char *ciphertext,
                             unsigned char *aes_credentials);

void calculate_ciphertext_hmac(const unsigned char *ciphertext,
                               const unsigned char *hmac_key,
                               unsigned char *hmac);

int verify_signature(const unsigned char *signature,
                     const unsigned char *pubkey,
                     const unsigned char *expected_hmac);

void init_rsa_context_with_public_key(rsa_context *rsa,
                                      const unsigned char *pubkey);

void init_rsa_context_with_private_key(rsa_context *rsa,
                                       const unsigned char *private_key);


void extract_public_rsa_key(uint8_t* device_pubkey, const uint8_t* device_privkey);

#ifdef USE_MBEDTLS
#undef rsa_context
#endif

#ifdef __cplusplus
}
#endif

