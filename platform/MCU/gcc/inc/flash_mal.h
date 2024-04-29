/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "platform_config.h"

#ifdef USE_SERIAL_FLASH

#define EXTERNAL_FLASH_SIZE (sFLASH_PAGESIZE * sFLASH_PAGECOUNT)

#define EXTERNAL_FLASH_ASSET_STORAGE_FIRST_PAGE (sFLASH_ASSET_STORAGE_FIRST_PAGE)
#define EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT (sFLASH_ASSET_STORAGE_PAGE_COUNT)

#endif // defined(USE_SERIAL_FLASH)

inline int FLASH_ModuleInfo(module_info_t* const infoOut, uint8_t device, uint32_t address, uint32_t* offset) {
    return -1;
}
