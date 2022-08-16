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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "rtl8721d.h"
#include "rtl_support.h"
#include "km0_km4_ipc.h"
#include "sleep_handler.h"
#include "update_handler.h"

extern uintptr_t link_dynalib_flash_start;
extern uintptr_t link_dynalib_start;
extern uintptr_t link_dynalib_end;
#define link_dynalib_table_size ((uintptr_t)&link_dynalib_end - (uintptr_t)&link_dynalib_start)

extern uintptr_t link_bss_location;
extern uintptr_t link_bss_end;
#define link_bss_size ((uintptr_t)&link_bss_end - (uintptr_t)&link_bss_location)

extern uintptr_t link_ram_copy_flash_start;
extern uintptr_t link_ram_copy_start;
extern uintptr_t link_ram_copy_end;
#define link_ram_copy_size ((uintptr_t)&link_ram_copy_end - (uintptr_t)&link_ram_copy_start)

__attribute__((section(".xip.text"), used)) int bootloader_part1_preinit(void) {
    // Copy the dynalib table to SRAM
    if ((&link_dynalib_start != &link_dynalib_flash_start) && (link_dynalib_table_size != 0)) {
        _memcpy(&link_dynalib_start, &link_dynalib_flash_start, link_dynalib_table_size);
    }
    // Initialize .bss
    _memset(&link_bss_location, 0, link_bss_size );
    // Copy RAM code and static data
    if ((&link_ram_copy_start != &link_ram_copy_flash_start) && (link_ram_copy_size != 0)) {
        _memcpy(&link_ram_copy_start, &link_ram_copy_flash_start, link_ram_copy_size);
    }
    return 0;
}

typedef void (*constructor_ptr_t)(void);
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;
#define link_constructors_size ((unsigned long)&link_constructors_end - (unsigned long)&link_constructors_location)

int bootloader_part1_init(void) {
    rtlLowLevelInit();
    rtlPmuInit();

    rtlPowerOnBigCore();

    km0_km4_ipc_init(KM0_KM4_IPC_CHANNEL_GENERIC);

    // invoke constructors
    // It might get IPC involed, so it needs to be called after KM4 is powered on.
    unsigned int ctor_num;
    for (ctor_num = 0; ctor_num < (link_constructors_size / sizeof(constructor_ptr_t)); ctor_num++) {
        link_constructors_location[ctor_num]();
    }
    return 0;
}

int bootloader_part1_setup(void) {
    sleepInit();
    bootloaderUpdateInit();
    return 0;
}

int bootloader_part1_loop(void) {
    sleepProcess();
    bootloaderUpdateProcess();
    return 0;
}


#define DYNALIB_EXPORT
#include "part1_preinit_dynalib.h"
#include "part1_dynalib.h"

__attribute__((externally_visible)) const void* const bootloader_part1_module[] = {
    DYNALIB_TABLE_NAME(part1_preinit),
    DYNALIB_TABLE_NAME(part1),
};
