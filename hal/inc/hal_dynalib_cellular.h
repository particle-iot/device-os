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

#ifndef HAL_DYNALIB_CELLULAR_H
#define	HAL_DYNALIB_CELLULAR_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "cellular_hal.h"
#include "inet_hal.h"
#endif

DYNALIB_BEGIN(hal_cellular)
DYNALIB_FN(hal_cellular, cellular_off, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_on, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_init, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_register, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_pdp_activate, cellular_result_t(CellularCredentials*, void*))
DYNALIB_FN(hal_cellular, cellular_pdp_deactivate, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_gprs_attach, cellular_result_t(CellularCredentials*, void*))
DYNALIB_FN(hal_cellular, cellular_gprs_detach, cellular_result_t(void*))
DYNALIB_FN(hal_cellular, cellular_fetch_ipconfig, cellular_result_t(WLanConfig*, void*))
DYNALIB_FN(hal_cellular, cellular_device_info, cellular_result_t(CellularDevice*, void*))
DYNALIB_FN(hal_cellular, cellular_credentials_set, cellular_result_t(const char*, const char*, const char*, void*))
DYNALIB_FN(hal_cellular, cellular_credentials_get, CellularCredentials*(void*))
DYNALIB_FN(hal_cellular, cellular_sim_ready, bool(void*))
DYNALIB_FN(hal_cellular, cellular_cancel, void(bool, bool, void*))
DYNALIB_FN(hal_cellular, HAL_NET_SetNetWatchDog, uint32_t(uint32_t))
DYNALIB_FN(hal_cellular, inet_gethostbyname, int(const char*, uint16_t, HAL_IPAddress*, network_interface_t, void*))
DYNALIB_FN(hal_cellular, inet_ping, int(const HAL_IPAddress*, network_interface_t, uint8_t, void*))
DYNALIB_FN(hal_cellular, cellular_signal, cellular_result_t(CellularSignalHal&, void*))
DYNALIB_FN(hal_cellular, cellular_command, cellular_result_t(_CALLBACKPTR_MDM, void*, system_tick_t, const char*, ...))

DYNALIB_END(hal_cellular)


#endif
