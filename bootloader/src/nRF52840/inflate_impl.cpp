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

#include "system_error.h"

namespace {

const size_t BUFFER_SIZE = (1 << INFLATE_MAX_WINDOW_BITS);

inflate_ctx g_ctx = {};
char g_buf[BUFFER_SIZE] = {};
bool g_alloced = false;

} // namespace

int inflate_alloc_ctx(inflate_ctx** ctx, char** buf, size_t buf_size) {
    // Only one decompressor instance can be "allocated" in the bootloader
    if (g_alloced || buf_size > sizeof(g_buf)) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    *ctx = &g_ctx;
    *buf = g_buf;
    g_alloced = true;
    return 0;
}

void inflate_free_ctx(inflate_ctx* ctx, char* buf) {
    g_alloced = false;
}

int inflate_reset_impl(inflate_ctx* ctx) {
    return 0;
}

#endif // HAL_PLATFORM_COMPRESSED_OTA
