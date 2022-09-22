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

#include "storage_hal.h"
#include "flash_hal.h"
#include "exflash_hal.h"
#include "platform_config.h"
#include "system_error.h"
#include "flash_mal.h"
#include "check.h"

int hal_storage_read(hal_storage_id id, uintptr_t addr, uint8_t* buf, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        CHECK_TRUE(hal_flash_read(addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    } else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        CHECK_TRUE(hal_exflash_read(addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    } else if (id == HAL_STORAGE_ID_OTP) {
        CHECK_TRUE(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_write(hal_storage_id id, uintptr_t addr, const uint8_t* buf, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        CHECK_TRUE(hal_flash_write(addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    } else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        CHECK_TRUE(hal_exflash_write(addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    } else if (id == HAL_STORAGE_ID_OTP) {
        CHECK_TRUE(hal_exflash_write_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, addr, buf, size) >= 0, SYSTEM_ERROR_FLASH_IO);
        return size;
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_erase(hal_storage_id id, uintptr_t addr, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        size_t sectors = (size + INTERNAL_FLASH_PAGE_SIZE - 1) / INTERNAL_FLASH_PAGE_SIZE;
        CHECK_TRUE(hal_flash_erase_sector(addr, sectors) >= 0, SYSTEM_ERROR_FLASH_IO);
        return sectors * INTERNAL_FLASH_PAGE_SIZE;
    } else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        size_t sectors = (size + sFLASH_PAGESIZE - 1) / sFLASH_PAGESIZE;
        CHECK_TRUE(hal_exflash_erase_sector(addr, sectors) >= 0, SYSTEM_ERROR_FLASH_IO);
        return sectors * sFLASH_PAGESIZE;
    }

    return SYSTEM_ERROR_NOT_FOUND;
}
