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

#include "cellular_enums_hal.h"
#include "platform_ncp.h"

inline Dev cellular_dev_from_ncp(PlatformNCPIdentifier identifer) {
    Dev device = DEV_UNKNOWN;
    switch (identifer) {
    case PLATFORM_NCP_SARA_U201:
        device = DEV_SARA_U201;
        break;
    case PLATFORM_NCP_SARA_G350:
        device = DEV_SARA_G350;
        break;
    case PLATFORM_NCP_SARA_R410:
        device = DEV_SARA_R410;
        break;
    case PLATFORM_NCP_SARA_R510:
        device = DEV_SARA_R510;
        break;
    case PLATFORM_NCP_QUECTEL_BG96:
        device = DEV_QUECTEL_BG96;
        break;
    case PLATFORM_NCP_QUECTEL_EG91_E:
        device = DEV_QUECTEL_EG91_E;
        break;
    case PLATFORM_NCP_QUECTEL_EG91_NA:
        device = DEV_QUECTEL_EG91_NA;
        break;
    case PLATFORM_NCP_QUECTEL_EG91_EX:
        device = DEV_QUECTEL_EG91_EX;
        break;
    case PLATFORM_NCP_QUECTEL_BG95_M1:
        device = DEV_QUECTEL_BG95_M1;
        break;
    case PLATFORM_NCP_QUECTEL_BG95_MF:
        device = DEV_QUECTEL_BG95_MF;
        break;
    case PLATFORM_NCP_QUECTEL_EG91_NAX:
        device = DEV_QUECTEL_EG91_NAX;
        break;
    case PLATFORM_NCP_QUECTEL_BG77:
        device = DEV_QUECTEL_BG77;
        break;
    case PLATFORM_NCP_QUECTEL_BG95_M6:
        device = DEV_QUECTEL_BG95_M6;
        break;
    case PLATFORM_NCP_QUECTEL_BG95_M5:
        device = DEV_QUECTEL_BG95_M5;
        break;
    default:
        device = DEV_UNKNOWN;
        break;
    }
    return device;
}
