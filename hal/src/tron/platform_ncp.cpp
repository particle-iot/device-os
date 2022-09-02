/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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
#include "check.h"
#include "system_error.h"
#include "deviceid_hal.h"

PlatformNCPIdentifier platform_primary_ncp_identifier() {
    return PLATFORM_NCP_REALTEK_RTL872X;
}

int platform_ncp_get_info(int idx, PlatformNCPInfo* info) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(idx >= 0 && idx < platform_ncp_count(), SYSTEM_ERROR_INVALID_ARGUMENT);
    if (idx == 0) {
        info->identifier = PLATFORM_NCP_REALTEK_RTL872X;
        info->updatable = false;
    }
    return 0;
}
