/**
 ******************************************************************************
 * @file    hal_dynalib_i2c.h
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

#ifndef HAL_DYNALIB_I2C_H
#define	HAL_DYNALIB_I2C_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "i2c_hal.h"
#endif

DYNALIB_BEGIN(hal_i2c)
// I2C has to be included because the original tinker app was linked with some I2C calls
// such as HAL_I2C_Is_Enabled
#if !defined(SYSTEM_MINIMAL) && 0
DYNALIB_FN(hal_i2c,HAL_I2C_Set_Speed)
DYNALIB_FN(hal_i2c,HAL_I2C_Enable_DMA_Mode)
DYNALIB_FN(hal_i2c,HAL_I2C_Stretch_Clock)
DYNALIB_FN(hal_i2c,HAL_I2C_Begin)
DYNALIB_FN(hal_i2c,HAL_I2C_End)
DYNALIB_FN(hal_i2c,HAL_I2C_Request_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Begin_Transmission)
DYNALIB_FN(hal_i2c,HAL_I2C_End_Transmission)
DYNALIB_FN(hal_i2c,HAL_I2C_Write_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Available_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Read_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Peek_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Flush_Data)
DYNALIB_FN(hal_i2c,HAL_I2C_Is_Enabled)
DYNALIB_FN(hal_i2c,HAL_I2C_Set_Callback_On_Receive)
DYNALIB_FN(hal_i2c,HAL_I2C_Set_Callback_On_Request)
#endif
DYNALIB_END(hal_i2c)

#endif	/* HAL_DYNALIB_I2C_H */

