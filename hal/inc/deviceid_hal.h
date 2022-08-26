/**
 ******************************************************************************
 * @file    deviceid_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef DEVICEID_HAL_H
#define	DEVICEID_HAL_H

#include <stdint.h>
#include <stddef.h>

// Size of the device ID in the binary form
#define HAL_DEVICE_ID_SIZE 12

// Size of the device's serial number
#define HAL_DEVICE_SERIAL_NUMBER_SIZE 15

// Size of the device secret data
#define HAL_DEVICE_SECRET_SIZE 15

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Fetches the unique ID for this device.
 *
 * @param dest      The buffer to receive the device ID
 * @param destLen   The maximum length of the buffer. Can be 0.
 * @return          The number of bytes in the device ID. This is independent
 *                  of the buffer size.
 *
 * To find out the size of the device ID, call this method with
 * {@code destLen}==0.
 */
unsigned hal_get_device_id(uint8_t* dest, unsigned destLen);

/**
 * Retrievse the platform ID of this device.
 */
unsigned hal_get_platform_id();

/**
 *
 */
int HAL_Get_Device_Identifier(const char** name, char* buf, size_t buflen, unsigned index, void* reserved);

/**
 * Save the device ID to persistent storage so it can be retrieved via DCT. This is
 * done automatically by the system on startup.
 */
void hal_save_device_id(uint32_t offset);

/**
 * Get the device's serial number.
 */
int hal_get_device_serial_number(char* str, size_t size, void* reserved);

/**
 * Get the device secret data.
 */
int hal_get_device_secret(char* data, size_t size, void* reserved);

/**
 * Set the device's secret.
 */
int hal_set_device_secret(char* data, size_t size, void* reserved);

/**
 * Get the device's hardware version.
 */
int hal_get_device_hw_version(uint32_t* revision, void* reserved);

/**
 * Get the device's hardware model number and variant.
 */
int hal_get_device_hw_model(uint32_t* model, uint32_t* variant, void* reserved);

#define HAL_DEVICE_MAX_NCP_IDENTIFIERS (5)

typedef struct hal_device_hw_info {
    uint16_t size;
    uint16_t version;

    uint32_t revision;
    uint32_t model;
    uint32_t variant;
    uint32_t features;
    uint16_t ncp[HAL_DEVICE_MAX_NCP_IDENTIFIERS];
} hal_device_hw_info;

/**
 * Get the device's hardware information
 */
int hal_get_device_hw_info(hal_device_hw_info* info, void* reserved);

#include "deviceid_hal_compat.h"

#include "deviceid_hal_impl.h"

#ifdef	__cplusplus
}
#endif

#endif	/* DEVICEID_HAL_H */

