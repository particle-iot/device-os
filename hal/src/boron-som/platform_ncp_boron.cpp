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

#include "exflash_hal.h"

#include "dct.h"

namespace {

const uintptr_t NCP_ID_OTP_ADDRESS = 0x00000020;

bool isValidNcpId(uint8_t id) {
    switch (id) {
    case MeshNCPIdentifier::MESH_NCP_SARA_U201:
    case MeshNCPIdentifier::MESH_NCP_SARA_G350:
    case MeshNCPIdentifier::MESH_NCP_SARA_R410:
        return true;
    default:
        return false;
    }
}

} // unnamed

MeshNCPIdentifier platform_current_ncp_identifier() {
    // Check the DCT
    uint8_t ncpId = 0;
    int r = dct_read_app_data_copy(DCT_NCP_ID_OFFSET, &ncpId, 1);
    if (r < 0 || !isValidNcpId(ncpId)) {
        // Check the OTP flash
        r = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, NCP_ID_OTP_ADDRESS, &ncpId, 1);
        if (r < 0 || !isValidNcpId(ncpId)) {
            ncpId = MeshNCPIdentifier::MESH_NCP_UNKNOWN;
        }
    }
    return (MeshNCPIdentifier)ncpId;
}
