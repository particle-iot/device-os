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

#ifndef HAL_NRF52840_FLASH_COMMON_H
#define HAL_NRF52840_FLASH_COMMON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IS_WORD_ALIGNED(value)                  (((value) & 0x03) == 0)
#define ADDR_ALIGN_WORD(addr)                   ((addr) & 0xFFFFFFFC)
#define ADDR_ALIGN_WORD_RIGHT(addr)             (((addr + 3) / 4) * 4)
#define CEIL_DIV(A, B)                          (((A) + (B) - 1) / (B))
#define FLASH_OPERATION_TEMP_BLOCK_SIZE         256

typedef int (*hal_flash_common_write_cb)(uintptr_t addr, const uint8_t* data, size_t size);
typedef int (*hal_flash_common_read_cb)(uintptr_t addr, uint8_t* data, size_t size);

int hal_flash_common_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size,
                           hal_flash_common_write_cb write_func,
                           hal_flash_common_read_cb read_func);


/* This function can be used for unaligned writes to retrive the data from the flash
 * to keep unaligned unaffected bytes unmodified.
 * Due to how flash works, we can simply return 0xff, as the bits in most flash memories
 * can only be flipped from 1 to 0.
 */
int hal_flash_common_dummy_read(uintptr_t addr, uint8_t* buf, size_t size);

int hal_exflash_lock(void);
int hal_exflash_unlock(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HAL_NRF52840_FLASH_COMMON_H */
