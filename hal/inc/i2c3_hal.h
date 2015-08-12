/**
 ******************************************************************************
 * @file    i2c3_hal.h
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C3_HAL_H
#define __I2C3_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported types ------------------------------------------------------------*/
// typedef enum
// {
//   I2C_MODE_MASTER = 0, I2C_MODE_SLAVE = 1
// } I2C_Mode;


/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define CLOCK_SPEED_100KHZ      (uint32_t)100000
#define CLOCK_SPEED_400KHZ      (uint32_t)400000

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_I2C3_Set_Speed(uint32_t speed);
void HAL_I2C3_Enable_DMA_Mode(bool enable);
void HAL_I2C3_Stretch_Clock(bool stretch);
void HAL_I2C3_Begin(bool mode, uint8_t address);
void HAL_I2C3_End(void);
uint32_t HAL_I2C3_Request_Data(uint8_t address, uint8_t quantity, uint8_t stop);
void HAL_I2C3_Begin_Transmission(uint8_t address);
uint8_t HAL_I2C3_End_Transmission(uint8_t stop);
uint32_t HAL_I2C3_Write_Data(uint8_t data);
int32_t HAL_I2C3_Available_Data(void);
int32_t HAL_I2C3_Read_Data(void);
int32_t HAL_I2C3_Peek_Data(void);
void HAL_I2C3_Flush_Data(void);
bool HAL_I2C3_Is_Enabled(void);
void HAL_I2C3_Set_Callback_On_Receive(void (*function)(int));
void HAL_I2C3_Set_Callback_On_Request(void (*function)(void));

#ifdef __cplusplus
}
#endif

#endif  /* __I2C3_HAL_H */
