/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "module_info.h"
#include "ota_flash_hal.h"
#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum PlatformNCPManufacturer {
    PLATFORM_NCP_MANUFACTURER_UNKNOWN,
    PLATFORM_NCP_MANUFACTURER_ESPRESSIF = 1,
    PLATFORM_NCP_MANUFACTURER_UBLOX = 2,
    PLATFORM_NCP_MANUFACTURER_QUECTEL = 3,
    PLATFORM_NCP_MANUFACTURER_BROADCOM = 4,
    PLATFORM_NCP_MANUFACTURER_REALTEK = 5
} PlatformNCPManufacturer;

#define PLATFORM_NCP_IDENTIFIER(mf,t) (t|(mf<<5))
#define PLATFORM_NCP_MANUFACTURER(v) ((v) >> 5)

typedef enum PlatformNCPIdentifier {
    PLATFORM_NCP_UNKNOWN = 0xffff,
    PLATFORM_NCP_NONE = 0,

    // 0x2x
    PLATFORM_NCP_ESP32 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_ESPRESSIF, 1),

    // 0x4x
    PLATFORM_NCP_SARA_U201 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 2),
    PLATFORM_NCP_SARA_G350 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 3),
    PLATFORM_NCP_SARA_R410 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 4),
    PLATFORM_NCP_SARA_U260 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 5),
    PLATFORM_NCP_SARA_U270 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 6),
    PLATFORM_NCP_SARA_R510 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 7),

    // 0x6x
    PLATFORM_NCP_QUECTEL_BG96     = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 1),
    PLATFORM_NCP_QUECTEL_EG91_E   = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 2),
    PLATFORM_NCP_QUECTEL_EG91_NA  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 3),
    PLATFORM_NCP_QUECTEL_EG91_EX  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 4),
    PLATFORM_NCP_QUECTEL_BG95_M1  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 5),
    PLATFORM_NCP_QUECTEL_EG91_NAX = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 6),
    PLATFORM_NCP_QUECTEL_BG77     = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 7),
    PLATFORM_NCP_QUECTEL_BG95_MF  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 8),
    PLATFORM_NCP_QUECTEL_BG95_M6  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 9),
    PLATFORM_NCP_QUECTEL_BG95_M5  = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 10),

    // 0x8x
    PLATFORM_NCP_BROADCOM_BCM9WCDUSI09 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_BROADCOM, 1),
    PLATFORM_NCP_BROADCOM_BCM9WCDUSI14 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_BROADCOM, 2),
    PLATFORM_NCP_REALTEK_RTL872X = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_REALTEK, 0)
} PlatformNCPIdentifier;

#if PLATFORM_ID != PLATFORM_GCC
static_assert(sizeof(PlatformNCPIdentifier) == sizeof(uint16_t), "sizeof(Platform_NCPIdentifier) has changed");
#endif

struct PlatformNCPInfo {
    PlatformNCPIdentifier identifier;
    bool updatable;
};

/**
 * Determine the primary NCP that is running on this platform.
 */
PlatformNCPIdentifier platform_primary_ncp_identifier();

/**
 * Get total number of NCP that are available on this platform.
 */
int platform_ncp_count();

/**
 * Determine NCP type of a particular NCP on this platform.
 */
int platform_ncp_get_info(int idx, PlatformNCPInfo* info);

#if HAL_PLATFORM_NCP_UPDATABLE
/**
 * Update the NCP firmware from the given module. The module has been validated for integrity and matching platform and dependencies checked.
 */
int platform_ncp_update_module(const hal_module_t* module);

/**
 * Augments the module info with data retrieved from the NCP.
 */
int platform_ncp_fetch_module_info(hal_module_t* module);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
