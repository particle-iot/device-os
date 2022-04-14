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

#include "hal_platform.h"

#if HAL_PLATFORM_NEWLIB

#include <sys/reent.h>
#include <stddef.h>
#include <stdint.h>
#include <_newlib_version.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define NEWLIB_VERSION_NUM (((__NEWLIB__) << 24) | \
                            ((__NEWLIB_MINOR__) << 16) | \
                            ((__NEWLIB_PATCHLEVEL__) << 8))

typedef void (*newlib_impure_cb)(struct _reent* r, size_t size, uint32_t version, void* ctx);

void newlib_impure_ptr_callback(newlib_impure_cb cb, void* ctx);
void newlib_impure_ptr_change(struct _reent* r);
void newlib_impure_ptr_change_module(struct _reent* r, size_t size, uint32_t version);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_PLATFORM_NEWLIB