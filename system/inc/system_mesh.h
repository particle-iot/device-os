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

#include "hal_platform.h"

#if HAL_PLATFORM_MESH

#include <stdint.h>

/**
 * API version number.
 */
#define MESH_API_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mesh antenna type.
 */
typedef enum mesh_antenna_type {
    MESH_ANT_DEFAULT = 0,
    MESH_ANT_INTERNAL = 1,
    MESH_ANT_EXTERNAL = 2,
} mesh_antenna_type;


/**
 * Select the antenna for mesh radio.
 *
 * @param antenna The antenna type for mesh (see `mesh_antenna_type`).
 *
 * @return `0` on success, or a negative result code in case of an error.
 */
int mesh_select_antenna(mesh_antenna_type antenna);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_PLATFORM_MESH
