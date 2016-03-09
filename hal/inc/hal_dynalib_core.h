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
DYNALIB_FN(hal_core, HAL_core_subsystem_version, int(char*, int))
DYNALIB_FN(hal_core, HAL_Core_Init, void(void))
DYNALIB_FN(hal_core, HAL_Core_Config, void(void))
DYNALIB_FN(hal_core, HAL_Core_Mode_Button_Pressed, bool(uint16_t))
DYNALIB_FN(hal_core, HAL_Core_Mode_Button_Reset, void(void))
DYNALIB_FN(hal_core, HAL_Core_System_Reset, void(void))
DYNALIB_FN(hal_core, HAL_Core_Factory_Reset, void(void))
DYNALIB_FN(hal_core, HAL_Core_Enter_Bootloader, void(bool))
DYNALIB_FN(hal_core, HAL_Core_Enter_Stop_Mode, void(uint16_t, uint16_t, long))
DYNALIB_FN(hal_core, HAL_Core_Execute_Stop_Mode, void(void))
DYNALIB_FN(hal_core, HAL_Core_Enter_Standby_Mode, void(void))
DYNALIB_FN(hal_core, HAL_Core_Execute_Standby_Mode, void(void))
DYNALIB_FN(hal_core, HAL_Core_Compute_CRC32, uint32_t(const uint8_t*, uint32_t))
DYNALIB_FN(hal_core, HAL_device_ID, unsigned(uint8_t*, unsigned))

DYNALIB_FN(hal_core, HAL_Get_Sys_Health, eSystemHealth(void))
DYNALIB_FN(hal_core, HAL_Set_Sys_Health, void(eSystemHealth))
DYNALIB_FN(hal_core, HAL_watchdog_reset_flagged, bool(void))
DYNALIB_FN(hal_core, HAL_Notify_WDT, void(void))
DYNALIB_FN(hal_core, HAL_Bootloader_Get_Flag, uint16_t(BootloaderFlag))
DYNALIB_FN(hal_core, HAL_Bootloader_Lock, void(bool))
DYNALIB_FN(hal_core, HAL_Core_System_Reset_FlagSet, bool(RESET_TypeDef))
DYNALIB_FN(hal_core, HAL_Core_Runtime_Info, uint32_t(runtime_info_t*, void*))
DYNALIB_FN(hal_core, HAL_Set_System_Config, int(hal_system_config_t, const void*, unsigned))
DYNALIB_FN(hal_core, HAL_Core_Enter_Safe_Mode, void(void*))
DYNALIB_FN(hal_core, HAL_Feature_Get, bool(HAL_Feature))
DYNALIB_FN(hal_core, HAL_Feature_Set, int(HAL_Feature, bool))
DYNALIB_END(hal_core)





#endif	/* HAL_DYNALIB_CORE_H */

