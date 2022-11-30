/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#include "exflash_hal.h"
#include "dct.h"
#include "system_error.h"

namespace {

const uintptr_t NCP_ID_OTP_ADDRESS = 0x00000020;

bool isValidNcpId(uint8_t id) {
    switch (id) {
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG96:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_EG91_E:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_EG91_NA:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_EG91_EX:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG95_M1:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG95_MF:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_EG91_NAX:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG77:
    case PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG95_M6:
        return true;
    default:
        return false;
    }
}

} // unnamed

PlatformNCPIdentifier platform_primary_ncp_identifier() {
    // Check the DCT
    uint8_t ncpId = 0;
    int r = dct_read_app_data_copy(DCT_NCP_ID_OFFSET, &ncpId, sizeof(ncpId));
    if (r < 0 || !isValidNcpId(ncpId)) {
        // Check the OTP flash
        r = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, NCP_ID_OTP_ADDRESS, &ncpId, sizeof(ncpId));
        if (r < 0 || !isValidNcpId(ncpId)) {
            return PlatformNCPIdentifier::PLATFORM_NCP_UNKNOWN;
        }
    }
    return (PlatformNCPIdentifier)ncpId;
}

int platform_ncp_get_info(int idx, PlatformNCPInfo* info) {
    if (idx == 0 && info) {
        info->identifier = platform_primary_ncp_identifier();
        info->updatable = false;
        return 0;
    }

    return SYSTEM_ERROR_INVALID_ARGUMENT;
}