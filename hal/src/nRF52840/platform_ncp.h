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

enum MeshNCPManufacturer {
	MESH_NCP_MANUFACTURER_UNKNOWN,
	MESH_NCP_MANUFACTURER_ESPRESSIF = 1,
	MESH_NCP_MANUFACTURER_UBLOX = 2
};

#define MESH_NCP_IDENTIFIER(mf,t) (t|(mf<<5))

enum MeshNCPIdentifier {
	MESH_NCP_UNKNOWN = -1,
	MESH_NCP_NONE = 0,
	MESH_NCP_ESP32 = MESH_NCP_IDENTIFIER(MESH_NCP_MANUFACTURER_ESPRESSIF, 1),
	MESH_NCP_SARA_U201 = MESH_NCP_IDENTIFIER(MESH_NCP_MANUFACTURER_UBLOX, 2),
	MESH_NCP_SARA_G350 = MESH_NCP_IDENTIFIER(MESH_NCP_MANUFACTURER_UBLOX, 3),
	MESH_NCP_SARA_R410 = MESH_NCP_IDENTIFIER(MESH_NCP_MANUFACTURER_UBLOX, 4)
};


/**
 * Determine the NCP identifier from the module info given.
 * Returns MESH_NCP_UNKNOWN when the module info does not represent an NCP module, or if the NCP type is unknown, or if the main platform does not match this device's platform.
 */
MeshNCPIdentifier platform_ncp_identifier(module_info_t* moduleInfo);

/**
 * Determine the NCP that is running on this platform.
 */
MeshNCPIdentifier platform_current_ncp_identifier();

#if HAL_PLATFORM_NCP_UPDATABLE
/**
 * Update the NCP firmware from the given module. The module has been validated for integrity and matching platform and dependencies checked.
 */
hal_update_complete_t platform_ncp_update_module(const hal_module_t* module);

/**
 * Augments the module info with data retrieved from the NCP.
 */
int platform_ncp_fetch_module_info(hal_system_info_t* sys_info, bool create);

#endif

