/*
 ******************************************************************************
 *  Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "platform_headers.h"

#include <stdint.h>

#include "cellular_backoff_retained.h"
#include "hal_platform.h"
#if HAL_PLATFORM_BACKUP_RAM_NEED_SYNC
#include "backup_ram_hal.h"
#endif

namespace particle {

namespace {

// Arbitrary, just has to be unlikely
// Backup RAM is not zero initialised on nRF52840, so a cold boot holds whatever it powered up with
const uint32_t BACKOFF_RETAINED_MARKER = 0xba0ff5c1;

retained_system struct {
    uint32_t marker;
    uint32_t stage;
} g_retainedBackoff;

} // anonymous namespace

unsigned cellularBackoffRetainedStage() {
    if (g_retainedBackoff.marker != BACKOFF_RETAINED_MARKER) {
        return 0;
    }
    return g_retainedBackoff.stage;
}

void cellularBackoffSetRetainedStage(unsigned stage) {
    if (!stage) {
        // Clear the marker too, so this reads the same as a cold boot
        g_retainedBackoff.marker = 0;
        g_retainedBackoff.stage = 0;
    } else {
        g_retainedBackoff.marker = BACKOFF_RETAINED_MARKER;
        g_retainedBackoff.stage = stage;
    }
#if HAL_PLATFORM_BACKUP_RAM_NEED_SYNC
    // Gen 4 restores backup RAM from flash, so the stage also survives a power cycle
    // Gen 3 holds it in retained SRAM, which a power cycle clears
    hal_backup_ram_sync(nullptr);
#endif
}

} // namespace particle
