/**
 ******************************************************************************
 * @file    hal_dynalib_ota.h
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

#ifndef HAL_DYNALIB_OTA_H
#define	HAL_DYNALIB_OTA_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "ota_flash_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_ota)

DYNALIB_FN(0, hal_ota, HAL_OTA_FlashAddress, uint32_t(void))
DYNALIB_FN(1, hal_ota, HAL_OTA_FlashLength, uint32_t(void))
DYNALIB_FN(2, hal_ota, HAL_OTA_ChunkSize, uint16_t(void))

DYNALIB_FN(3, hal_ota, HAL_OTA_Flashed_GetStatus, bool(void))
DYNALIB_FN(4, hal_ota, HAL_OTA_Flashed_ResetStatus, void(void))

DYNALIB_FN(5, hal_ota, HAL_FLASH_Begin, bool(uint32_t, uint32_t, void*))
DYNALIB_FN(6, hal_ota, HAL_FLASH_Update, int(const uint8_t*, uint32_t, uint32_t, void*))
DYNALIB_FN(7, hal_ota, HAL_FLASH_End, hal_update_complete_t(hal_module_t*))
DYNALIB_FN(8, hal_ota, HAL_FLASH_OTA_Validate, int(hal_module_t*, bool, module_validation_flags_t, void*))
DYNALIB_FN(9, hal_ota, HAL_OTA_Add_System_Info, void(hal_system_info_t* info, bool create, void* reserved))
DYNALIB_FN(10, hal_ota, HAL_FLASH_ApplyPendingUpdate,  hal_update_complete_t(hal_module_t*, bool, void*))
DYNALIB_END(hal_ota)

#endif	/* HAL_DYNALIB_OTA_H */

