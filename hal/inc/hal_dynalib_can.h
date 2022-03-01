/**
 ******************************************************************************
 * @file    hal_dynalib_can.h
 * @authors Brian Spranger
 * @date    01 October 2015
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

#ifndef HAL_DYNALIB_CAN_H
#define HAL_DYNALIB_CAN_H

#include "dynalib.h"
#include "hal_platform.h"

#if !HAL_PLATFORM_NRF52840

#ifdef DYNALIB_EXPORT
#include "can_hal.h"
#endif

DYNALIB_BEGIN(hal_can)

DYNALIB_FN(0, hal_can, HAL_CAN_Init, void(HAL_CAN_Channel, uint16_t, uint16_t, void*))
DYNALIB_FN(1, hal_can, HAL_CAN_Begin, void(HAL_CAN_Channel, uint32_t, uint32_t, void*))
DYNALIB_FN(2, hal_can, HAL_CAN_End, void(HAL_CAN_Channel, void*))
DYNALIB_FN(3, hal_can, HAL_CAN_Transmit, bool(HAL_CAN_Channel, const CANMessage*, void*))
DYNALIB_FN(4, hal_can, HAL_CAN_Receive, bool(HAL_CAN_Channel, CANMessage*, void*))
DYNALIB_FN(5, hal_can, HAL_CAN_Available_Messages, uint8_t(HAL_CAN_Channel, void*))
DYNALIB_FN(6, hal_can, HAL_CAN_Add_Filter, bool(HAL_CAN_Channel, uint32_t, uint32_t, HAL_CAN_Filters, void*))
DYNALIB_FN(7, hal_can, HAL_CAN_Clear_Filters, void(HAL_CAN_Channel, void*))
DYNALIB_FN(8, hal_can, HAL_CAN_Is_Enabled, bool(HAL_CAN_Channel))
DYNALIB_FN(9, hal_can, HAL_CAN_Error_Status, HAL_CAN_Errors(HAL_CAN_Channel))

DYNALIB_END(hal_can)

#endif

#endif	/* HAL_DYNALIB_CAN_H */

