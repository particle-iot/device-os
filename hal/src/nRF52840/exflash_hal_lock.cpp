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

// TODO: coalesce into a single exflash_hal.cpp
#include "exflash_hal.h"

// Do not define Particle's STATIC_ASSERT() to avoid conflicts with the nRF SDK's own macro
#define NO_STATIC_ASSERT
#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#include "static_recursive_mutex.h"

/* Because non-trivial designated initializers are not supported in GCC,
 * we can't have exflash_hal.c as exflash_hal.cpp and thus can't use
 * StaticRecursiveMutex in it.
 *
 * Initialize mutex here and provide helper functions for locking
 */
static StaticRecursiveMutex s_exflash_mutex;

int hal_exflash_lock(void) {
    return !s_exflash_mutex.lock();
}

int hal_exflash_unlock(void) {
    return !s_exflash_mutex.unlock();
}

#else

__attribute__((weak)) int hal_exflash_lock(void) {
    return 0;
}

__attribute__((weak)) int hal_exflash_unlock(void) {
    return 0;
}
#endif /* MODULE_FUNCTION != MOD_FUNC_BOOTLOADER */
