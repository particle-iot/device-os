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

#ifndef EXFLASH_HAL_H
#define EXFLASH_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum hal_exflash_special_sector_t {
    HAL_EXFLASH_SPECIAL_SECTOR_NONE = 0,
    HAL_EXFLASH_SPECIAL_SECTOR_OTP  = 1
} hal_exflash_special_sector_t;

typedef enum hal_exflash_command_t {
    HAL_EXFLASH_COMMAND_NONE            = 0,
    HAL_EXFLASH_COMMAND_LOCK_ENTIRE_OTP = 1,
    HAL_EXFLASH_COMMAND_SLEEP           = 2,
    HAL_EXFLASH_COMMAND_WAKEUP          = 3,
    HAL_EXFLASH_COMMAND_SUSPEND_PGMERS  = 4,
    HAL_EXFLASH_COMMAND_RESET           = 5,
    HAL_EXFLASH_COMMAND_READID          = 6,
    HAL_EXFLASH_COMMAND_GET_OTP_SIZE    = 7,
    HAL_EXFLASH_COMMAND_GET_SIZE        = 8
} hal_exflash_command_t;

typedef enum hal_exflash_state_t {
    HAL_EXFLASH_STATE_DISABLED,
    HAL_EXFLASH_STATE_ENABLED,
    HAL_EXFLASH_STATE_SUSPENDED
} hal_exflash_state_t;

int hal_exflash_init(void);
int hal_exflash_uninit(void);
int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size);
int hal_exflash_erase_sector(uintptr_t addr, size_t num_sectors);
int hal_exflash_erase_block(uintptr_t addr, size_t num_blocks);
int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size);
int hal_exflash_copy_sector(uintptr_t src_addr, size_t dest_addr, size_t data_size);
int hal_exflash_lock(void);
int hal_exflash_unlock(void);

int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size);
int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size);
int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size);
int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size);
int hal_exflash_sleep(bool sleep, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* EXFLASH_HAL_H */
