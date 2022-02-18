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
#include "hw_config.h"
#include "platform_headers.h"
#include "dct_hal.h"
#include "dct.h"

// NOTE: we are using a dedicated flash page for this as timings for writing
// into fs vs raw page are about 10x
// We might extend this functionality later and allow for more frequent syncs
// to flash.
extern uintptr_t platform_backup_ram_all_start[];
extern uintptr_t platform_backup_ram_all_end;
extern uintptr_t platform_backup_ram_persisted_flash_start;
extern uintptr_t platform_backup_ram_persisted_flash_end;
extern uintptr_t platform_backup_ram_persisted_flash_size;

int hal_backup_ram_init(void) {
    // NOTE: using SDK API here as last reset info in core_hal is initialized later
    platform_system_flags_t dctFlags = {};
    dct_read_app_data_copy(DCT_SYSTEM_FLAGS_OFFSET, &dctFlags, DCT_SYSTEM_FLAGS_SIZE);
    if ((BOOT_Reason() & BIT_BOOT_DSLP_RESET_HAPPEN) || SYSTEM_FLAG(entered_hibernate) == 1 || dctFlags.entered_hibernate == 1) {
        SYSTEM_FLAG(entered_hibernate) = 0;
        dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
        // Woke up from deep sleep
        CHECK(hal_flash_read((uintptr_t)&platform_backup_ram_persisted_flash_start, (uint8_t*)&platform_backup_ram_all_start,
                (size_t)&platform_backup_ram_persisted_flash_size));
    }
    return 0;
}

int hal_backup_ram_sync(void) {
    if (memcmp((void*)&platform_backup_ram_all_start,
            (void*)&platform_backup_ram_persisted_flash_start,
            (uintptr_t)&platform_backup_ram_persisted_flash_size)) {
        CHECK(hal_flash_erase_sector((uintptr_t)&platform_backup_ram_persisted_flash_start,
                CEIL_DIV((uintptr_t)&platform_backup_ram_persisted_flash_size, INTERNAL_FLASH_PAGE_SIZE)));
        CHECK(hal_flash_write((uintptr_t)&platform_backup_ram_persisted_flash_start,
                (const uint8_t*)&platform_backup_ram_all_start, (uintptr_t)&platform_backup_ram_persisted_flash_size));
    }
    return 0;
}
