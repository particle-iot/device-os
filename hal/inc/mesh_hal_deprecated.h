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

#pragma once

#include "radio_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Select the antenna for mesh radio.
 *
 * @param antenna Antenna type (a value defined by the `radio_antenna_type` enum).
 *
 * @return `0` on success or a negative result code in case of an error.
 */
int mesh_select_antenna_deprecated(int antenna, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
