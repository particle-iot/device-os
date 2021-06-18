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

#include "platform_ncp.h"

PlatformNCPIdentifier platform_ncp_identifier(module_info_t* mi) {
    PlatformNCPIdentifier ncp = PLATFORM_NCP_UNKNOWN;
    if (mi->platform_id == PLATFORM_ID) {
        switch (mi->reserved) {
        case PLATFORM_NCP_ESP32:
        case PLATFORM_NCP_SARA_U201:
        case PLATFORM_NCP_SARA_G350:
        case PLATFORM_NCP_SARA_R410:
        case PLATFORM_NCP_SARA_R510:
        case PLATFORM_NCP_QUECTEL_BG96:
        case PLATFORM_NCP_QUECTEL_EG91_E:
        case PLATFORM_NCP_QUECTEL_EG91_NA:
        case PLATFORM_NCP_QUECTEL_EG91_EX:
        case PLATFORM_NCP_SARA_U260:
        case PLATFORM_NCP_SARA_U270:
        case PLATFORM_NCP_BROADCOM_BCM9WCDUSI09:
        case PLATFORM_NCP_BROADCOM_BCM9WCDUSI14:
            ncp = static_cast<PlatformNCPIdentifier>(mi->reserved);
            break;
        }
    }
    return ncp;
}

int platform_ncp_count() {
    return HAL_PLATFORM_NCP_COUNT;
}
