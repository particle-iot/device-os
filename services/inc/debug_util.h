/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

/**
 * Watchpoint type.
 */
typedef enum watchpoint_type {
    WATCHPOINT_TYPE_WRITE = 0, ///< Write access.
    WATCHPOINT_TYPE_READ = 1, ///< Read access.
    WATCHPOINT_TYPE_READ_WRITE = 2 ///< Read/write access.
} watchpoint_type;

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_NRF52840

/**
 * Set a watchpoint.
 *
 * @param addr Region address. Must be aligned by `size`.
 * @param size Region size. Must be a power of 2.
 * @param type Watchpoint type as defined by the `watchpoint_type` enum.
 * @returns Index of the newly set watchpoint or an error code defined by the `system_error_t` enum.
 */
int set_watchpoint(const void* addr, size_t size, int type); // TODO: Inline or export via a dynalib

/**
 * Clear a watchpoint.
 *
 * @param idx Watchpoint index.
 */
void clear_watchpoint(int idx);

/**
 * Generate a breakpoint exception.
 */
void breakpoint();

#endif // HAL_PLATFORM_NRF52840

#ifdef __cplusplus
} // extern "C"
#endif
