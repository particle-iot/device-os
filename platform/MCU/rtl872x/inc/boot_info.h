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


#pragma once

#define BOOT_INFO_FLASH_START_ADDR              0x0005F000
#define BOOT_INFO_FLASH_XIP_START_ADDR          0x0805F000

#define KM0_UPDATE_MAGIC_NUMBER                 0x20211116

#define KM4_BOOTLOADER_UPDATE_INFO_OFFSET       0
#define KM0_PART1_UPDATE_INFO_OFFSET            16
#define TESTING_FIRMWARE_FLAG_OFFSET            32

typedef struct {
    uint32_t magic_num;
    uint32_t src_addr;
    uint32_t dest_addr;
    uint32_t size;
} flash_update_info_t;

typedef struct {
    uint8_t hw_tested;
    uint8_t reserved[3];
} mfg_flags;
