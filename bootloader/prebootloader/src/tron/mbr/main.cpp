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

#include <cstdlib>
extern "C" {
#include "rtl8721d.h"
}
#include "module_info.h"
#include "hw_config.h"
#include "bootloader_update.h"

__attribute__((used)) void* dynalib_table_location = 0; // part1 dynalib location

extern uintptr_t link_part1_flash_start;
extern uintptr_t link_part1_flash_end;
extern uintptr_t link_part1_module_info_flash_start;
extern uintptr_t link_part1_dynalib_table_flash_start;
extern uintptr_t link_part1_dynalib_table_ram_start;

extern "C" {
int bootloader_part1_preinit(void);
int bootloader_part1_init(void);
int bootloader_part1_setup(void);
int bootloader_part1_loop(void);
}

static bool isPart1ImageValid() {
    module_info_t info = {};
    _memcpy(&info, &link_part1_module_info_flash_start, sizeof(module_info_t));
    if (((uint32_t)info.module_start_address == (uint32_t)&link_part1_flash_start)
            && ((uint32_t)info.module_end_address <= (uint32_t)&link_part1_flash_end)
            && (info.platform_id == PLATFORM_ID)) {
        uint32_t length = (uint32_t)info.module_end_address - (uint32_t)info.module_start_address;
        uint32_t crc = Compute_CRC32((const uint8_t*)info.module_start_address, length, nullptr);
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)((uint32_t)info.module_start_address + length)));
        return (crc == expectedCRC);
    }
    return false;
}

extern "C" int main() {
    /*
     * FIXME: Do NOT allocate memory from heap in MBR, since the heap start address is incorrect!
     * As a workaround, we can use AtomicSimpleStaticPool instead.
     */
    if (!bootloaderUpdateIfPending()) {
        NVIC_SystemReset();
    }

    if (!isPart1ImageValid()) {
        while (true) {
            __WFE();
        }
    }

    // dynalib table point to flash
    dynalib_table_location = &link_part1_dynalib_table_flash_start;
    bootloader_part1_preinit();

    // dynalib table point to SRAM
    dynalib_table_location = &link_part1_dynalib_table_ram_start;
    bootloader_part1_init();

    bootloader_part1_setup();

    while (true) {
        __SEV(); // signal event, also signal to KM4
        __WFE(); // clear event, immediately exits
        __WFE(); // sleep, waiting for event

        bootloader_part1_loop();
    }

    return 0;
}
