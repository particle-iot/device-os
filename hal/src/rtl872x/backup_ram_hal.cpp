/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "backup_ram_hal.h"
#include "flash_hal.h"
#include "flash_mal.h"
#include "flash_common.h"
#include "core_hal.h"
#include "check.h"

// NOTE: we are using a dedicated flash page for this as timings for writing
// into fs vs raw page are about 10x
// We might extend this functionality later and allow for more frequent syncs
// to flash.
extern uintptr_t platform_backup_ram_all_start[];
extern uintptr_t platform_backup_ram_all_end;
extern uintptr_t platform_backup_ram_persisted_start;
extern uintptr_t platform_backup_ram_persisted_end;
extern uintptr_t platform_backup_ram_persisted_size;

int hal_backup_ram_init(void) {
    int reason = 0;
    uint32_t data;
    CHECK(HAL_Core_Get_Last_Reset_Info(&reason, &data, nullptr));
    if (reason == POWER_MANAGEMENT_RESET) {
        // Woke up from deep sleep
        CHECK(hal_flash_read((uintptr_t)&platform_backup_ram_persisted_start, (uint8_t*)&platform_backup_ram_all_start,
                (size_t)&platform_backup_ram_persisted_size));
    }
    return 0;
}

int hal_backup_ram_sync(void) {
    if (memcmp((void*)&platform_backup_ram_all_start,
            (void*)&platform_backup_ram_persisted_start,
            (uintptr_t)&platform_backup_ram_persisted_size)) {
        CHECK(hal_flash_erase_sector((uintptr_t)&platform_backup_ram_persisted_start,
                CEIL_DIV((uintptr_t)&platform_backup_ram_persisted_size, INTERNAL_FLASH_PAGE_SIZE)));
        CHECK(hal_flash_write((uintptr_t)&platform_backup_ram_persisted_start,
                (const uint8_t*)&platform_backup_ram_all_start, (uintptr_t)&platform_backup_ram_persisted_size));
    }
    return 0;
}
