/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#define HAL_DEVICE_MAC_ADDR_SIZE        6

#define HAL_DEVICE_MAC_WIFI_STA         0
#define HAL_DEVICE_MAC_BLE              1
#define HAL_DEVICE_MAC_WIFI_AP          2
#define HAL_DEVICE_MAC_ETHERNET         3

#define isSupportedMacType(type)        ((type) == HAL_DEVICE_MAC_WIFI_STA || \
                                         (type) == HAL_DEVICE_MAC_BLE || \
                                         (type) == HAL_DEVICE_MAC_WIFI_AP || \
                                         (type) == HAL_DEVICE_MAC_ETHERNET)

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Get the device's BLE MAC address.
 */
int hal_get_mac_address(uint8_t type, uint8_t* dest, size_t destLen, void* reserved);

#ifdef	__cplusplus
}
#endif
