/**
  ******************************************************************************
  * @file    handshake.cpp
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   HANDSHAKE
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */
#include "handshake.h"
#include "protocol_selector.h"

#if HAL_PLATFORM_CLOUD_TCP

int ciphertext_from_nonce_and_id(const unsigned char *nonce,
                                 const unsigned char *id,
                                 const unsigned char *pubkey,
                                 unsigned char *ciphertext)
{
  unsigned char plaintext[52];

  memcpy(plaintext, nonce, 40);
  memcpy(plaintext + 40, id, 12);

  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, pubkey);

  int ret = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, 52, plaintext, ciphertext);
  rsa_free(&rsa);
  return ret;
}

int decipher_aes_credentials(const unsigned char *private_key,
                             const unsigned char *ciphertext,
                             unsigned char *aes_credentials)
{
  rsa_context rsa;
  init_rsa_context_with_private_key(&rsa, private_key);

  int len = 128;
  int ret = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &len, ciphertext,
                              aes_credentials, 40);
  rsa_free(&rsa);
  return ret;
}

void calculate_ciphertext_hmac(const unsigned char *ciphertext,
                               const unsigned char *hmac_key,
                               unsigned char *hmac)
{
  sha1_hmac(hmac_key, 40, ciphertext, 128, hmac);
}

int verify_signature(const unsigned char *signature,
                     const unsigned char *pubkey,
                     const unsigned char *expected_hmac)
{
  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, pubkey);

  int ret = rsa_pkcs1_verify(&rsa, RSA_PUBLIC, RSA_RAW, 20,
                             expected_hmac, signature);
  rsa_free(&rsa);
  return ret;
}

void init_rsa_context_with_public_key(rsa_context *rsa,
                                      const unsigned char *pubkey)
{
  rsa_init(rsa, RSA_PKCS_V15, RSA_RAW, NULL, NULL);

  rsa->len = 256;
  mpi_read_binary(&rsa->N, pubkey + 33, 256);
  mpi_read_string(&rsa->E, 16, "10001");
}

/* Very simple ASN.1 parsing.
 * Mainly needed because, even though all the RSA big integers
 * are always a specific number of bytes, the key generation
 * and encoding process sometimes pads each number with a
 * leading zero byte.
 */
void init_rsa_context_with_private_key(rsa_context *rsa,
                                       const unsigned char *private_key)
{
  rsa_init(rsa, RSA_PKCS_V15, RSA_RAW, NULL, NULL);

  rsa->len = 128;

  int i = 9;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->N, private_key + i, 128);
  mpi_read_string(&rsa->E, 16, "10001");

  i = i + 135;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->D, private_key + i, 128);

  i = i + 129;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->P, private_key + i, 64);

  i = i + 65;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->Q, private_key + i, 64);

  i = i + 65;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->DP, private_key + i, 64);

  i = i + 65;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->DQ, private_key + i, 64);

  i = i + 65;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->QP, private_key + i, 64);
}


/* Sample Usage:
 * uint8_t device_public_key[162];
 * uint8_t device_private_key[612];
 * HAL_FLASH_Read_CorePrivateKey(device_private_key);
 * parse_device_pubkey_from_privkey(device_public_key, device_private_key);
 */
void extract_public_rsa_key(uint8_t* device_pubkey, const uint8_t* device_privkey)
{
    uint8_t device_pubkey_header[29] = {
            0x30, 0x81, 0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a,
            0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01,
            0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81,
            0x89, 0x02, 0x81, 0x81, 0x00
    };

    uint8_t device_pubkey_exponent[5] = {0x02, 0x03, 0x01, 0x00, 0x01};

    memcpy(device_pubkey, device_pubkey_header, 29);
    memcpy(device_pubkey + 29, device_privkey + 11, 128);
    memcpy(device_pubkey + 157, device_pubkey_exponent, 5);
}


#endif
