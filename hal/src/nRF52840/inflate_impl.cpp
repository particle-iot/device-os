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

#include "system_error.h"

#include <memory>

int inflate_alloc_ctx(inflate_ctx** ctx, char** buf, size_t buf_size) {
    std::unique_ptr<char[]> bufPtr(new(std::nothrow) char[buf_size]);
    if (!bufPtr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    std::unique_ptr<inflate_ctx> ctxPtr(new(std::nothrow) inflate_ctx);
    if (!ctxPtr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    *ctx = ctxPtr.release();
    *buf = bufPtr.release();
    return 0;
}

void inflate_free_ctx(inflate_ctx* ctx, char* buf) {
    delete ctx;
    delete[] buf;
}

#endif // HAL_PLATFORM_COMPRESSED_MODULES
