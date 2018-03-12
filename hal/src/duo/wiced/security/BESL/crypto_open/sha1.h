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
 */

/**
 * \file sha1.h
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
#ifndef TROPICSSL_SHA1_H
#define TROPICSSL_SHA1_H

#include "crypto_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * \brief          SHA-1 context setup
     *
     * \param ctx      context to be initialized
     */
    void sha1_starts(sha1_context * ctx);

    /**
     * \brief          SHA-1 process buffer
     *
     * \param ctx      SHA-1 context
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     */
    void sha1_update(sha1_context *ctx, const unsigned char *input, int32_t ilen);

    /**
     * \brief          SHA-1 final digest
     *
     * \param ctx      SHA-1 context
     * \param output   SHA-1 checksum result
     */
    void sha1_finish(sha1_context * ctx, unsigned char output[20]);

    /**
     * \brief          Output = SHA-1( input buffer )
     *
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     * \param output   SHA-1 checksum result
     */
    void sha1(const unsigned char *input, int32_t ilen, unsigned char output[20]);

    /**
     * \brief          Output = SHA-1( file contents )
     *
     * \param path     input file name
     * \param output   SHA-1 checksum result
     *
     * \return         0 if successful, 1 if fopen failed,
     *                 or 2 if fread failed
     */
    int32_t sha1_file(const char *path, unsigned char output[20]);

    /**
     * \brief          SHA-1 HMAC context setup
     *
     * \param ctx      HMAC context to be initialized
     * \param key      HMAC secret key
     * \param keylen   length of the HMAC key
     */
    void sha1_hmac_starts(sha1_context * ctx, const unsigned char *key,
                  uint32_t keylen);

    /**
     * \brief          SHA-1 HMAC process buffer
     *
     * \param ctx      HMAC context
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     */
    void sha1_hmac_update(sha1_context * ctx, const unsigned char *input,
                  uint32_t ilen);

    /**
     * \brief          SHA-1 HMAC final digest
     *
     * \param ctx      HMAC context
     * \param output   SHA-1 HMAC checksum result
     */
    void sha1_hmac_finish(sha1_context * ctx, unsigned char output[20]);

    /**
     * \brief          Output = HMAC-SHA-1( hmac key, input buffer )
     *
     * \param key      HMAC secret key
     * \param keylen   length of the HMAC key
     * \param input    buffer holding the  data
     * \param ilen     length of the input data
     * \param output   HMAC-SHA-1 result
     */
    void sha1_hmac(const unsigned char *key, int32_t keylen,
               const unsigned char *input, int32_t ilen,
               unsigned char output[20]);

    /**
     * \brief          Checkup routine
     *
     * \return         0 if successful, or 1 if the test failed
     */
    int32_t sha1_self_test(int32_t verbose);

#ifdef __cplusplus
}
#endif
#endif                /* sha1.h */
