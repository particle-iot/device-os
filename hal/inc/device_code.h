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

#include <stdint.h>
#include <stddef.h>
#include "deviceid_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Variable String.
 */
typedef struct device_code_t {
    uint8_t length;
    uint8_t value[32];
} device_code_t;

// XXX: This constant is obtained from dct.h
#define HAL_SETUP_CODE_SIZE 6

/**
 * Appends the device code to the end of the existing value. The length field must be set to the length of
 * the current value.
 */
bool fetch_or_generate_device_code(device_code_t* value);
bool fetch_or_generate_setup_ssid(device_code_t* value);

/**
 * This function is provided externally by the device HAL since the prefix is specific to the platform.
 */
extern bool fetch_or_generate_ssid_prefix(device_code_t* value);

int get_device_name(char* buf, size_t size);

int get_device_setup_code(char* code, size_t size);

#ifdef __cplusplus
}
#endif
