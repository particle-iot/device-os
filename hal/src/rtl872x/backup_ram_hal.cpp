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
#include "rtl8721d.h"
#include "timer_hal.h"
#include "static_recursive_mutex.h"
#include "scope_guard.h"

// NOTE: we are using a dedicated flash page for this as timings for writing
// into fs vs raw page are about 10x
// We might extend this functionality later and allow for more frequent syncs
// to flash.
extern uintptr_t platform_backup_ram_all_start[];
extern uintptr_t platform_backup_ram_all_end;
extern uintptr_t platform_backup_ram_persisted_flash_start;
extern uintptr_t platform_backup_ram_persisted_flash_end;
extern uintptr_t platform_backup_ram_persisted_flash_size;

namespace {

system_tick_t lastSyncTimeMs = 0;
constexpr system_tick_t syncIntervalMs = 10000;

StaticRecursiveMutex backupMutex;

uint8_t backupRamShadow[HAL_PLATFORM_BACKUP_RAM_SIZE] = {};
}

constexpr uint32_t HAL_BACKUP_RAM_VALID_VALUE = 0x4a171bc4;
retained_system uint32_t g_backupRamValidMarker;

class BackupRamLock {
public:
    BackupRamLock(bool threading = true)
            : locked_(false) {
        lock();
    }

    ~BackupRamLock() {
        if (locked_) {
            unlock();
        }
    }

    BackupRamLock(BackupRamLock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        backupMutex.lock();
        locked_ = true;
    }

    void unlock() {
        backupMutex.unlock();
        locked_ = false;
    }

    BackupRamLock(const BackupRamLock&) = delete;
    BackupRamLock& operator=(const BackupRamLock&) = delete;

private:
    bool locked_;
};

int hal_backup_ram_init(void) {
    SPARK_ASSERT(sizeof(backupRamShadow) == (size_t)&platform_backup_ram_persisted_flash_size);

    // NOTE: using SDK API here as last reset info in core_hal is initialized later
    platform_system_flags_t dctFlags = {};
    dct_read_app_data_copy(DCT_SYSTEM_FLAGS_OFFSET, &dctFlags, DCT_SYSTEM_FLAGS_SIZE);
    // In case of software reset or watchdog reset (software reset also uses watchdog reset),
    // RAM is preserved and we may end up restoring stale data out of flash instead.
    // Check g_backupRamValidMarker value which resides in system retained area, if it's valid
    // our backup RAM is fine as-is and we shouldn't restore anything.
    SCOPE_GUARD({
        g_backupRamValidMarker = HAL_BACKUP_RAM_VALID_VALUE;
    });
    if ((g_backupRamValidMarker != HAL_BACKUP_RAM_VALID_VALUE) &&
            ((BOOT_Reason() & BIT_BOOT_DSLP_RESET_HAPPEN) || SYSTEM_FLAG(restore_backup_ram) == 1 || dctFlags.restore_backup_ram == 1)) {
        SYSTEM_FLAG(restore_backup_ram) = 0;
        dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
        // Woke up from deep sleep
        CHECK(hal_flash_read((uintptr_t)&platform_backup_ram_persisted_flash_start, (uint8_t*)&platform_backup_ram_all_start,
                (size_t)&platform_backup_ram_persisted_flash_size));
    }
    return SYSTEM_ERROR_NONE;
}

int hal_backup_ram_sync(void* reserved) {
    BackupRamLock lk;
    if (memcmp((void*)&platform_backup_ram_all_start, backupRamShadow, sizeof(backupRamShadow))) {
        memcpy(backupRamShadow, (void*)&platform_backup_ram_all_start, sizeof(backupRamShadow));
        if (SYSTEM_FLAG(restore_backup_ram) != 1) {
            SYSTEM_FLAG(restore_backup_ram) = 1;
            dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
        }
        CHECK(hal_flash_erase_sector((uintptr_t)&platform_backup_ram_persisted_flash_start,
                CEIL_DIV((uintptr_t)&platform_backup_ram_persisted_flash_size, INTERNAL_FLASH_PAGE_SIZE)));
        CHECK(hal_flash_write((uintptr_t)&platform_backup_ram_persisted_flash_start,
                backupRamShadow, sizeof(backupRamShadow)));
    }
    return SYSTEM_ERROR_NONE;
}

int hal_backup_ram_routine(void) {
    auto now = hal_timer_millis(nullptr);
    if (now - lastSyncTimeMs >= syncIntervalMs) {
        lastSyncTimeMs = now;
        return hal_backup_ram_sync(nullptr);
    }
    return SYSTEM_ERROR_NONE;
}
