/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mbedtls/aes.h"

// #include <openthread-core-config.h>
#include <string.h>

#ifdef MBEDTLS_AES_ALT

#include "aes_alt_cc310.h"

#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
#include <nrf.h>
#include "aes_alt_soft.h"
#endif

void mbedtls_aes_init(mbedtls_aes_context * ctx)
{
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    uint32_t  active_vector_id = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;

    // Check if this function is called from main thread.
    if (active_vector_id == 0)
    {
        aes_soft_init(ctx);
#endif
        aes_cc310_init(ctx);
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
        ctx->using_cc310 = true;
    }
    else
    {
        aes_soft_init(ctx);
        ctx->using_cc310 = false;
    }
#endif
}

void mbedtls_aes_free(mbedtls_aes_context * ctx)
{
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    if (ctx->using_cc310)
    {
#endif
        aes_cc310_free(ctx);
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
        aes_soft_free(ctx);
    }
    else
    {
        aes_soft_free(ctx);
    }
#endif
}

int mbedtls_aes_setkey_enc(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits)
{
    int result;

#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    if (ctx->using_cc310 && keybits == 128)
    {
#endif
        result = aes_cc310_setkey_enc(ctx, key, keybits);
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    }
    else
    {
        ctx->using_cc310 = false;
        result = aes_soft_setkey_enc(ctx, key, keybits);
    }
#endif

    return result;
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits)
{
    return mbedtls_aes_setkey_enc(ctx, key, keybits);
}

int mbedtls_aes_crypt_ecb(mbedtls_aes_context * ctx,
                          int                   mode,
                          const unsigned char   input[16],
                          unsigned char         output[16])
{
    int result;

#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    if (ctx->using_cc310)
    {
#endif
        result = aes_cc310_crypt_ecb(ctx, mode, input, output);
#if defined(NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT) && NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
    }
    else
    {
        result = aes_soft_crypt_ecb(ctx, mode, input, output);
    }
#endif

    return result;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * AES-CBC buffer encryption/decryption
 */
int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output )
{
    int i;
    unsigned char temp[16];

    if( length % 16 )
        return( MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH );

#if defined(MBEDTLS_PADLOCK_C) && defined(MBEDTLS_HAVE_X86)
    if( aes_padlock_ace )
    {
        if( mbedtls_padlock_xcryptcbc( ctx, mode, length, iv, input, output ) == 0 )
            return( 0 );

        // If padlock data misaligned, we just fall back to
        // unaccelerated mode
        //
    }
#endif

    if( mode == MBEDTLS_AES_DECRYPT )
    {
        while( length > 0 )
        {
            memcpy( temp, input, 16 );
            mbedtls_aes_crypt_ecb( ctx, mode, input, output );

            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 16 );

            input  += 16;
            output += 16;
            length -= 16;
        }
    }
    else
    {
        while( length > 0 )
        {
            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            mbedtls_aes_crypt_ecb( ctx, mode, output, output );
            memcpy( iv, output, 16 );

            input  += 16;
            output += 16;
            length -= 16;
        }
    }

    return( 0 );
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#endif /* MBEDTLS_AES_ALT */
