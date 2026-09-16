/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>

SYSTEM_MODE(SEMI_AUTOMATIC);

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

const uint32_t MAGIC = 0x600df00d;

const char* const backupRamFilePath = "/sys/backup_ram.bin";
const char* const phaseFilePath = "/usr/backup_ram_test_phase";

enum class Phase {
    INIT = 0,
    HIBERNATE_PERSISTED,
    COLD_INIT_DONE
};

Phase readPhase() {
    Phase phase = Phase::INIT;
    const int fd = ::open(phaseFilePath, O_RDONLY);
    if (fd >= 0) {
        SCOPE_GUARD({
            ::close(fd);
        });
        if (::read(fd, &phase, sizeof(phase)) != sizeof(phase)) {
            phase = Phase::INIT;
        }
    }
    return phase;
}

void writePhase(Phase phase) {
    const int fd = ::open(phaseFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assertMoreOrEqual(fd, 0);
    SCOPE_GUARD({
        ::close(fd);
    });
    assertEqual((int)sizeof(phase), (int)::write(fd, &phase, sizeof(phase)));
}

retained uint32_t marker;

} // anonymous namespace

test(BACKUP_RAM_01_hibernate_persistence_1) {
    marker = MAGIC;
    writePhase(Phase::HIBERNATE_PERSISTED);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::HIBERNATE).duration(5s));
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}

test(BACKUP_RAM_01_hibernate_persistence_2) {
    assertEqual((int)Phase::HIBERNATE_PERSISTED, (int)readPhase());
    assertEqual(RESET_REASON_POWER_MANAGEMENT, System.resetReason());
    assertEqual((uint32_t)MAGIC, (uint32_t)marker);
}

test(BACKUP_RAM_02_cold_boot_init_1) {
    writePhase(Phase::COLD_INIT_DONE);

    unlink(backupRamFilePath);
    extern uintptr_t platform_backup_ram_all_start[];
    extern uintptr_t platform_backup_ram_all_end;
    memset(platform_backup_ram_all_start, 0,
            (uintptr_t)&platform_backup_ram_all_end - (uintptr_t)platform_backup_ram_all_start);

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(BACKUP_RAM_02_cold_boot_init_2) {
    assertEqual((int)Phase::COLD_INIT_DONE, (int)readPhase());
    assertEqual(RESET_REASON_USER, System.resetReason());
    assertNotEqual((uint32_t)MAGIC, (uint32_t)marker);
}

test(BACKUP_RAM_03_file_recreated_after_cold_boot) {
    assertEqual((int)Phase::COLD_INIT_DONE, (int)readPhase());

    struct stat st = {};
    assertEqual(0, stat(backupRamFilePath, &st));
    assertMoreOrEqual((int)st.st_size, 4);
}

test(BACKUP_RAM_04_cleanup) {
    assertEqual(0, unlink(phaseFilePath));
}
