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
 * int arguments/returns changed to int32_t
 * Add HKDF function prototype
 * remove sha4_file
 */

/**
 * \file sha4.h
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
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
#ifndef TROPICSSL_SHA4_H
#define TROPICSSL_SHA4_H

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define UL64(x) x##ui64
#define int64 __int64
#else
#define UL64(x) x##ULL
#define int64 long long
#endif

#include <stdint.h>
#include "crypto_structures.h"

/**
 * \brief          SHA-512 context structure
 */

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * \brief          SHA-512 context setup
     *
     * \param ctx      context to be initialized
     * \param is384    0 = use SHA512, 1 = use SHA384
     */
    void sha4_starts(sha4_context * ctx, int32_t is384);

    /**
     * \brief          SHA-512 process buffer
     *
     * \param ctx      SHA-512 context
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     */
    void sha4_update(sha4_context * ctx, const unsigned char *input, int32_t ilen);

    /**
     * \brief          SHA-512 final digest
     *
     * \param ctx      SHA-512 context
     * \param output   SHA-384/512 checksum result
     */
    void sha4_finish(sha4_context * ctx, unsigned char output[64]);

    /**
     * \brief          Output = SHA-512( input buffer )
     *
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     * \param output   SHA-384/512 checksum result
     * \param is384    0 = use SHA512, 1 = use SHA384
     */
    void sha4(const unsigned char *input, int32_t ilen,
          unsigned char output[64], int32_t is384);

    /**
     * \brief          SHA-512 HMAC context setup
     *
     * \param ctx      HMAC context to be initialized
     * \param is384    0 = use SHA512, 1 = use SHA384
     * \param key      HMAC secret key
     * \param keylen   length of the HMAC key
     */
    void sha4_hmac_starts(sha4_context * ctx, const unsigned char *key,
                  int32_t keylen, int32_t is384);

    /**
     * \brief          SHA-512 HMAC process buffer
     *
     * \param ctx      HMAC context
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     */
    void sha4_hmac_update(sha4_context * ctx, const unsigned char *input,
                  int32_t ilen);

    /**
     * \brief          SHA-512 HMAC final digest
     *
     * \param ctx      HMAC context
     * \param output   SHA-384/512 HMAC checksum result
     */
    void sha4_hmac_finish(sha4_context * ctx, unsigned char output[64]);

    /**
     * \brief          Output = HMAC-SHA-512( hmac key, input buffer )
     *
     * \param key      HMAC secret key
     * \param keylen   length of the HMAC key
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     * \param output   HMAC-SHA-384/512 result
     * \param is384    0 = use SHA512, 1 = use SHA384
     */
    void sha4_hmac(const unsigned char *key, int32_t keylen,
               const unsigned char *input, int32_t ilen,
               unsigned char output[64], int32_t is384);


    /*
     * \brief          HMAC Key Derivation Function for SHA-512 / SHA-384
     *
     * \description    Generates keying material using HKDF.
     *
     * \param salt[in]      The optional salt value (a non-secret random value);
     *                      if not provided (salt == NULL), it is set internally
     *                      to a string of HashLen(whichSha) zeros.
     * \param salt_len[in]  The length of the salt value.  (Ignored if salt == NULL.)
     * \param ikm[in]       Input keying material.
     * \param ikm_len[in]   The length of the input keying material.
     * \param info[in]      The optional context and application specific information.
     *                      If info == NULL or a zero-length string, it is ignored.
     * \param info_len[in]  The length of the optional context and application specific
     *                      information.  (Ignored if info == NULL.)
     * \param okm[out]      Where the HKDF is to be stored.
     * \param okm_len[in]   The length of the buffer to hold okm.
     *                      okm_len must be <= 255 * USHABlockSize(whichSha)
     * \param is384         0 = use SHA512, 1 = use SHA384
     *
     * \return              0 = success
     *
     */
    int sha4_hkdf(
        const unsigned char *salt, int salt_len,
        const unsigned char *ikm, int ikm_len,
        const unsigned char *info, int info_len,
        uint8_t okm[ ], int okm_len,
        int32_t is384);

    /**
     * \brief          Checkup routine
     *
     * \return         0 if successful, or 1 if the test failed
     */
    int32_t sha4_self_test(int32_t verbose);

#ifdef __cplusplus
}
#endif
#endif                /* sha4.h */
