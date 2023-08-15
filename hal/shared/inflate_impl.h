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

#pragma once

#include "inflate.h"

#include "miniz.h"
#include "miniz_tinfl.h"
#include "hal_platform.h"

#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "filesystem.h"
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM

struct inflate_ctx {
    tinfl_decompressor decomp;
    char* buf;
    size_t buf_size;
    size_t buf_offs;
    size_t buf_avail;
    inflate_output output;
    void* user_data;
    int result;
    bool done;
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    lfs_file_t temp_file;
    char* temp_file_name;
    char* write_cache;
    char* read_cache;
    size_t write_cache_size;
    size_t read_cache_size;
    size_t write_cache_pos;
    size_t read_cache_pos;
    size_t write_cache_block_size;
    char* read_request;
    size_t read_request_size;
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
};

#ifdef __cplusplus
extern "C" {
#endif

int inflate_alloc_ctx(inflate_ctx** ctx, char** buf, size_t buf_size);
void inflate_free_ctx(inflate_ctx* ctx, char* buf);
int inflate_reset_impl(inflate_ctx* ctx);

#ifdef __cplusplus
} // extern "C"
#endif
