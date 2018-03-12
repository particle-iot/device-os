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
 * Whitespace converted (Tab to 4 spaces, LF to CRLF)
 */

/**
 * \file md4.h
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
#ifndef TROPICSSL_MD4_H
#define TROPICSSL_MD4_H

#include <stdint.h>

/**
 * \brief          MD4 context structure
 */
/* Prevent redefinition of types from crypto_structures.h */
#ifndef CRYPTO_STRUCTURE
typedef struct {
    uint32_t total[2];    /*!< number of bytes processed  */
    uint32_t state[4];    /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */

       unsigned char ipad[64];       /*!< HMAC: inner padding        */
       unsigned char opad[64];       /*!< HMAC: outer padding        */
} md4_context;
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**
    * \brief          MD4 context setup
    *
    * \param ctx      context to be initialized
    */
    void md4_starts(md4_context * ctx);

    /**
    * \brief          MD4 process buffer
    *
    * \param ctx      MD4 context
    * \param input    buffer holding the  data
    * \param ilen     length of the input data
    */
    void md4_update(md4_context * ctx, const unsigned char *input, int32_t ilen);

    /**
    * \brief          MD4 final digest
    *
    * \param ctx      MD4 context
    * \param output   MD4 checksum result
    */
    void md4_finish(md4_context * ctx, unsigned char output[16]);

    /**
    * \brief          Output = MD4( input buffer )
    *
    * \param input    buffer holding the  data
    * \param ilen     length of the input data
    * \param output   MD4 checksum result
    */
    void md4(const unsigned char *input, int32_t ilen, unsigned char output[16]);

   /**
    * \brief          MD4 HMAC context setup
    *
    * \param ctx      HMAC context to be initialized
    * \param key      HMAC secret key
    * \param keylen   length of the HMAC key
    */
    void md4_hmac_starts(md4_context * ctx, const unsigned char *key, uint32_t keylen);

    /**
    * \brief          MD4 HMAC process buffer
    *
    * \param ctx      HMAC context
    * \param input    buffer holding the  data
    * \param ilen     length of the input data
    */
    void md4_hmac_update(md4_context * ctx, const unsigned char *input, uint32_t ilen);

    /**
    * \brief          MD4 HMAC final digest
    *
    * \param ctx      HMAC context
    * \param output   MD4 HMAC checksum result
    */
    void md4_hmac_finish(md4_context * ctx, unsigned char output[16]);

    /**
    * \brief          Output = HMAC-MD4( hmac key, input buffer )
    *
    * \param key      HMAC secret key
    * \param keylen   length of the HMAC key
    * \param input    buffer holding the  data
    * \param ilen     length of the input data
    * \param output   HMAC-MD4 result
    */
    void md4_hmac(const unsigned char *key, int32_t keylen,
                  const unsigned char *input, int32_t ilen,
                  unsigned char output[16]);

    /**
    * \brief          Checkup routine
    *
    * \return         0 if successful, or 1 if the test failed
    */
    int32_t md4_self_test(int32_t verbose);

#ifdef __cplusplus
}
#endif
#endif                      /* md4.h */
