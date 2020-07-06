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

enum PlatformNCPManufacturer {
    PLATFORM_NCP_MANUFACTURER_UNKNOWN,
    PLATFORM_NCP_MANUFACTURER_ESPRESSIF = 1,
    PLATFORM_NCP_MANUFACTURER_UBLOX = 2,
    PLATFORM_NCP_MANUFACTURER_QUECTEL = 3,
    PLATFORM_NCP_MANUFACTURER_BROADCOM = 4
};

#define PLATFORM_NCP_IDENTIFIER(mf,t) (t|(mf<<5))
#define PLATFORM_NCP_MANUFACTURER(v) ((v) >> 5)

enum PlatformNCPIdentifier {
    PLATFORM_NCP_UNKNOWN = -1,
    PLATFORM_NCP_NONE = 0,
    PLATFORM_NCP_ESP32 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_ESPRESSIF, 1),
    PLATFORM_NCP_SARA_U201 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 2),
    PLATFORM_NCP_SARA_G350 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 3),
    PLATFORM_NCP_SARA_R410 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 4),
    PLATFORM_NCP_SARA_U260 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 5),
    PLATFORM_NCP_SARA_U270 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_UBLOX, 6),
    PLATFORM_NCP_QUECTEL_BG96 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 1),
    PLATFORM_NCP_QUECTEL_EG91_E = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 2),
    PLATFORM_NCP_QUECTEL_EG91_NA = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 3),
    PLATFORM_NCP_QUECTEL_EG91_EX = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_QUECTEL, 4),
    PLATFORM_NCP_BROADCOM_BCM9WCDUSI09 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_BROADCOM, 1),
    PLATFORM_NCP_BROADCOM_BCM9WCDUSI14 = PLATFORM_NCP_IDENTIFIER(PLATFORM_NCP_MANUFACTURER_BROADCOM, 2)
};


/**
 * Determine the NCP identifier from the module info given.
 * Returns PLATFORM_NCP_UNKNOWN when the module info does not represent an NCP module, or if the NCP type is unknown, or if the main platform does not match this device's platform.
 */
PlatformNCPIdentifier platform_ncp_identifier(module_info_t* moduleInfo);

/**
 * Determine the NCP that is running on this platform.
 */
PlatformNCPIdentifier platform_current_ncp_identifier();

#if HAL_PLATFORM_NCP_UPDATABLE
/**
 * Update the NCP firmware from the given module. The module has been validated for integrity and matching platform and dependencies checked.
 */
int platform_ncp_update_module(const hal_module_t* module);

/**
 * Augments the module info with data retrieved from the NCP.
 */
int platform_ncp_fetch_module_info(hal_system_info_t* sys_info, bool create);

#endif

