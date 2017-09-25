/**
 ******************************************************************************
 * @file    hal_dynalib_gpio.h
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

#ifndef HAL_DYNALIB_GPIO_H
#define	HAL_DYNALIB_GPIO_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "adc_hal.h"
#include "dac_hal.h"
#include "pwm_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_gpio)

DYNALIB_FN(0, hal_gpio, HAL_Pin_Map, STM32_Pin_Info*(void))
DYNALIB_FN(1, hal_gpio, HAL_Validate_Pin_Function, PinFunction(pin_t, PinFunction))
DYNALIB_FN(2, hal_gpio, HAL_Pin_Mode, void(pin_t, PinMode))
DYNALIB_FN(3, hal_gpio, HAL_Get_Pin_Mode, PinMode(pin_t))
DYNALIB_FN(4, hal_gpio, HAL_GPIO_Write, void(pin_t, uint8_t))
DYNALIB_FN(5, hal_gpio, HAL_GPIO_Read, int32_t(pin_t))
DYNALIB_FN(6, hal_gpio, HAL_Interrupts_Attach, void(uint16_t, HAL_InterruptHandler, void*, InterruptMode, HAL_InterruptExtraConfiguration*))
DYNALIB_FN(7, hal_gpio, HAL_Interrupts_Detach, void(uint16_t))
DYNALIB_FN(8, hal_gpio, HAL_Interrupts_Enable_All, void(void))
DYNALIB_FN(9, hal_gpio, HAL_Interrupts_Disable_All, void(void))

DYNALIB_FN(10, hal_gpio, HAL_DAC_Write, void(pin_t, uint16_t))
DYNALIB_FN(11, hal_gpio, HAL_ADC_Set_Sample_Time, void(uint8_t))
DYNALIB_FN(12, hal_gpio, HAL_ADC_Read, int32_t(uint16_t))

DYNALIB_FN(13, hal_gpio, HAL_PWM_Write, void(uint16_t, uint8_t))
DYNALIB_FN(14, hal_gpio, HAL_PWM_Get_Frequency, uint16_t(uint16_t))
DYNALIB_FN(15, hal_gpio, HAL_PWM_Get_AnalogValue, uint16_t(uint16_t))

DYNALIB_FN(16, hal_gpio, HAL_Set_System_Interrupt_Handler, uint8_t(hal_irq_t, const HAL_InterruptCallback*, HAL_InterruptCallback*, void*))
DYNALIB_FN(17, hal_gpio, HAL_Get_System_Interrupt_Handler, uint8_t(hal_irq_t, HAL_InterruptCallback*, void*))
DYNALIB_FN(18, hal_gpio, HAL_System_Interrupt_Trigger, void(hal_irq_t, void*))

DYNALIB_FN(19, hal_gpio, HAL_Pulse_In, uint32_t(pin_t, uint16_t))
DYNALIB_FN(20, hal_gpio, HAL_Interrupts_Suspend, void(void))
DYNALIB_FN(21, hal_gpio, HAL_Interrupts_Restore, void(void))

DYNALIB_FN(22, hal_gpio, HAL_PWM_Write_With_Frequency, void(uint16_t, uint8_t, uint16_t))
DYNALIB_FN(23, hal_gpio, HAL_DAC_Is_Enabled, uint8_t(pin_t))
DYNALIB_FN(24, hal_gpio, HAL_DAC_Enable, uint8_t(pin_t, uint8_t))

DYNALIB_FN(25, hal_gpio, HAL_DAC_Get_Resolution, uint8_t(pin_t))
DYNALIB_FN(26, hal_gpio, HAL_DAC_Set_Resolution, void(pin_t, uint8_t))
DYNALIB_FN(27, hal_gpio, HAL_DAC_Enable_Buffer, void(pin_t pin, uint8_t state))
DYNALIB_FN(28, hal_gpio, HAL_PWM_Get_Resolution, uint8_t(uint16_t))
DYNALIB_FN(29, hal_gpio, HAL_PWM_Set_Resolution, void(uint16_t, uint8_t))
DYNALIB_FN(30, hal_gpio, HAL_PWM_Write_Ext, void(uint16_t, uint32_t))
DYNALIB_FN(31, hal_gpio, HAL_PWM_Write_With_Frequency_Ext, void(uint16_t, uint32_t, uint32_t))
DYNALIB_FN(32, hal_gpio, HAL_PWM_Get_Frequency_Ext, uint32_t(uint16_t))
DYNALIB_FN(33, hal_gpio, HAL_PWM_Get_AnalogValue_Ext, uint32_t(uint16_t))
DYNALIB_FN(34, hal_gpio, HAL_PWM_Get_Max_Frequency, uint32_t(uint16_t))
DYNALIB_FN(35, hal_gpio, HAL_Interrupts_Detach_Ext, void(uint16_t, uint8_t, void*))
DYNALIB_FN(36, hal_gpio, HAL_Set_Direct_Interrupt_Handler, int(IRQn_Type irqn, HAL_Direct_Interrupt_Handler handler, uint32_t flags, void* reserved))

DYNALIB_END(hal_gpio)

#endif	/* HAL_DYNALIB_GPIO_H */

