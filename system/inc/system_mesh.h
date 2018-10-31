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

#if HAL_PLATFORM_OPENTHREAD

/**
 * Size of a network ID.
 */
#define MESH_NETWORK_ID_SIZE 24
/**
 * Maximum size of a network name.
 */
#define MESH_MAX_NETWORK_NAME_SIZE 16
/**
 * Size of an extended PAN ID.
 */
#define MESH_EXT_PAN_ID_SIZE 8

#endif // HAL_PLATFORM_OPENTHREAD

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Device role.
 */
typedef enum mesh_device_role {
    MESH_DEVICE_ROLE_DEFAULT = 0, ///< The role is determined by the system.
    MESH_DEVICE_ROLE_DISABLED = 1, ///< Mesh networking is disabled.
    MESH_DEVICE_ROLE_DETACHED = 2, ///< Detached from the network.
    MESH_DEVICE_ROLE_ENDPOINT = 3, ///< Endpoint device.
    MESH_DEVICE_ROLE_REPEATER = 4, ///< Repeater device.
    MESH_DEVICE_ROLE_GATEWAY = 5 ///< Gateway device.
} mesh_device_role;

/**
 * Device type.
 */
typedef enum mesh_device_type {
    /**
     * Device type is determined by the system.
     */
    MESH_DEVICE_TYPE_DEFAULT = 0,
    /**
     * Full endpoint device.
     *
     * Device of this type can interact with multiple repeaters of the network.
     */
    MESH_DEVICE_TYPE_FULL = 1,
    /**
     * Minimal endpoint device.
     *
     * Device of this type interacts only with its parent repeater device.
     */
    MESH_DEVICE_TYPE_MINIMAL = 2,
    /**
     * Sleepy endpoint device.
     *
     * Device of this type is normally disabled and wakes on occasion to poll for messages from its
     * parent repeater device.
     */
    MESH_DEVICE_TYPE_SLEEPY = 3
} mesh_device_type;

/**
 * Mesh network info.
 */
typedef struct mesh_network_info {
    uint8_t version; ///< API version number (see `MESH_API_VERSION`).
#if HAL_PLATFORM_OPENTHREAD
    uint8_t channel; ///< Channel number.
    uint16_t pan_id; ///< PAN ID.
    uint8_t ext_pan_id[MESH_EXT_PAN_ID_SIZE]; ///< Extended PAN ID.
    char name[MESH_MAX_NETWORK_NAME_SIZE + 1]; ///< Network name (null-terminated).
    char id[MESH_NETWORK_ID_SIZE + 1]; ///< Network ID (null-terminated).
#endif // HAL_PLATFORM_OPENTHREAD
} mesh_network_info;

/**
 * Set the device role.
 *
 * The changes will not be applied immediately. Whether the device is eligible for the specified
 * role depends on the implementation.
 *
 * @param role Device role (see `mesh_device_role`).
 * @param type Device type (see `mesh_device_type`).
 * @param flags Reserved argument. Should be set to 0.
 * @param reserved Reserved argument. Should be set to NULL.
 *
 * @return `0` on success, or a negative result code in case of an error.
 */
int mesh_set_device_role(int role, int type, unsigned flags, void* reserved);
/**
 * Get the device role.
 *
 * @param current_role[out] Effective device role (see `mesh_device_role`).
 * @param configured_role[out] Configured device role (see `mesh_device_role`).
 * @param type[out] Device type (see `mesh_device_type`).
 * @param reserved Reserved argument. Should be set to NULL.
 *
 * @return `0` on success, or a negative result code in case of an error.
 */
int mesh_get_device_role(int* current_role, int* configured_role, int* type, void* reserved);
/**
 * Get the network info.
 *
 * @param info[out] Network info.
 * @param reserved Reserved argument. Should be set to NULL.
 *
 * @return `0` on success, or a negative result code in case of an error.
 */
int mesh_get_network_info(mesh_network_info* info, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_PLATFORM_MESH
