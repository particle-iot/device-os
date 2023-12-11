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

#if HAL_PLATFORM_COMPRESSED_OTA

#include "inflate_impl.h"

#include "check.h"
#include <algorithm>
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "core_hal.h"
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM

#include "scope_guard.h"

namespace {

#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM

const auto INFLATE_USE_FILESYSTEM_WRITE_CACHE_SIZE = FILESYSTEM_BLOCK_SIZE + 128;
const auto INFLATE_USE_FILESYSTEM_READ_CACHE_SIZE = 1024;

#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM

} // anonymous

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
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    runtime_info_t info = {};
    info.size = sizeof(info);
    HAL_Core_Runtime_Info(&info, nullptr);

    bool useFilesystem = true;

    size_t allocateBufSize = INFLATE_USE_FILESYSTEM_WRITE_CACHE_SIZE + INFLATE_USE_FILESYSTEM_READ_CACHE_SIZE;
    if (info.freeheap >= bufSize * 2) {
        if (!inflate_alloc_ctx(ctx, &buf, bufSize)) {
            useFilesystem = false;
            allocateBufSize = bufSize;
        }
    }

    if (useFilesystem) {
        CHECK(inflate_alloc_ctx(ctx, &buf, allocateBufSize));
    }
#else
    size_t allocateBufSize = bufSize;
    CHECK(inflate_alloc_ctx(ctx, &buf, allocateBufSize));
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#if !HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    (*ctx)->buf = buf;
#else
    if (useFilesystem) {
        (*ctx)->buf = nullptr;
        (*ctx)->write_cache = buf;
        (*ctx)->read_cache = buf + INFLATE_USE_FILESYSTEM_WRITE_CACHE_SIZE;
        (*ctx)->write_cache_size = INFLATE_USE_FILESYSTEM_WRITE_CACHE_SIZE;
        (*ctx)->write_cache_block_size = FILESYSTEM_BLOCK_SIZE;
        (*ctx)->read_cache_size = INFLATE_USE_FILESYSTEM_READ_CACHE_SIZE;
    } else {
        (*ctx)->buf = buf;
    }

#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    (*ctx)->buf_size = bufSize;
    (*ctx)->output = output;
    (*ctx)->user_data = user_data;
    NAMED_SCOPE_GUARD(sg, {
        inflate_destroy(*ctx);
        *ctx = nullptr;
    });
    CHECK(inflate_reset(*ctx));
    sg.dismiss();
    return 0;
}

void inflate_destroy(inflate_ctx* ctx) {
    if (ctx) {
#if !HAL_PLATFORM_INFLATE_USE_FILESYSTEM
        inflate_free_ctx(ctx, ctx->buf);
#else
        inflate_free_ctx(ctx, ctx->buf ? ctx->buf : ctx->write_cache);
#endif // !HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    }
}

int inflate_reset(inflate_ctx* ctx) {
    tinfl_init(&ctx->decomp);
    CHECK(inflate_reset_impl(ctx));
    ctx->buf_offs = 0;
    ctx->buf_avail = 0;
    ctx->result = INFLATE_NEEDS_MORE_INPUT;
    ctx->done = false;
    return 0;
}

int inflate_input(inflate_ctx* ctx, const char* data, size_t* size, unsigned flags) {
    if (ctx->result <= 0) { // INFLATE_DONE or an error
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
        if (ctx->result == INFLATE_DONE && ctx->buf == nullptr && ctx->buf_avail > 0) {
            // We still need to call into ctx->decomp.read_buf a few times
        } else {
            return SYSTEM_ERROR_INVALID_STATE;
        }
#else
        return SYSTEM_ERROR_INVALID_STATE;
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    }
    size_t srcOffs = 0;
    bool needMore = false;
    for (;;) {
        if (ctx->buf_avail > 0) {
#if !HAL_PLATFORM_INFLATE_USE_FILESYSTEM
            const int n = ctx->output(ctx->buf + ctx->buf_offs, ctx->buf_avail, ctx->user_data);
#else
            int n = 0;
            if (ctx->buf == nullptr) {
                n = ctx->decomp.read_buf(nullptr, ctx->buf_offs, ctx->buf_avail, ctx);
                if (n > 0 && ctx->read_request && ctx->read_request_size > 0) {
                    n = ctx->output(ctx->read_request, ctx->read_request_size, ctx->user_data);
                } else {
                    // Something went wrong
                    n = SYSTEM_ERROR_INTERNAL;
                }
            } else {
                n = ctx->output(ctx->buf + ctx->buf_offs, ctx->buf_avail, ctx->user_data);
            }
#endif // !HAL_PLATFORM_INFLATE_USE_FILESYSTEM
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
        const uint32_t tinflFlags = 
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
                (ctx->buf == nullptr ? TINFL_FLAG_OUTPUT_BUFFER_NOT_IN_RAM : 0) |
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
                ((flags & INFLATE_HAS_MORE_INPUT) ? TINFL_FLAG_HAS_MORE_INPUT : 0);

        const auto status = tinfl_decompress(&ctx->decomp, (const mz_uint8*)data + srcOffs, &srcSize,
                (mz_uint8*)ctx->buf, (mz_uint8*)ctx->buf + ctx->buf_offs, &destSize,
                tinflFlags);

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

#endif // HAL_PLATFORM_COMPRESSED_OTA
