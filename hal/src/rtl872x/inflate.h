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

#include <stddef.h>
#include <stdint.h>

#define INFLATE_MIN_WINDOW_BITS 8
#define INFLATE_MAX_WINDOW_BITS 15

typedef struct inflate_ctx inflate_ctx;

typedef int (*inflate_output)(const char* data, size_t size, void* user_data);

typedef enum inflate_result {
    INFLATE_DONE = 0,
    INFLATE_NEEDS_MORE_INPUT = 1,
    INFLATE_HAS_MORE_OUTPUT = 2
} inflate_result;

typedef enum inflate_flag {
    INFLATE_HAS_MORE_INPUT = 0x01
} inflate_flag;

typedef struct inflate_opts {
    uint8_t window_bits;
} inflate_opts;

#ifdef __cplusplus
extern "C" {
#endif

int inflate_create(inflate_ctx** ctx, const inflate_opts* opts, inflate_output output, void* user_data);
void inflate_destroy(inflate_ctx* ctx);

int inflate_reset(inflate_ctx* ctx);

int inflate_input(inflate_ctx* ctx, const char* data, size_t* size, unsigned flags);

#ifdef __cplusplus
} // extern "C"
#endif
