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

#include "service_debug.h"

#ifdef USE_MBEDTLS
#include "mbedtls/md.h"
#include "mbedtls_util.h"
#include "mbedtls/pk.h"
#include "mbedtls/asn1.h"
#else
#include "tropicssl/sha1.h"
#endif

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

#ifdef USE_MBEDTLS
  int ret = mbedtls_rsa_pkcs1_encrypt(&rsa, default_rng, nullptr, MBEDTLS_RSA_PUBLIC, 52, plaintext, ciphertext);
#else
  int ret = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, 52, plaintext, ciphertext);
#endif
  rsa_free(&rsa);
  return ret;
}

int decipher_aes_credentials(const unsigned char *private_key,
                             const unsigned char *ciphertext,
                             unsigned char *aes_credentials)
{
  rsa_context rsa;
  int ret = init_rsa_context_with_private_key(&rsa, private_key);
  if (ret)
  {
    return ret;
  }

#ifdef USE_MBEDTLS
  size_t len = 128;
  ret = mbedtls_rsa_pkcs1_decrypt(&rsa, default_rng, nullptr, MBEDTLS_RSA_PRIVATE, &len, ciphertext,
                              aes_credentials, 40);
#else
  int len = 128;
  ret = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &len, ciphertext,
                              aes_credentials, 40);
#endif
  rsa_free(&rsa);
  return ret;
}

void calculate_ciphertext_hmac(const unsigned char *ciphertext,
                               const unsigned char *hmac_key,
                               unsigned char *hmac)
{
#ifdef USE_MBEDTLS
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), hmac_key, 40, ciphertext, 128, hmac);
#else
  sha1_hmac(hmac_key, 40, ciphertext, 128, hmac);
#endif
}

int verify_signature(const unsigned char *signature,
                     const unsigned char *pubkey,
                     const unsigned char *expected_hmac)
{
  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, pubkey);

#ifdef USE_MBEDTLS
  int ret = mbedtls_rsa_pkcs1_verify(&rsa, default_rng, nullptr, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, 20,
                             expected_hmac, signature);
#else
  int ret = rsa_pkcs1_verify(&rsa, RSA_PUBLIC, RSA_RAW, 20,
                             expected_hmac, signature);
#endif

  rsa_free(&rsa);
  return ret;
}

int init_rsa_context_with_public_key(rsa_context *rsa,
                                      const unsigned char *pubkey)
{
#ifdef USE_MBEDTLS
  // mbedtls_pk_context pk;
  // mbedtls_pk_init(&pk);
  // unsigned char **p = (unsigned char **)&pubkey;
  // int ret = mbedtls_pk_parse_subpubkey(p, *p + 294, &pk);
  // if (ret)
  // {
  //   mbedtls_pk_free(&pk);
  //   return ret;
  // }
  // *rsa = *mbedtls_pk_rsa(pk);
  int ret;
  size_t len;
  mbedtls_asn1_buf alg_params;
  mbedtls_rsa_init(rsa, MBEDTLS_RSA_PKCS_V15, 0);
  unsigned char **p = (unsigned char **)&pubkey;
  unsigned char *end = *p + 294;

  if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
                  MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
  {
      return( MBEDTLS_ERR_PK_KEY_INVALID_FORMAT + ret );
  }

  end = *p + len;

  mbedtls_asn1_buf alg_oid;

  memset( &alg_params, 0, sizeof(mbedtls_asn1_buf) );

  if( ( ret = mbedtls_asn1_get_alg( p, end, &alg_oid, &alg_params ) ) != 0 )
      return( MBEDTLS_ERR_PK_INVALID_ALG + ret );

  if( ( ret = mbedtls_asn1_get_bitstring_null( p, end, &len ) ) != 0 )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY + ret );

  if( *p + len != end )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY +
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

  if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
          MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY + ret );

  if( *p + len != end )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY +
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

  if( ( ret = mbedtls_asn1_get_mpi( p, end, &rsa->N ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( p, end, &rsa->E ) ) != 0 )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY + ret );

  if( *p != end )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY +
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

  if( ( ret = mbedtls_rsa_check_pubkey( rsa ) ) != 0 )
      return( MBEDTLS_ERR_PK_INVALID_PUBKEY );

  rsa->len = mbedtls_mpi_size( &rsa->N );
#else
  rsa_init(rsa, RSA_PKCS_V15, RSA_RAW, NULL, NULL);

  rsa->len = 256;
  mpi_read_binary(&rsa->N, pubkey + 33, 256);
  mpi_read_string(&rsa->E, 16, "10001");
#endif
  return 0;
}

/* Very simple ASN.1 parsing.
 * Mainly needed because, even though all the RSA big integers
 * are always a specific number of bytes, the key generation
 * and encoding process sometimes pads each number with a
 * leading zero byte.
 */
int init_rsa_context_with_private_key(rsa_context *rsa,
                                       const unsigned char *private_key)
{
#ifdef USE_MBEDTLS
  // mbedtls_pk_context pk;
  // mbedtls_pk_init(&pk);
  // int ret = mbedtls_pk_parse_key(&pk, private_key, 612, NULL, 0);
  // if (ret)
  // {
  //   mbedtls_pk_free(&pk);
  //   return ret;
  // }
  // *rsa = *mbedtls_pk_rsa(pk);
  int ret;
  size_t len;
  unsigned char *p, *end;
  mbedtls_rsa_init(rsa, MBEDTLS_RSA_PKCS_V15, 0);

  p = (unsigned char *) private_key;
  end = p + 612;

  /*
    * This function parses the RSAPrivateKey (PKCS#1)
    *
    *  RSAPrivateKey ::= SEQUENCE {
    *      version           Version,
    *      modulus           INTEGER,  -- n
    *      publicExponent    INTEGER,  -- e
    *      privateExponent   INTEGER,  -- d
    *      prime1            INTEGER,  -- p
    *      prime2            INTEGER,  -- q
    *      exponent1         INTEGER,  -- d mod (p-1)
    *      exponent2         INTEGER,  -- d mod (q-1)
    *      coefficient       INTEGER,  -- (inverse of q) mod p
    *      otherPrimeInfos   OtherPrimeInfos OPTIONAL
    *  }
    */
  if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
          MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
  {
      return( MBEDTLS_ERR_PK_KEY_INVALID_FORMAT + ret );
  }

  end = p + len;

  if( ( ret = mbedtls_asn1_get_int( &p, end, &rsa->ver ) ) != 0 )
  {
      return( MBEDTLS_ERR_PK_KEY_INVALID_FORMAT + ret );
  }

  if( rsa->ver != 0 )
  {
      return( MBEDTLS_ERR_PK_KEY_INVALID_VERSION );
  }

  if( ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->N  ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->E  ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->D  ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->P  ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->Q  ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->DP ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->DQ ) ) != 0 ||
      ( ret = mbedtls_asn1_get_mpi( &p, end, &rsa->QP ) ) != 0 )
  {
      mbedtls_rsa_free( rsa );
      return( MBEDTLS_ERR_PK_KEY_INVALID_FORMAT + ret );
  }

  rsa->len = mbedtls_mpi_size( &rsa->N );

  if( p != end )
  {
      mbedtls_rsa_free( rsa );
      return( MBEDTLS_ERR_PK_KEY_INVALID_FORMAT +
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );
  }

  if( ( ret = mbedtls_rsa_check_privkey( rsa ) ) != 0 )
  {
      mbedtls_rsa_free( rsa );
      return( ret );
  }
#else
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
#endif
  return 0;
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
