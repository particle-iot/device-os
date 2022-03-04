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

#define KM0_BOOTLOADER_UPDATE_MAGIC_NUMBER      0x20211116

#define KM0_BOOTLOADER_UPDATE_INFO_OFFSET       0

typedef struct {
    uint32_t magic_num;
    uint32_t src_addr;
    uint32_t dest_addr;
    uint32_t size;
    uint32_t flags; // Copy of platform_flash_modules_t.flags field which is copied from DCT, which is converted from module_info_t.flags
    uint32_t crc32; /* of this struct */
} flash_update_info_t;
