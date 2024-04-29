/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

// Need to define some LED constants that are normally provided by platform_config.h,
// used by sparking_wiring_rgb.cpp
#define PARTICLE_LED_RED             PARTICLE_LED2
#define PARTICLE_LED_GREEN           PARTICLE_LED3
#define PARTICLE_LED_BLUE            PARTICLE_LED4

#define USE_SERIAL_FLASH

#ifdef USE_SERIAL_FLASH

#define sFLASH_PAGESIZE                     0x1000 /* 4096 bytes sector size that needs to be erased */
#define sFLASH_PAGECOUNT                    2048   /* 8MByte storage */
#define sFLASH_FILESYSTEM_PAGE_COUNT        512    /* 2MB */
#define sFLASH_FILESYSTEM_FIRST_PAGE        0
#define sFLASH_ASSET_STORAGE_PAGE_COUNT     512    /* 2MB */
#define sFLASH_ASSET_STORAGE_FIRST_PAGE     (sFLASH_FILESYSTEM_FIRST_PAGE + sFLASH_FILESYSTEM_PAGE_COUNT)

#endif // defined(USE_SERIAL_FLASH)

#endif /* __PLATFORM_CONFIG_H */
