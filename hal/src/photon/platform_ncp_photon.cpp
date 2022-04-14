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

#include "platforms.h"
#include "platform_ncp.h"
#include "system_error.h"

PlatformNCPIdentifier platform_primary_ncp_identifier() {
    #if PLATFORM_ID == PLATFORM_PHOTON
        return PLATFORM_NCP_BROADCOM_BCM9WCDUSI09;
    #elif PLATFORM_ID == PLATFORM_P1
        return PLATFORM_NCP_BROADCOM_BCM9WCDUSI14;
    #endif
}

int platform_ncp_get_info(int idx, PlatformNCPInfo* info) {
    if (idx == 0 && info) {
        info->identifier = platform_primary_ncp_identifier();
        info->updatable = false;
        return SYSTEM_ERROR_NONE;
    }

    return SYSTEM_ERROR_INVALID_ARGUMENT;
}
