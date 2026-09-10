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
#include "flash_mal.h"
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
#include "dtls_session_persist.h"
#include "simple_file_storage.h"

extern uintptr_t platform_backup_ram_all_start[];
extern uintptr_t platform_backup_ram_all_end;
extern uintptr_t platform_backup_ram_persisted_flash_size;

extern SessionPersistDataOpaque session;

namespace {

StaticRecursiveMutex backupMutex;

uint32_t persistedCrc = 0;

const char* const backupRamFilePath = "/sys/backup_ram.bin";

using namespace particle;

int saveBackupRamFile() {
    persistedCrc = Compute_CRC32((const uint8_t*)&platform_backup_ram_all_start, (size_t)&platform_backup_ram_persisted_flash_size, nullptr);
    return SimpleFileStorage::save(backupRamFilePath, &platform_backup_ram_all_start,
            (size_t)&platform_backup_ram_persisted_flash_size);
}

int loadBackupRamFile() {
    const int r = SimpleFileStorage::load(backupRamFilePath, &platform_backup_ram_all_start,
            (size_t)&platform_backup_ram_persisted_flash_size);
    if (r != (int)(size_t)&platform_backup_ram_persisted_flash_size) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    persistedCrc = Compute_CRC32((const uint8_t*)&platform_backup_ram_all_start, (size_t)&platform_backup_ram_persisted_flash_size, nullptr);
    return SYSTEM_ERROR_NONE;
}

void initRetainedSystem() {
    extern char link_global_retained_system_initial_values;
    extern char link_global_retained_system_start;
    extern char link_global_retained_system_end;
    const size_t len = &link_global_retained_system_end - &link_global_retained_system_start;
    memcpy(&link_global_retained_system_start, &link_global_retained_system_initial_values, len);
    persistedCrc = Compute_CRC32((const uint8_t*)&platform_backup_ram_all_start, (size_t)&platform_backup_ram_persisted_flash_size, nullptr);
}

} // anonymous namespace

constexpr uint32_t HAL_BACKUP_RAM_VALID_VALUE = 0x94baa52a;
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
            ((BOOT_Reason() & BIT_BOOT_DSLP_RESET_HAPPEN) ||
             (SYSTEM_FLAG(restore_backup_ram) & SYSTEM_FLAG_RESTOR_BACKUP_RAM_MASK) ||
             (dctFlags.restore_backup_ram & SYSTEM_FLAG_RESTOR_BACKUP_RAM_MASK))) {
        SYSTEM_FLAG(restore_backup_ram) &= ~SYSTEM_FLAG_RESTOR_BACKUP_RAM_MASK;
        dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
        // Woke up from deep sleep
        if (loadBackupRamFile() != SYSTEM_ERROR_NONE) {
            // Nothing to restore
            initRetainedSystem();
            memset(&session, 0, sizeof(session));
            return SYSTEM_ERROR_NONE;
        }

        if ((BOOT_Reason() & BIT_BOOT_DSLP_RESET_HAPPEN) && !(SYSTEM_FLAG(restore_backup_ram) & SYSTEM_FLAG_SESSION_DATA_STALE_MASK)) {
            // The session info restored out of flash is valid
            // Invalidate the session info in flash, cos it's probably stale from now on
            auto s = new SessionPersistDataOpaque();
            if (s) {
                std::swap(*s, session);
                memset(&session, 0, sizeof(session));
                hal_backup_ram_sync(nullptr);
                std::swap(*s, session);
                delete s;
            }
        } else {
            // Other cases:
            // 1. Pin reset / PoR / BoR
            // 2. Waking up from hibernate mode, but the session info is stale
            memset(&session, 0, sizeof(session));
        }
    } else if (g_backupRamValidMarker != HAL_BACKUP_RAM_VALID_VALUE) {
        // Cold boot: initialize the system region of the backup RAM with initial values
        initRetainedSystem();
    }
    return SYSTEM_ERROR_NONE;
}

int hal_backup_ram_sync(void* reserved) {
    BackupRamLock lk;
    const uint32_t crc = Compute_CRC32((const uint8_t*)&platform_backup_ram_all_start, (size_t)&platform_backup_ram_persisted_flash_size, nullptr);
    if (crc == persistedCrc) {
        return SYSTEM_ERROR_NONE;
    }
    if (!(SYSTEM_FLAG(restore_backup_ram) & SYSTEM_FLAG_RESTOR_BACKUP_RAM_MASK)) {
        SYSTEM_FLAG(restore_backup_ram) |= SYSTEM_FLAG_RESTOR_BACKUP_RAM_MASK;
        dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
    }
    CHECK(saveBackupRamFile());

    // We don't care about this bit in DCT
    SYSTEM_FLAG(restore_backup_ram) &= ~SYSTEM_FLAG_SESSION_DATA_STALE_MASK;
    return SYSTEM_ERROR_NONE;
}

int hal_backup_ram_routine(void) {
    // Only explicit System.backupRamSync() or entry into hibernate trigger sync into flash
    return SYSTEM_ERROR_NONE;
}
