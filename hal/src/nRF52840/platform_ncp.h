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

enum MeshNCPModemIdentifier {
	UNKNOWN,
	ESP32 = 1,
	SARA_U201 = 2,
	SARA_G350 = 3,
	SARA_R410 = 4
};

/**
 * Determine the modem identifier from the module info given.
 * Returns UNKNOWN when the module info does not represent an NCP module, or if the NCP type is unknown, or if the main platform does not match this device's platform.
 */
MeshNCPModemIdentifier platform_ncp_modem_identifier(module_info_t* moduleInfo);
