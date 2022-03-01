/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int hal_flash_init(void);
int hal_flash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size);
int hal_flash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size);
int hal_flash_erase_sector(uintptr_t addr, size_t num_sectors);
int hal_flash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* FLASH_HAL_H */
