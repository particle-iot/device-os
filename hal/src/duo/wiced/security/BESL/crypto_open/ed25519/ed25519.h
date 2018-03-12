/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 *
 * Ed25519 elliptic curve digital signing functions
 *
 */

#ifndef ED25519_H
#define ED25519_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];

/**
 * Generate a Ed25519 public key from a secret key
 *
 * @param  secret_key The 32 byte secret key
 * @param  public_key Receives the 32 byte output public key
 */
void ed25519_publickey( const ed25519_secret_key secret_key, ed25519_public_key public_key );


/**
 * Sign a message using Ed25519
 *
 * @param  message_data     The message data to sign
 * @param  message_len      The length in bytes of the message data
 * @param  secret_key       The 32 byte secret key
 * @param  public_key       The 32 byte public key
 * @param  signature_output Receives the 64 byte output signature
 */
void ed25519_sign( const unsigned char *message_data, size_t message_len, const ed25519_secret_key secret_key, const ed25519_public_key public_key, ed25519_signature signature_output );

/**
 * Verify an Ed25519 message signature
 *
 * @param  message_data     The message data to verify
 * @param  message_len      The length in bytes of the message data
 * @param  public_key       The 32 byte public key
 * @param  signature Receives the 64 byte output signature
 *
 * @return 0 if signature matches
 */
int ed25519_sign_open( const unsigned char *message_data, size_t message_len, const ed25519_public_key public_key, const ed25519_signature signature);


//int ed25519_sign_open_batch(const unsigned char **m, size_t *mlen, const unsigned char **pk, const unsigned char **RS, size_t num, int *valid);

#ifdef __cplusplus
}
#endif

#endif // ED25519_H
