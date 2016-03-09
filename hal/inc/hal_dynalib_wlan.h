/**
 ******************************************************************************
 * @file    hal_dynalib_wlan.h
 * @authors Matthew McGowan
 * @date    04 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef HAL_DYNALIB_WLAN_H
#define	HAL_DYNALIB_WLAN_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "wlan_hal.h"
#include "inet_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_wlan)
DYNALIB_FN(hal_wlan, wlan_connect_init, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_connect_finalize, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_reset_credentials_store_required, bool(void))
DYNALIB_FN(hal_wlan, wlan_reset_credentials_store, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_disconnect_now, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_activate, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_deactivate, wlan_result_t(void))
DYNALIB_FN(hal_wlan, wlan_connected_rssi, int(void))

DYNALIB_FN(hal_wlan, wlan_clear_credentials, int(void))
DYNALIB_FN(hal_wlan, wlan_has_credentials, int(void))
DYNALIB_FN(hal_wlan, wlan_set_credentials, int(WLanCredentials*))

DYNALIB_FN(hal_wlan, wlan_smart_config_init, void(void))
DYNALIB_FN(hal_wlan, wlan_smart_config_cleanup, void(void))
DYNALIB_FN(hal_wlan, wlan_smart_config_finalize, bool(void))

DYNALIB_FN(hal_wlan, wlan_set_error_count, void(uint32_t))
DYNALIB_FN(hal_wlan, wlan_fetch_ipconfig, void(WLanConfig*))
DYNALIB_FN(hal_wlan, wlan_setup, void(void))

DYNALIB_FN(hal_wlan, HAL_NET_SetNetWatchDog, uint32_t(uint32_t))
DYNALIB_FN(hal_wlan, inet_gethostbyname, int(const char*, uint16_t, HAL_IPAddress*, network_interface_t, void*))
DYNALIB_FN(hal_wlan, inet_ping, int(const HAL_IPAddress*, network_interface_t, uint8_t, void*))
DYNALIB_FN(hal_wlan, wlan_select_antenna, int(WLanSelectAntenna_TypeDef))
DYNALIB_FN(hal_wlan, wlan_set_ipaddress, void(const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, void*))
DYNALIB_FN(hal_wlan, wlan_set_ipaddress_source, void(IPAddressSource, bool, void*))
DYNALIB_FN(hal_wlan, wlan_scan, int(wlan_scan_result_t, void*))
DYNALIB_FN(hal_wlan, wlan_get_credentials, int(wlan_scan_result_t, void*))
DYNALIB_END(hal_wlan)

#endif	/* HAL_DYNALIB_WLAN_H */

