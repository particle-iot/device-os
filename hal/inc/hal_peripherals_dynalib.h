/**
 ******************************************************************************
 * @file    hal_peripherals_dynalib.h
 * @authors Matthew McGowan
 * @date    16 February 2015
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

/**
 * All "optional" peripherals not used by the system code.
 */

#ifndef HAL_PERIPHERALS_DYNALIB_H
#define	HAL_PERIPHERALS_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(hal_peripherals)
DYNALIB_FN(hal_peripherals,HAL_ADC_Set_Sample_Time)
DYNALIB_FN(hal_peripherals,HAL_ADC_Read)

DYNALIB_FN(hal_peripherals,HAL_Tone_Start)
DYNALIB_FN(hal_peripherals,HAL_Tone_Stop)
DYNALIB_FN(hal_peripherals,HAL_Tone_Get_Frequency)
DYNALIB_FN(hal_peripherals,HAL_Tone_Is_Stopped)

DYNALIB_FN(hal_peripherals,HAL_DAC_Write)

DYNALIB_FN(hal_peripherals,HAL_EEPROM_Init)
DYNALIB_FN(hal_peripherals,HAL_EEPROM_Read)
DYNALIB_FN(hal_peripherals,HAL_EEPROM_Write)
DYNALIB_FN(hal_peripherals,HAL_EEPROM_Length)

DYNALIB_FN(hal_peripherals,HAL_Validate_Pin_Function)
DYNALIB_FN(hal_peripherals,HAL_Pin_Mode)
DYNALIB_FN(hal_peripherals,HAL_Get_Pin_Mode)
DYNALIB_FN(hal_peripherals,HAL_GPIO_Write)
DYNALIB_FN(hal_peripherals,HAL_GPIO_Read)

DYNALIB_FN(hal_peripherals,HAL_I2C_Set_Speed)
DYNALIB_FN(hal_peripherals,HAL_I2C_Enable_DMA_Mode)
DYNALIB_FN(hal_peripherals,HAL_I2C_Stretch_Clock)
DYNALIB_FN(hal_peripherals,HAL_I2C_Begin)
DYNALIB_FN(hal_peripherals,HAL_I2C_End)
DYNALIB_FN(hal_peripherals,HAL_I2C_Request_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Begin_Transmission)
DYNALIB_FN(hal_peripherals,HAL_I2C_End_Transmission)
DYNALIB_FN(hal_peripherals,HAL_I2C_Write_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Available_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Read_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Peek_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Flush_Data)
DYNALIB_FN(hal_peripherals,HAL_I2C_Is_Enabled)
DYNALIB_FN(hal_peripherals,HAL_I2C_Set_Callback_On_Receive)
DYNALIB_FN(hal_peripherals,HAL_I2C_Set_Callback_On_Request)

DYNALIB_FN(hal_peripherals,HAL_Interrupts_Attach)
DYNALIB_FN(hal_peripherals,HAL_Interrupts_Detach)
DYNALIB_FN(hal_peripherals,HAL_Interrupts_Enable_All)
DYNALIB_FN(hal_peripherals,HAL_Interrupts_Disable_All)

DYNALIB_FN(hal_peripherals,HAL_PWM_Write)
DYNALIB_FN(hal_peripherals,HAL_PWM_Get_Frequency)
DYNALIB_FN(hal_peripherals,HAL_PWM_Get_AnalogValue)

DYNALIB_FN(hal_peripherals,HAL_Servo_Attach)
DYNALIB_FN(hal_peripherals,HAL_Servo_Detach)
DYNALIB_FN(hal_peripherals,HAL_Servo_Write_Pulse_Width)
DYNALIB_FN(hal_peripherals,HAL_Servo_Read_Pulse_Width)
DYNALIB_FN(hal_peripherals,HAL_Servo_Read_Frequency)        
        
DYNALIB_FN(hal_peripherals,HAL_SPI_Begin)
DYNALIB_FN(hal_peripherals,HAL_SPI_End)
DYNALIB_FN(hal_peripherals,HAL_SPI_Set_Bit_Order)
DYNALIB_FN(hal_peripherals,HAL_SPI_Set_Data_Mode)
DYNALIB_FN(hal_peripherals,HAL_SPI_Set_Clock_Divider)
DYNALIB_FN(hal_peripherals,HAL_SPI_Send_Receive_Data)
DYNALIB_FN(hal_peripherals,HAL_SPI_Is_Enabled)
        
DYNALIB_FN(hal_peripherals,HAL_USART_Init)
DYNALIB_FN(hal_peripherals,HAL_USART_Begin)
DYNALIB_FN(hal_peripherals,HAL_USART_End)
DYNALIB_FN(hal_peripherals,HAL_USART_Write_Data)
DYNALIB_FN(hal_peripherals,HAL_USART_Available_Data)
DYNALIB_FN(hal_peripherals,HAL_USART_Read_Data)
DYNALIB_FN(hal_peripherals,HAL_USART_Peek_Data)
DYNALIB_FN(hal_peripherals,HAL_USART_Flush_Data)
DYNALIB_FN(hal_peripherals,HAL_USART_Is_Enabled)
DYNALIB_END(hal_peripherals)        
        
#endif	/* HAL_PERIPHERALS_DYNALIB_H */

