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
    ctx->last_status = TINFL_STATUS_NEEDS_MORE_INPUT;
}

int inflate_input(inflate_ctx* ctx, const char* data, size_t* size, unsigned flags) {
    size_t offs = 0;
    for (;;) {
        if (ctx->buf_avail > 0) {
            const size_t n = CHECK(ctx->output(ctx->buf + ctx->buf_offs, ctx->buf_avail, ctx->user_data));
            if (n > ctx->buf_avail) {
                return SYSTEM_ERROR_OUT_OF_RANGE;
            }
            ctx->buf_offs = (ctx->buf_offs + n) % ctx->buf_size;
            ctx->buf_avail -= n;
            if (ctx->buf_avail > 0) {
                break;
            }
        }
        if (ctx->last_status == TINFL_STATUS_DONE) {
            break;
        }
        size_t n = *size - offs;
        ctx->buf_avail = ctx->buf_size - ctx->buf_offs;
        ctx->last_status = tinfl_decompress(&ctx->decomp, (const mz_uint8*)data + offs, &n,
                (mz_uint8*)ctx->buf, (mz_uint8*)ctx->buf + ctx->buf_offs, &ctx->buf_avail,
                (flags & INFLATE_HAS_MORE_INPUT) ? TINFL_FLAG_HAS_MORE_INPUT : 0);
        if (ctx->last_status < 0) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        offs += n;
    }
    *size = offs;
    if (ctx->buf_avail > 0) {
        return INFLATE_HAS_MORE_OUTPUT;
    }
    if (ctx->last_status == TINFL_STATUS_NEEDS_MORE_INPUT) {
        return INFLATE_NEEDS_MORE_INPUT;
    }
    return INFLATE_DONE;
}

#endif // HAL_PLATFORM_COMPRESSED_MODULES
