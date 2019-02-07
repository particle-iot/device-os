/**
 ******************************************************************************
 * @file    hal_dynalib_core.h
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

#ifndef HAL_DYNALIB_CORE_H
#define	HAL_DYNALIB_CORE_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "core_hal.h"
#include "deviceid_hal.h"
#include "syshealth_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_core)

DYNALIB_FN(0, hal_core, HAL_core_subsystem_version, int(char*, int))
DYNALIB_FN(1, hal_core, HAL_Core_Init, void(void))
DYNALIB_FN(2, hal_core, HAL_Core_Config, void(void))
DYNALIB_FN(3, hal_core, HAL_Core_Mode_Button_Pressed, bool(uint16_t))
DYNALIB_FN(4, hal_core, HAL_Core_Mode_Button_Reset, void(uint16_t))
DYNALIB_FN(5, hal_core, HAL_Core_System_Reset, void(void))
DYNALIB_FN(6, hal_core, HAL_Core_Factory_Reset, void(void))
DYNALIB_FN(7, hal_core, HAL_Core_Enter_Bootloader, void(bool))
DYNALIB_FN(8, hal_core, HAL_Core_Enter_Stop_Mode, void(uint16_t, uint16_t, long))
DYNALIB_FN(9, hal_core, HAL_Core_Execute_Stop_Mode, void(void))
DYNALIB_FN(10, hal_core, HAL_Core_Enter_Standby_Mode, int(uint32_t, uint32_t))
DYNALIB_FN(11, hal_core, HAL_Core_Execute_Standby_Mode, void(void))
DYNALIB_FN(12, hal_core, HAL_Core_Compute_CRC32, uint32_t(const uint8_t*, uint32_t))
DYNALIB_FN(13, hal_core, HAL_device_ID, unsigned(uint8_t*, unsigned))

DYNALIB_FN(14, hal_core, HAL_Get_Sys_Health, eSystemHealth(void))
DYNALIB_FN(15, hal_core, HAL_Set_Sys_Health, void(eSystemHealth))
DYNALIB_FN(16, hal_core, HAL_watchdog_reset_flagged, bool(void))
DYNALIB_FN(17, hal_core, HAL_Notify_WDT, void(void))
DYNALIB_FN(18, hal_core, HAL_Bootloader_Get_Flag, uint16_t(BootloaderFlag))
DYNALIB_FN(19, hal_core, HAL_Bootloader_Lock, void(bool))
DYNALIB_FN(20, hal_core, HAL_Core_System_Reset_FlagSet, bool(RESET_TypeDef))
DYNALIB_FN(21, hal_core, HAL_Core_Runtime_Info, uint32_t(runtime_info_t*, void*))
DYNALIB_FN(22, hal_core, HAL_Set_System_Config, int(hal_system_config_t, const void*, unsigned))
DYNALIB_FN(23, hal_core, HAL_Core_Enter_Safe_Mode, void(void*))
DYNALIB_FN(24, hal_core, HAL_Feature_Get, bool(HAL_Feature))
DYNALIB_FN(25, hal_core, HAL_Feature_Set, int(HAL_Feature, bool))
DYNALIB_FN(26, hal_core, HAL_Core_System_Reset_Ex, void(int, uint32_t, void*))
DYNALIB_FN(27, hal_core, HAL_Core_Get_Last_Reset_Info, int(int*, uint32_t*, void*))
DYNALIB_FN(28, hal_core, HAL_Core_Button_Mirror_Pin, void(uint16_t, InterruptMode, uint8_t, uint8_t, void*))
DYNALIB_FN(29, hal_core, HAL_Core_Button_Mirror_Pin_Disable, void(uint8_t, uint8_t, void*))
DYNALIB_FN(30, hal_core, HAL_Core_Led_Mirror_Pin, void(uint8_t, pin_t, uint32_t, uint8_t, void*))
DYNALIB_FN(31, hal_core, HAL_Core_Led_Mirror_Pin_Disable, void(uint8_t, uint8_t, void*))

DYNALIB_FN(32, hal_core, HAL_Set_Event_Callback, void(HAL_Event_Callback, void*))
DYNALIB_FN(33, hal_core, HAL_Core_Enter_Stop_Mode_Ext, int(const uint16_t*, size_t, const InterruptMode*, size_t, long, void*))
DYNALIB_FN(34, hal_core, HAL_Core_Execute_Standby_Mode_Ext, int(uint32_t, void*))

DYNALIB_END(hal_core)

#endif	/* HAL_DYNALIB_CORE_H */
