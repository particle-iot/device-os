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

#include "ble_types.h" // For BLE_CONN_HANDLE_INVALID
#include "ble_gatt.h"

#include "sdk_config.h"

#if !defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) || (NRF_SDH_BLE_GATT_MAX_MTU_SIZE == 0)
#error "NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined"
#endif

// Invalid connection handle
// TODO: Prefix all BLE HAL definitions with "hal_"
#define BLE_INVALID_CONN_HANDLE BLE_CONN_HANDLE_INVALID

// Invalid attribute handle
#define BLE_INVALID_ATTR_HANDLE BLE_GATT_HANDLE_INVALID

// Maximum number of peripheral connections
#define BLE_MAX_PERIPH_CONN_COUNT NRF_SDH_BLE_PERIPHERAL_LINK_COUNT

// Maximum number of services per profile
#define BLE_MAX_SERVICE_COUNT 1

// Maximum number of characteristics per service
#define BLE_MAX_CHAR_COUNT 4

// Maximum supported size of an ATT packet in bytes (ATT_MTU)
#define BLE_MAX_ATT_MTU_SIZE NRF_SDH_BLE_GATT_MAX_MTU_SIZE
