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

// TODO: Prefix all BLE HAL definitions with "hal_"
#define BLE_INVALID_CONN_HANDLE BLE_CONN_HANDLE_INVALID

// Maximum number of services per profile
#define BLE_MAX_SERVICE_COUNT 1

// Maximum number of characteristics per service
#define BLE_MAX_CHAR_COUNT 4
