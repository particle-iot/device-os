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
#include "rtl_header.h"

extern uintptr_t link_rtl_xip_start;
extern uintptr_t link_rtl_xip_size;
extern uintptr_t link_rtl_ram_copy_start;
extern uintptr_t link_rtl_ram_copy_size;

extern "C" void Reset_Handler(void);

__attribute__((used, section(".rtl_header_xip"))) const rtl_binary_header rtlHeader = {
    .signature_high = RTL_HEADER_SIGNATURE_HIGH,
    .signature_low = RTL_HEADER_SIGNATURE_LOW,
    .size = (uint32_t)&link_rtl_xip_size,
    .load_address = (uint32_t)&link_rtl_xip_start,
    .sboot_address = 0xffffffff,
    .reserved0 = 0xffffffff,
    .reserved1 = 0xffffffffffffffff
};

__attribute__((used, section(".rtl_header_ram"))) const rtl_binary_header rtlHeaderRam = {
    .signature_high = RTL_HEADER_SIGNATURE_HIGH,
    .signature_low = RTL_HEADER_SIGNATURE_LOW,
    .size = (uint32_t)&link_rtl_ram_copy_size,
    .load_address = (uint32_t)&link_rtl_ram_copy_start,
    .sboot_address = 0xffffffff,
    .reserved0 = 0xffffffff,
    .reserved1 = 0xffffffffffffffff
};

__attribute__((used)) const uint8_t blah[1024] = {0xff};

__attribute__((used, section(".rtl_header_ram_valid_pattern"))) const uint8_t rtlRamValidPattern[] = RTL_HEADER_RAM_VALID_PATTERN;

__attribute__((used, section(".rtl_header_ram_start_table"))) RAM_FUNCTION_START_TABLE rtlRamStartTable = {
    .RamStartFun = &Reset_Handler,
    .RamWakeupFun = &Reset_Handler,
    .RamPatchFun0 = &Reset_Handler,
    .RamPatchFun1 = &Reset_Handler,
    .RamPatchFun2 = &Reset_Handler,
    .FlashStartFun = &Reset_Handler,
    .Img1ValidCode = (uint32_t)rtlRamValidPattern,
    .ExportTable = (BOOT_EXPORT_SYMB_TABLE*)&blah
};
