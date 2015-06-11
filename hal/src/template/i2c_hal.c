/**
 ******************************************************************************
 * @file    i2c_hal.c
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

/* Includes ------------------------------------------------------------------*/
#include "i2c_hal.h"
#include "gpio_hal.h"

void HAL_I2C_Set_Speed(uint32_t speed)
{
}

void HAL_I2C_Stretch_Clock(bool stretch)
{
}

void HAL_I2C_Begin(I2C_Mode mode, uint8_t address)
{
}

void HAL_I2C_End(void)
{
}

uint32_t HAL_I2C_Request_Data(uint8_t address, uint8_t quantity, uint8_t stop)
{
    return 0;
}

void HAL_I2C_Begin_Transmission(uint8_t address)
{
}

uint8_t HAL_I2C_End_Transmission(uint8_t stop)
{
  return 0;
}

uint32_t HAL_I2C_Write_Data(uint8_t data)
{
  return 0;
}

int32_t HAL_I2C_Available_Data(void)
{
  return 0;
}

int32_t HAL_I2C_Read_Data(void)
{
  return 0;
}

int32_t HAL_I2C_Peek_Data(void)
{
    return 0;
}

void HAL_I2C_Flush_Data(void)
{
  // XXX: to be implemented.
}

bool HAL_I2C_Is_Enabled(void)
{
    return false;
}

void HAL_I2C_Set_Callback_On_Receive(void (*function)(int))
{
  
}

void HAL_I2C_Set_Callback_On_Request(void (*function)(void))
{
  
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_EV_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request(Only for Slave mode).
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_EV_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_ER_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_ER_Handler(void)
{
}

void HAL_I2C_Enable_DMA_Mode(bool enable)
{
}
