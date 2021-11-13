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
#include "sleep_handler.h"

// TODO: update it
__attribute__((used)) void* dynalib_table_location = 0; // mbr dynalib location

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

__attribute__((section(".xip.text"), used)) int bootloader_part1_preinit(uintptr_t* mbr_dynalib_start) {
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

    dynalib_table_location = mbr_dynalib_start;
    return 0;
}

typedef void (*constructor_ptr_t)(void);
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;
#define link_constructors_size ((unsigned long)&link_constructors_end - (unsigned long)&link_constructors_location)

int bootloader_part1_init(void) {
    // invoke constructors
    int ctor_num;
    for (ctor_num = 0; ctor_num < (link_constructors_size / sizeof(constructor_ptr_t)); ctor_num++) {
        link_constructors_location[ctor_num]();
    }
    return 0;
}

int bootloader_part1_setup(void) {
    sleepInit();
    return 0;
}

int bootloader_part1_loop(void) {
    sleepProcess();
    return 0;
}


#define DYNALIB_EXPORT
#include "part1_preinit_dynalib.h"
#include "part1_dynalib.h"

__attribute__((externally_visible)) const void* const bootloader_part1_module[] = {
    DYNALIB_TABLE_NAME(part1_preinit),
    DYNALIB_TABLE_NAME(part1),
};
