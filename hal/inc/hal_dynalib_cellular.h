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
#include "hal_platform.h"

#if PLATFORM_ID == 10 || HAL_PLATFORM_CELLULAR

#ifdef DYNALIB_EXPORT
#include "cellular_hal.h"
#include "inet_hal.h"
#include "net_hal.h"
#endif

DYNALIB_BEGIN(hal_cellular)

DYNALIB_FN(0, hal_cellular, cellular_off, cellular_result_t(void*))
DYNALIB_FN(1, hal_cellular, cellular_on, cellular_result_t(void*))
DYNALIB_FN(2, hal_cellular, cellular_init, cellular_result_t(void*))
DYNALIB_FN(3, hal_cellular, cellular_register, cellular_result_t(void*))
DYNALIB_FN(4, hal_cellular, cellular_pdp_activate, cellular_result_t(CellularCredentials*, void*))
DYNALIB_FN(5, hal_cellular, cellular_pdp_deactivate, cellular_result_t(void*))
DYNALIB_FN(6, hal_cellular, cellular_gprs_attach, cellular_result_t(CellularCredentials*, void*))
DYNALIB_FN(7, hal_cellular, cellular_gprs_detach, cellular_result_t(void*))
DYNALIB_FN(8, hal_cellular, cellular_fetch_ipconfig, cellular_result_t(CellularConfig*, void*))
DYNALIB_FN(9, hal_cellular, cellular_device_info, cellular_result_t(CellularDevice*, void*))
DYNALIB_FN(10, hal_cellular, cellular_credentials_set, cellular_result_t(const char*, const char*, const char*, void*))
DYNALIB_FN(11, hal_cellular, cellular_credentials_get, CellularCredentials*(void*))
DYNALIB_FN(12, hal_cellular, cellular_sim_ready, bool(void*))
DYNALIB_FN(13, hal_cellular, cellular_cancel, void(bool, bool, void*))
DYNALIB_FN(14, hal_cellular, HAL_NET_SetNetWatchDog, uint32_t(uint32_t))
DYNALIB_FN(15, hal_cellular, inet_gethostbyname, int(const char*, uint16_t, HAL_IPAddress*, network_interface_t, void*))
DYNALIB_FN(16, hal_cellular, inet_ping, int(const HAL_IPAddress*, network_interface_t, uint8_t, void*))
DYNALIB_FN(17, hal_cellular, cellular_signal, cellular_result_t(CellularSignalHal*, cellular_signal_t*))
DYNALIB_FN(18, hal_cellular, cellular_command, cellular_result_t(_CALLBACKPTR_MDM, void*, system_tick_t, const char*, ...))
DYNALIB_FN(19, hal_cellular, cellular_data_usage_set, cellular_result_t(CellularDataHal*,void*))
DYNALIB_FN(20, hal_cellular, cellular_data_usage_get, cellular_result_t(CellularDataHal*,void*))
DYNALIB_FN(21, hal_cellular, cellular_band_select_set, cellular_result_t(MDM_BandSelect* bands, void* reserved))
DYNALIB_FN(22, hal_cellular, cellular_band_select_get, cellular_result_t(MDM_BandSelect* bands, void* reserved))
DYNALIB_FN(23, hal_cellular, cellular_band_available_get, cellular_result_t(MDM_BandSelect* bands, void* reserved))
DYNALIB_FN(24, hal_cellular, cellular_sms_received_handler_set, cellular_result_t(_CELLULAR_SMS_CB_MDM cb, void* data, void* reserved))
DYNALIB_FN(25, hal_cellular, HAL_USART3_Handler_Impl, void(void*))
DYNALIB_FN(26, hal_cellular, HAL_NET_SetCallbacks, void(const HAL_NET_Callbacks*, void*))
DYNALIB_FN(27, hal_cellular, cellular_pause, cellular_result_t(void*))
DYNALIB_FN(28, hal_cellular, cellular_resume, cellular_result_t(void*))
DYNALIB_FN(29, hal_cellular, cellular_imsi_to_network_provider, cellular_result_t(void*))
DYNALIB_FN(30, hal_cellular, cellular_network_provider_data_get, CellularNetProvData(void*))
DYNALIB_FN(31, hal_cellular, cellular_lock, int(void*))
DYNALIB_FN(32, hal_cellular, cellular_unlock, void(void*))
DYNALIB_FN(33, hal_cellular, cellular_set_power_mode, void(int mode, void* reserved))

#if !HAL_PLATFORM_MESH
DYNALIB_FN(34, hal_cellular, cellular_connect, cellular_result_t(void*))
DYNALIB_FN(35, hal_cellular, cellular_disconnect, cellular_result_t(void*))
#define BASE_CELL_IDX 36 // Base index for all subsequent functions
#else // HAL_PLATFORM_MESH
DYNALIB_FN(34, hal_cellular, cellular_set_active_sim, cellular_result_t(int, void*))
DYNALIB_FN(35, hal_cellular, cellular_get_active_sim, cellular_result_t(int*, void*))
DYNALIB_FN(36, hal_cellular, cellular_credentials_clear, int(void*))
#define BASE_CELL_IDX 37 // Base index for all subsequent functions
#endif // !HAL_PLATFORM_MESH

DYNALIB_FN(BASE_CELL_IDX + 0, hal_cellular, cellular_global_identity, cellular_result_t(CellularGlobalIdentity*, void*))

DYNALIB_END(hal_cellular)

#endif  // PLATFORM_ID == 10 || HAL_PLATFORM_CELLULAR

#endif  // HAL_DYNALIB_CELLULAR_H
