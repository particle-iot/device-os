/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <sys/types.h>
#include <stddef.h>

typedef int (*bspatch_seek)(ssize_t offs, void* user_data);
typedef int (*bspatch_read)(char* data, size_t size, void* user_data);
typedef int (*bspatch_write)(const char* data, size_t size, void* user_data);

#ifdef __cplusplus
extern "C" {
#endif

int bspatch(bspatch_read patch_read, bspatch_read src_read, bspatch_seek src_seek, bspatch_write dest_write,
        size_t dest_size, void* user_data);

#ifdef __cplusplus
} // extern "C"
#endif
