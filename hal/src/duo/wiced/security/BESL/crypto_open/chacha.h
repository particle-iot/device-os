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
 * ChaCha Cipher - see http://cr.yp.to/chacha.html for details
 */

#ifndef INCLUDED_CHACHA_H_
#define INCLUDED_CHACHA_H_

#include "crypto_structures.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialise ChaCha context with a key
 *
 * @param context  A chacha_context_t that will be used as temporary storage whilst performing ChaCha calculations
 * @param key      The key data - must be either 256 bits (32 bytes) or 128 bits (16 bytes) in length
 * @param key_bits The length of the key - must be either 256 or 128
 */
extern void chacha_keysetup       ( chacha_context_t *context, const uint8_t *key, uint32_t key_bits );

/**
 * Add an initial value to a ChaCha context
 *
 * @param context       A chacha_context_t that has been initialised with chacha_keysetup
 * @param initial_value The initial value data - 8 bytes
 */
extern void chacha_ivsetup        ( chacha_context_t *context, const uint8_t *initial_value );

/**
 * Add an nonce initial value to a ChaCha20 block context
 *
 * @param context       A chacha_context_t that has been initialised with chacha_keysetup
 * @param nonce         The initial value data - 12 bytes
 * @param block_count   The sequence number
 */
extern void chacha20_block_ivsetup( chacha_context_t *context, const uint8_t nonce[12], uint32_t block_count );

/**
 * Add an nonce initial value to a ChaCha20 TLS context
 *
 * @param context       A chacha_context_t that has been initialised with chacha_keysetup
 * @param nonce         The initial value data - 8 bytes
 * @param block_count   The sequence number
 */
extern void chacha20_tls_ivsetup( chacha_context_t *context, const uint8_t nonce[8], uint64_t block_count );

/**
 * Encrypt data with the ChaCha Cipher
 *
 * @param context    A chacha_context_t that has been initialised with chacha_keysetup
 * @param plaintext  The plaintext data to be encoded.
 * @param ciphertext Receives the output encrypted data
 * @param bytes      Size in bytes of the plaintext (and encrypted) data
 * @param rounds     Number of encryption loops to perform e.g. ChaCha20 = 20 rounds
 */
extern void chacha_encrypt_bytes  ( chacha_context_t *context, const uint8_t *plaintext, uint8_t *ciphertext, uint32_t bytes, uint8_t rounds);

/**
 * Decrypt data with the ChaCha Cipher
 *
 * @param context    A chacha_context_t that has been initialised with chacha_keysetup
 * @param ciphertext The encrypted data to be decrypted.
 * @param plaintext  Receives the output plaintext data
 * @param bytes      Size in bytes of the encrypted (and plaintext) data
 * @param rounds     Number of encryption loops to perform e.g. ChaCha20 = 20 rounds
 */
extern void chacha_decrypt_bytes  ( chacha_context_t *context, const uint8_t *ciphertext, uint8_t *plaintext, uint32_t bytes, uint8_t rounds );

/**
 * Generates a stream that can be used for key material using the chacha cipher
 *
 * @param context    A chacha_context_t that has been initialised with chacha_keysetup
 * @param stream     The buffer that will receive the key stream data
 * @param bytes      The number of bytes of keystream data to write to the buffer
 * @param rounds     Number of encryption loops to perform e.g. ChaCha20 = 20 rounds
 */
extern void chacha_keystream_bytes( chacha_context_t *context, uint8_t *stream, uint32_t bytes, uint8_t rounds );

/**
 * Pseudo-random key generator using the ChaCha20 Cipher
 *
 * As per section 2.3 of draft-nir-cfrg-chacha20-poly1305-05
 *
 * @param key           A 256 bit chacha key
 * @param nonce         A 96 bit nonce - THIS MUST NEVER BE REUSED WITH THE SAME KEY
 * @param block_count   Sequence number
 * @param output_random The buffer that will receive the 64 bit random data
 */
extern void chacha20_block_function( const uint8_t key[32],
                                     const uint8_t nonce[12],
                                     uint32_t block_count,
                                     uint8_t output_random[64] );

#ifdef __cplusplus
}
#endif

#endif /* ifndef INCLUDED_CHACHA_H_ */
