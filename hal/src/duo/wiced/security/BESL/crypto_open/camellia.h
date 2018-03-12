/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* Originally taken from TropicSSL
 * https://gitorious.org/tropicssl/
 * commit: 92bb3462dfbdb4568c92be19e8904129a17b1eed
 * Whitespace converted (Tab to 4 spaces, LF to CRLF)
 * unsigned long -> uint32_t for 64bit builds
 */

/**
 * \file camellia.h
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TROPICSSL_CAMELLIA_H
#define TROPICSSL_CAMELLIA_H

#include "crypto_structures.h"

#define CAMELLIA_ENCRYPT     1
#define CAMELLIA_DECRYPT     0

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * \brief          CAMELLIA key schedule (encryption)
     *
     * \param ctx      CAMELLIA context to be initialized
     * \param key      encryption key
     * \param keysize  must be 128, 192 or 256
     */
    void camellia_setkey_enc(camellia_context * ctx, const unsigned char *key,
                 int keysize);

    /**
     * \brief          CAMELLIA key schedule (decryption)
     *
     * \param ctx      CAMELLIA context to be initialized
     * \param key      decryption key
     * \param keysize  must be 128, 192 or 256
     */
    void camellia_setkey_dec(camellia_context * ctx, const unsigned char *key,
                 int keysize);

    /**
     * \brief          CAMELLIA-ECB block encryption/decryption
     *
     * \param ctx      CAMELLIA context
     * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
     * \param input    16-byte input block
     * \param output   16-byte output block
     */
    void camellia_crypt_ecb(camellia_context * ctx,
                int mode,
                const unsigned char input[16],
                unsigned char output[16]);

    /**
     * \brief          CAMELLIA-CBC buffer encryption/decryption
     *
     * \param ctx      CAMELLIA context
     * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
     * \param length   length of the input data
     * \param iv       initialization vector (updated after use)
     * \param input    buffer holding the input data
     * \param output   buffer holding the output data
     */
    void camellia_crypt_cbc(camellia_context * ctx,
                int mode,
                int length,
                unsigned char iv[16],
                const unsigned char *input,
                unsigned char *output);

    /**
     * \brief          CAMELLIA-CFB128 buffer encryption/decryption
     *
     * \param ctx      CAMELLIA context
     * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
     * \param length   length of the input data
     * \param iv_off   offset in IV (updated after use)
     * \param iv       initialization vector (updated after use)
     * \param input    buffer holding the input data
     * \param output   buffer holding the output data
     */
    void camellia_crypt_cfb128(camellia_context * ctx,
                   int mode,
                   int length,
                   int *iv_off,
                   unsigned char iv[16],
                   const unsigned char *input,
                   unsigned char *output);

    /**
     * \brief          Checkup routine
     *
     * \return         0 if successful, or 1 if the test failed
     */
    int camellia_self_test(int verbose);

#ifdef __cplusplus
}
#endif
#endif                /* camellia.h */
