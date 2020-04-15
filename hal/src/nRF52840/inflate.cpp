/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "hal_platform.h"

#if HAL_PLATFORM_COMPRESSED_MODULES

#include "inflate_impl.h"

#include "check.h"

int inflate_create(inflate_ctx** ctx, const inflate_opts* opts, inflate_output output, void* user_data) {
    CHECK_TRUE(output, SYSTEM_ERROR_INVALID_ARGUMENT);
    size_t bufSize = (1 << INFLATE_MAX_WINDOW_BITS);
    if (opts && opts->window_bits) {
        if (opts->window_bits < INFLATE_MIN_WINDOW_BITS || opts->window_bits > INFLATE_MAX_WINDOW_BITS) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        bufSize = (1 << opts->window_bits);
    }
    char* buf = nullptr;
    CHECK(inflate_alloc_ctx(ctx, &buf, bufSize));
    (*ctx)->buf = buf;
    (*ctx)->buf_size = bufSize;
    (*ctx)->output = output;
    (*ctx)->user_data = user_data;
    inflate_reset(*ctx);
    return 0;
}

void inflate_destroy(inflate_ctx* ctx) {
    if (ctx) {
        inflate_free_ctx(ctx, ctx->buf);
    }
}

void inflate_reset(inflate_ctx* ctx) {
    tinfl_init(&ctx->decomp);
    ctx->buf_offs = 0;
    ctx->buf_avail = 0;
    ctx->result = INFLATE_NEEDS_MORE_INPUT;
    ctx->done = false;
}

int inflate_input(inflate_ctx* ctx, const char* data, size_t* size, unsigned flags) {
    if (ctx->result <= 0) { // INFLATE_DONE or an error
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t srcOffs = 0;
    bool needMore = false;
    for (;;) {
        if (ctx->buf_avail > 0) {
            const int n = ctx->output(ctx->buf + ctx->buf_offs, ctx->buf_avail, ctx->user_data);
            if (n < 0) {
                ctx->result = n;
                break;
            }
            if ((size_t)n > ctx->buf_avail) {
                ctx->result = SYSTEM_ERROR_OUT_OF_RANGE;
                break;
            }
            ctx->buf_offs += n;
            if (ctx->buf_offs == ctx->buf_size) {
                ctx->buf_offs = 0;
            } else if (ctx->buf_offs > ctx->buf_size) { // Sanity check
                ctx->result = SYSTEM_ERROR_INTERNAL;
                break;
            }
            ctx->buf_avail -= n;
            if (ctx->buf_avail > 0) {
                ctx->result = INFLATE_HAS_MORE_OUTPUT;
                break;
            }
        }
        if (needMore) {
            ctx->result = INFLATE_NEEDS_MORE_INPUT;
            break;
        }
        if (ctx->done) {
            // tinfl_decompress() may return TINFL_STATUS_DONE even if there's still some data left in
            // the input buffer. In the context of this API, this situation is interpreted as an error,
            // i.e. the caller is not allowed to provide more data for decompression than necessary.
            //
            // Note that having the INFLATE_HAS_MORE_INPUT flag set for the last chunk of the compressed
            // data is fine, as the caller might not know the total size of the data in advance
            if (srcOffs < *size) {
                ctx->result = SYSTEM_ERROR_BAD_DATA;
            } else {
                ctx->result = INFLATE_DONE;
            }
            break;
        }
        size_t srcSize = *size - srcOffs;
        size_t destSize = ctx->buf_size - ctx->buf_offs;
        const auto status = tinfl_decompress(&ctx->decomp, (const mz_uint8*)data + srcOffs, &srcSize,
                (mz_uint8*)ctx->buf, (mz_uint8*)ctx->buf + ctx->buf_offs, &destSize,
                (flags & INFLATE_HAS_MORE_INPUT) ? TINFL_FLAG_HAS_MORE_INPUT : 0);
        if (status < 0) {
            ctx->result = SYSTEM_ERROR_BAD_DATA;
            break;
        }
        if (status == TINFL_STATUS_NEEDS_MORE_INPUT) {
            needMore = true;
        } else if (status == TINFL_STATUS_DONE) {
            ctx->done = true;
        } else if (!destSize) { // Sanity check to prevent the infinite loop
            ctx->result = SYSTEM_ERROR_INTERNAL;
            break;
        }
        ctx->buf_avail = destSize;
        srcOffs += srcSize;
    }
    if (ctx->result >= 0) { // INFLATE_DONE or an intermediate status
        *size = srcOffs;
    }
    return ctx->result;
}

#endif // HAL_PLATFORM_COMPRESSED_MODULES
