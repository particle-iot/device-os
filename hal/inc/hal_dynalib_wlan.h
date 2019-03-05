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
#include "hal_platform.h"

#if (PLATFORM_ID != 10 && PLATFORM_ID != 13 && PLATFORM_ID != 14 && PLATFORM_ID != 23 && PLATFORM_ID != 24) || HAL_PLATFORM_WIFI

#ifdef DYNALIB_EXPORT
#include "wlan_hal.h"
#include "inet_hal.h"
#include "softap_http.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_wlan)

DYNALIB_FN(0, hal_wlan, wlan_connect_init, wlan_result_t(void))
DYNALIB_FN(1, hal_wlan, wlan_connect_finalize, wlan_result_t(void))
DYNALIB_FN(2, hal_wlan, wlan_reset_credentials_store_required, bool(void))
DYNALIB_FN(3, hal_wlan, wlan_reset_credentials_store, wlan_result_t(void))
DYNALIB_FN(4, hal_wlan, wlan_disconnect_now, wlan_result_t(void))
DYNALIB_FN(5, hal_wlan, wlan_activate, wlan_result_t(void))
DYNALIB_FN(6, hal_wlan, wlan_deactivate, wlan_result_t(void))
DYNALIB_FN(7, hal_wlan, wlan_connected_rssi, int(void))

DYNALIB_FN(8, hal_wlan, wlan_clear_credentials, int(void))
DYNALIB_FN(9, hal_wlan, wlan_has_credentials, int(void))
DYNALIB_FN(10, hal_wlan, wlan_set_credentials, int(WLanCredentials*))

DYNALIB_FN(11, hal_wlan, wlan_smart_config_init, void(void))
DYNALIB_FN(12, hal_wlan, wlan_smart_config_cleanup, void(void))
DYNALIB_FN(13, hal_wlan, wlan_smart_config_finalize, bool(void))

DYNALIB_FN(14, hal_wlan, wlan_set_error_count, void(uint32_t))
DYNALIB_FN(15, hal_wlan, wlan_fetch_ipconfig, int(WLanConfig*))
DYNALIB_FN(16, hal_wlan, wlan_setup, void(void))

DYNALIB_FN(17, hal_wlan, HAL_NET_SetNetWatchDog, uint32_t(uint32_t))
DYNALIB_FN(18, hal_wlan, inet_gethostbyname, int(const char*, uint16_t, HAL_IPAddress*, network_interface_t, void*))
DYNALIB_FN(19, hal_wlan, inet_ping, int(const HAL_IPAddress*, network_interface_t, uint8_t, void*))
DYNALIB_FN(20, hal_wlan, wlan_select_antenna, int(WLanSelectAntenna_TypeDef))
DYNALIB_FN(21, hal_wlan, wlan_set_ipaddress, void(const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, const HAL_IPAddress*, void*))
DYNALIB_FN(22, hal_wlan, wlan_set_ipaddress_source, void(IPAddressSource, bool, void*))
DYNALIB_FN(23, hal_wlan, wlan_scan, int(wlan_scan_result_t, void*))
DYNALIB_FN(24, hal_wlan, wlan_get_credentials, int(wlan_scan_result_t, void*))
DYNALIB_FN(25, hal_wlan, softap_set_application_page_handler, int(PageProvider* provider, void* reserved))
DYNALIB_FN(26, hal_wlan, wlan_restart, int(void*))
DYNALIB_FN(27, hal_wlan, wlan_set_hostname, int(const char*, void*))
DYNALIB_FN(28, hal_wlan, wlan_get_hostname, int(char*, size_t, void*))
DYNALIB_FN(29, hal_wlan, wlan_connected_info, int(void*, wlan_connected_info_t*, void*))
DYNALIB_FN(30, hal_wlan, wlan_get_antenna, WLanSelectAntenna_TypeDef(void*))
DYNALIB_FN(31, hal_wlan, wlan_get_ipaddress, int(IPConfig*, void*))
DYNALIB_FN(32, hal_wlan, wlan_get_ipaddress_source, IPAddressSource(void*))
DYNALIB_END(hal_wlan)

#endif // (PLATFORM_ID != 10 && PLATFORM_ID != 13 && PLATFORM_ID != 14) || HAL_PLATFORM_WIFI

#endif	/* HAL_DYNALIB_WLAN_H */

