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

#include "platform_ncp.h"
#include "flash_mal.h"
#include "system_error.h"

namespace {

const uintptr_t NCP_ID_OTP_ADDRESS = 0x00000020;

bool isValidNcpId(uint8_t id) {
    switch (id) {
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_U201:
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_G350:
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_R410:
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_U260:
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_U270:
    case PlatformNCPIdentifier::PLATFORM_NCP_SARA_R510:
        return true;
    default:
        return false;
    }
}

} // unnamed

PlatformNCPIdentifier platform_primary_ncp_identifier() {
    uint8_t otp_ncp_id;
    PlatformNCPIdentifier identifier = PlatformNCPIdentifier::PLATFORM_NCP_UNKNOWN;

    int ret = FLASH_ReadOTP(NCP_ID_OTP_ADDRESS, (uint8_t*)&otp_ncp_id, sizeof(otp_ncp_id));
    if (ret == 0 && isValidNcpId(otp_ncp_id)) {
        identifier = (PlatformNCPIdentifier)otp_ncp_id;
    }

    return identifier;
}

int platform_ncp_get_info(int idx, PlatformNCPInfo* info) {
    if (idx == 0 && info) {
        info->identifier = platform_primary_ncp_identifier();
        info->updatable = false;
        return 0;
    }

    return SYSTEM_ERROR_INVALID_ARGUMENT;
}
