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

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum hal_storage_id {
    HAL_STORAGE_ID_INVALID = 0,
    HAL_STORAGE_ID_INTERNAL_FLASH = 1,
    HAL_STORAGE_ID_EXTERNAL_FLASH = 2,
    HAL_STORAGE_ID_OTP = 3,
    HAL_STORAGE_ID_MAX = 0x7fffffff
} hal_storage_id;

#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
int hal_storage_read(hal_storage_id id, uintptr_t addr, uint8_t* buf, size_t size);
int hal_storage_write(hal_storage_id id, uintptr_t addr, const uint8_t* buf, size_t size);
int hal_storage_erase(hal_storage_id id, uintptr_t addr, size_t size);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

#ifdef __cplusplus
}
#endif // __cplusplus
