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

#include "mbedtls/sha256.h"
#include "mbedtls/platform.h"

#include <string.h>

#ifdef MBEDTLS_SHA256_ALT

#include "cc310_mbedtls.h"

void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->mode = CRYS_HASH_NumOfModes; // CRYS HASH context is not initialized
}

void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    if (ctx->mode != CRYS_HASH_NumOfModes)
    {
        CC310_OPERATION_NO_RESULT(CRYS_HASH_Free(&ctx->user_context));
        ctx->mode = CRYS_HASH_NumOfModes;
    }
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src)
{
    // It seems to be safe to copy CRYS HASH contexts bitwise, but the library (or CC310)
    // seems to maintain some sort of a counter for initialized contexts, so we need to
    // make sure CRYS_HASH_Init() is called for each initialized mbedTLS SHA-256 context
    CRYSError_t ret = CRYS_OK;
    if (dst->mode != src->mode)
    {
        if (dst->mode == CRYS_HASH_NumOfModes)
        {
            CC310_OPERATION(CRYS_HASH_Init(&dst->user_context, src->mode), ret);
            if (ret == CRYS_OK)
            {
                dst->mode = src->mode;
            }
        }
        else
        {
            CC310_OPERATION_NO_RESULT(CRYS_HASH_Free(&dst->user_context));
            dst->mode = CRYS_HASH_NumOfModes;
        }
    }
    if (ret == CRYS_OK && src->mode != CRYS_HASH_NumOfModes)
    {
        memcpy(&dst->user_context, &src->user_context, sizeof(CRYS_HASHUserContext_t));
    }
}

int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224)
{
    if (ctx->mode != CRYS_HASH_NumOfModes)
    {
        CC310_OPERATION_NO_RESULT(CRYS_HASH_Free(&ctx->user_context));
        ctx->mode = CRYS_HASH_NumOfModes;
    }
    CRYSError_t ret = CRYS_OK;
    const CRYS_HASH_OperationMode_t mode = is224 ? CRYS_HASH_SHA224_mode : CRYS_HASH_SHA256_mode;
    CC310_OPERATION(CRYS_HASH_Init(&ctx->user_context, mode), ret);
    if (ret != CRYS_OK)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
    ctx->mode = mode;
    return 0;
}

int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen)
{
    if (ctx->mode == CRYS_HASH_NumOfModes)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
    CRYSError_t ret = CRYS_OK;
    CC310_OPERATION(CRYS_HASH_Update(&ctx->user_context, (unsigned char*)input, ilen), ret);
    if (ret != CRYS_OK)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
    return 0;
}

int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx, unsigned char output[32])
{
    if (ctx->mode == CRYS_HASH_NumOfModes)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
    CRYS_HASH_Result_t result = { 0 };
    CRYSError_t ret = CRYS_OK;
    CC310_OPERATION(CRYS_HASH_Finish(&ctx->user_context, result), ret);
    if (ret != CRYS_OK)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
    const size_t size = (ctx->mode == CRYS_HASH_SHA224_mode) ? CRYS_HASH_SHA224_DIGEST_SIZE_IN_BYTES :
            CRYS_HASH_SHA256_DIGEST_SIZE_IN_BYTES;
    memcpy(output, result, size);
    return 0;
}

int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    return mbedtls_sha256_update_ret(ctx, data, 64);
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)

void mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen)
{
    mbedtls_sha256_update_ret( ctx, input, ilen );
}

void mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    mbedtls_sha256_starts_ret( ctx, is224 );
}

void mbedtls_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    mbedtls_internal_sha256_process( ctx, data );
}

void mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char output[32])
{
    mbedtls_sha256_finish_ret( ctx, output );
}

#endif /* !defined(MBEDTLS_DEPRECATED_REMOVED) */

#endif /* MBEDTLS_SHA256_ALT */
