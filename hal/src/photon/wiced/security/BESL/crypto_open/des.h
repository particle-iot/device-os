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
 * add enum for DES_ENCRYPT/DES_DECRYPT
 */

/**
 * \file des.h
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
#ifndef TROPICSSL_DES_H
#define TROPICSSL_DES_H

#include "crypto_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * \brief          DES key schedule (56-bit, encryption)
     *
     * \param ctx      DES context to be initialized
     * \param key      8-byte secret key
     */
    void des_setkey_enc(des_context * ctx, const unsigned char key[8]);

    /**
     * \brief          DES key schedule (56-bit, decryption)
     *
     * \param ctx      DES context to be initialized
     * \param key      8-byte secret key
     */
    void des_setkey_dec(des_context * ctx, const unsigned char key[8]);

    /**
     * \brief          Triple-DES key schedule (112-bit, encryption)
     *
     * \param ctx      3DES context to be initialized
     * \param key      16-byte secret key
     */
    void des3_set2key_enc(des3_context * ctx, const unsigned char key[16]);

    /**
     * \brief          Triple-DES key schedule (112-bit, decryption)
     *
     * \param ctx      3DES context to be initialized
     * \param key      16-byte secret key
     */
    void des3_set2key_dec(des3_context * ctx, const unsigned char key[16]);

    /**
     * \brief          Triple-DES key schedule (168-bit, encryption)
     *
     * \param ctx      3DES context to be initialized
     * \param key      24-byte secret key
     */
    void des3_set3key_enc(des3_context * ctx, const unsigned char key[24]);

    /**
     * \brief          Triple-DES key schedule (168-bit, decryption)
     *
     * \param ctx      3DES context to be initialized
     * \param key      24-byte secret key
     */
    void des3_set3key_dec(des3_context * ctx, const unsigned char key[24]);

    /**
     * \brief          DES-ECB block encryption/decryption
     *
     * \param ctx      DES context
     * \param input    64-bit input block
     * \param output   64-bit output block
     */
    void des_crypt_ecb(des_context * ctx,
               const unsigned char input[8],
               unsigned char output[8]);

    /**
     * \brief          DES-CBC buffer encryption/decryption
     *
     * \param ctx      DES context
     * \param mode     DES_ENCRYPT or DES_DECRYPT
     * \param length   length of the input data
     * \param iv       initialization vector (updated after use)
     * \param input    buffer holding the input data
     * \param output   buffer holding the output data
     */
    void des_crypt_cbc(des_context * ctx,
               des_mode_t mode,
               int32_t length,
               unsigned char iv[8],
               const unsigned char *input,
               unsigned char *output);

    /**
     * \brief          3DES-ECB block encryption/decryption
     *
     * \param ctx      3DES context
     * \param input    64-bit input block
     * \param output   64-bit output block
     */
    void des3_crypt_ecb(des3_context * ctx,
                const unsigned char input[8],
                unsigned char output[8]);

    /**
     * \brief          3DES-CBC buffer encryption/decryption
     *
     * \param ctx      3DES context
     * \param mode     DES_ENCRYPT or DES_DECRYPT
     * \param length   length of the input data
     * \param iv       initialization vector (updated after use)
     * \param input    buffer holding the input data
     * \param output   buffer holding the output data
     */
    void des3_crypt_cbc(des3_context * ctx,
                des_mode_t mode,
                int32_t length,
                unsigned char iv[8],
                const unsigned char *input,
                unsigned char *output);

    /*
     * \brief          Checkup routine
     *
     * \return         0 if successful, or 1 if the test failed
     */
    int32_t des_self_test(int32_t verbose);

#ifdef __cplusplus
}
#endif
#endif                /* des.h */
