/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AT_DTM_HAL_H
#define AT_DTM_HAL_H

#include <stdint.h>
#include "hal_platform.h"

#if HAL_PLATFORM_AT_DTM

typedef enum hal_at_dtm_type_t {
    HAL_AT_DTM_TYPE_BLE,
    HAL_AT_DTM_TYPE_CELLULAR,
    HAL_AT_DTM_TYPE_WIFI,
    HAL_AT_DTM_TYPE_ETHERNET,
    HAL_AT_DTM_TYPE_GNSS,
    HAL_AT_DTM_TYPE_MAX
} hal_at_dtm_type_t;

typedef enum hal_at_dtm_interface_t {
    HAL_AT_DTM_INTERFACE_UART,
    HAL_AT_DTM_INTERFACE_SPI,
    HAL_AT_DTM_INTERFACE_I2C
} hal_at_dtm_interface_t;

typedef struct hal_at_dtm_interface_config_t {
    uint16_t size;
    uint16_t version;
    hal_at_dtm_interface_t interface;
    uint8_t index; // Interface index, e.g. UART 0/1/2/3
    union {
        uint16_t baudrate; // It's fixed to 19200 for now.
    } params;
} hal_at_dtm_interface_config_t;

#ifdef __cplusplus
extern "C" {
#endif

int hal_at_dtm_init(hal_at_dtm_type_t type, const hal_at_dtm_interface_config_t* config, void* reserved);

#ifdef __cplusplus
}
#endif

#endif // HAL_PLATFORM_AT_DTM

#endif // AT_DTM_HAL_H
