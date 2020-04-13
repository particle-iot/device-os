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

struct inflate_ctx {
    tinfl_decompressor decomp;
    char* buf;
    size_t buf_size;
    size_t buf_offs;
    size_t buf_avail;
    tinfl_status last_status;
    inflate_output output;
    void* user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int inflate_alloc_ctx(inflate_ctx** ctx, char** buf, size_t buf_size);
void inflate_free_ctx(inflate_ctx* ctx, char* buf);

#ifdef __cplusplus
} // extern "C"
#endif
