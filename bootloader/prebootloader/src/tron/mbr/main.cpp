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
#include "rtl_support.h"
extern "C" {
#include "rtl8721d.h"
}
#include "module_info.h"

__attribute__((used)) void* dynalib_table_location = 0; // part1 dynalib location

extern "C" int bootloader_part1_preinit(void);
extern "C" int bootloader_part1_init(void);
extern "C" int bootloader_part1_postinit(void);

extern uintptr_t link_part1_flash_start;
extern uintptr_t link_part1_flash_end;
extern uintptr_t link_part1_module_info_flash_start;
extern uintptr_t link_part1_dynalib_table_flash_start;
extern uintptr_t link_part1_dynalib_table_ram_start;

extern "C" uint32_t computeCrc32(const uint8_t *address, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    while (length > 0) {
        crc ^= *address++;
        for (uint8_t i = 0; i < 8; i++) {
            uint32_t mask = ~((crc & 1) - 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        length--;
    }
    return ~crc;
}

extern "C" bool isPart1ImageValid() {
    module_info_t info = {};
    _memcpy(&info, &link_part1_module_info_flash_start, sizeof(module_info_t));
    if (((uint32_t)info.module_start_address == (uint32_t)&link_part1_flash_start)
            && ((uint32_t)info.module_end_address <= (uint32_t)&link_part1_flash_end)
            && (info.platform_id == PLATFORM_ID)) {
        uint32_t length = (uint32_t)info.module_end_address - (uint32_t)info.module_start_address;
        uint32_t crc = computeCrc32((const uint8_t*)info.module_start_address, length);
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)((uint32_t)info.module_start_address + length)));
        return (crc == expectedCRC);
    }
    return false;
}

extern "C" int main() {
    /*
     * FIXME: Do NOT allocate memory from heap in MBR, since the heap start address is incorrect!
     * As a workaround, we can export run time APIs in part1.
     */

    rtlLowLevelInit();
    rtlPmuInit();

    rtlPowerOnBigCore();

    if (isPart1ImageValid()) {
        dynalib_table_location = &link_part1_dynalib_table_flash_start; // dynalib table point to flash

        // bootloader_part1_preinit() is executed in XIP
        if (bootloader_part1_preinit() < 0) {
            DiagPrintf("KM0 part1 preinit failed\n");
        }

        dynalib_table_location = &link_part1_dynalib_table_ram_start; // dynalib table point to SRAM

        if (bootloader_part1_init() < 0) {
            DiagPrintf("KM0 part1 init failed\n");
        }
        if (bootloader_part1_postinit() < 0) {
            DiagPrintf("KM0 part1 init failed\n");
        }
    }

    while (true) {
        __WFE();
    }

    return 0;
}
