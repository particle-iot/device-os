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

#include <cstdint>
#include "basic_types.h"

typedef struct rtl_binary_header {
    uint32_t signature_high;
    uint32_t signature_low;
    uint32_t size;
    uint32_t load_address;
    uint32_t sboot_address;
    uint32_t reserved0;
    uint64_t reserved1;
} rtl_binary_header;

extern uintptr_t link_rtl_header_size;
extern uintptr_t link_rtl_startup;
extern uintptr_t link_rtl_header_ram_size;

// __attribute__((used, section(".rtl_header"))) const rtl_binary_header rtl_header = {
//     .signature_high = 0x96969999,
//     .signature_low = 0xfc66cc3f,
//     .size = (uint32_t)&link_rtl_header_size,
//     .load_address = (uint32_t)&link_rtl_startup,
//     .sboot_address = 0xffffffff,
//     .reserved0 = 0xffffffff,
//     .reserved1 = 0xffffffffffffffff
// };

extern "C" void Reset_Handler(void);

void dummyFunc(void) {
    while(true) {
        ;
    }
}

__attribute__((used, section(".rtl_header"))) const rtl_binary_header rtlHeader = {
    // .signature_high = 0x35393138,
    // .signature_low = 0x31313738,
    .signature_high = 0x96969999,
    .signature_low = 0xfc66cc3f,
    .size = (uint32_t)&link_rtl_header_size,
    .load_address = (uint32_t)&link_rtl_startup,
    .sboot_address = 0xffffffff,
    .reserved0 = 0xffffffff,
    .reserved1 = 0xffffffffffffffff
};


__attribute__((used, section(".rtl_header_ram"))) const rtl_binary_header rtlHeaderRam = {
    // .signature_high = 0x35393138,
    // .signature_low = 0x31313738,
    .signature_high = 0x96969999,
    .signature_low = 0xfc66cc3f,
    .size = (uint32_t)&link_rtl_header_ram_size,
    .load_address = 0x1007d000,
    .sboot_address = 0xffffffff,
    .reserved0 = 0xffffffff,
    .reserved1 = 0xffffffffffffffff
};

__attribute__((used, section(".rtl_header_ram_boot_export_table"))) BOOT_EXPORT_SYMB_TABLE rtlBootExportTable = {
    .rdp_decrypt_func = nullptr,
    .psram_s_start_addr = 0x0
};


__attribute__((used, section(".rtl_header_ram_valid_pattern"))) const uint8_t rtlRamValidPattern[] = {
	0x23, 0x79, 0x16, 0x88, 0xff, 0xff, 0xff, 0xff
};

__attribute__((used, section(".rtl_header_ram_start_table"))) RAM_FUNCTION_START_TABLE rtlRamStartTable = {
    .RamStartFun = &dummyFunc,
    .RamWakeupFun = 0x0,
    .RamPatchFun0 = &dummyFunc,
    .RamPatchFun1 = &dummyFunc,
    .RamPatchFun2 = &dummyFunc,
    .FlashStartFun = &dummyFunc,
    .Img1ValidCode = (uint32_t)rtlRamValidPattern,
    .ExportTable = &rtlBootExportTable
};
