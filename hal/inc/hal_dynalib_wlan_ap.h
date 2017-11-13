/**
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#pragma once

#include "hal_platform.h"
#include "dynalib.h"

#if HAL_PLATFORM_WIFI_AP

#ifdef DYNALIB_EXPORT
#include "wlan_ap_hal.h"
#endif

DYNALIB_BEGIN(hal_wlan_ap)
DYNALIB_FN(0, hal_wlan_ap, wlan_ap_has_credentials, wlan_result_t(void*))
DYNALIB_FN(1, hal_wlan_ap, wlan_ap_set_credentials, wlan_result_t(WLanCredentials*,void*))
DYNALIB_FN(2, hal_wlan_ap, wlan_ap_get_credentials, wlan_result_t(WiFiAccessPoint*,void*))
DYNALIB_FN(3, hal_wlan_ap, wlan_ap_get_state, wlan_result_t(uint8_t*,void*))
DYNALIB_END(hal_wlan_ap)

#endif
