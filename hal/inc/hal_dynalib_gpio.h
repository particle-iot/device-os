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

DYNALIB_FN(0, hal_gpio, hal_pin_map, hal_pin_info_t*(void))
DYNALIB_FN(1, hal_gpio, hal_pin_validate_function, PinFunction(hal_pin_t, PinFunction))
DYNALIB_FN(2, hal_gpio, hal_gpio_mode, void(hal_pin_t, PinMode))
DYNALIB_FN(3, hal_gpio, hal_gpio_get_mode, PinMode(hal_pin_t))
DYNALIB_FN(4, hal_gpio, hal_gpio_write, void(hal_pin_t, uint8_t))
DYNALIB_FN(5, hal_gpio, hal_gpio_read, int32_t(hal_pin_t))
DYNALIB_FN(6, hal_gpio, hal_interrupt_attach, int(uint16_t, hal_interrupt_handler_t, void*, InterruptMode, hal_interrupt_extra_configuration_t*))
DYNALIB_FN(7, hal_gpio, hal_interrupt_detach, int(uint16_t))
DYNALIB_FN(8, hal_gpio, hal_interrupt_enable_all, void(void))
DYNALIB_FN(9, hal_gpio, hal_interrupt_disable_all, void(void))

DYNALIB_FN(10, hal_gpio, HAL_DAC_Write, void(hal_pin_t, uint16_t))
DYNALIB_FN(11, hal_gpio, hal_adc_set_sample_time, void(uint8_t))
DYNALIB_FN(12, hal_gpio, hal_adc_read, int32_t(uint16_t))

DYNALIB_FN(13, hal_gpio, hal_pwm_write, void(uint16_t, uint8_t))
DYNALIB_FN(14, hal_gpio, hal_pwm_get_frequency, uint16_t(uint16_t))
DYNALIB_FN(15, hal_gpio, hal_pwm_get_analog_value, uint16_t(uint16_t))

DYNALIB_FN(16, hal_gpio, hal_interrupt_set_system_handler, uint8_t(hal_irq_t, const hal_interrupt_callback_t*, hal_interrupt_callback_t*, void*))
DYNALIB_FN(17, hal_gpio, hal_interrupt_get_system_handler, uint8_t(hal_irq_t, hal_interrupt_callback_t*, void*))
DYNALIB_FN(18, hal_gpio, hal_interrupt_trigger_system, void(hal_irq_t, void*))

DYNALIB_FN(19, hal_gpio, hal_gpio_pulse_in, uint32_t(hal_pin_t, uint16_t))
DYNALIB_FN(20, hal_gpio, hal_interrupt_suspend, void(void))
DYNALIB_FN(21, hal_gpio, hal_interrupt_restore, void(void))

DYNALIB_FN(22, hal_gpio, hal_pwm_write_with_frequency, void(uint16_t, uint8_t, uint16_t))
DYNALIB_FN(23, hal_gpio, HAL_DAC_Is_Enabled, uint8_t(hal_pin_t))
DYNALIB_FN(24, hal_gpio, HAL_DAC_Enable, uint8_t(hal_pin_t, uint8_t))

DYNALIB_FN(25, hal_gpio, HAL_DAC_Get_Resolution, uint8_t(hal_pin_t))
DYNALIB_FN(26, hal_gpio, HAL_DAC_Set_Resolution, void(hal_pin_t, uint8_t))
DYNALIB_FN(27, hal_gpio, HAL_DAC_Enable_Buffer, void(hal_pin_t pin, uint8_t state))
DYNALIB_FN(28, hal_gpio, hal_pwm_get_resolution, uint8_t(uint16_t))
DYNALIB_FN(29, hal_gpio, hal_pwm_set_resolution, void(uint16_t, uint8_t))
DYNALIB_FN(30, hal_gpio, hal_pwm_write_ext, void(uint16_t, uint32_t))
DYNALIB_FN(31, hal_gpio, hal_pwm_write_with_frequency_ext, void(uint16_t, uint32_t, uint32_t))
DYNALIB_FN(32, hal_gpio, hal_pwm_get_frequency_ext, uint32_t(uint16_t))
DYNALIB_FN(33, hal_gpio, hal_pwm_get_analog_value_ext, uint32_t(uint16_t))
DYNALIB_FN(34, hal_gpio, hal_pwm_get_max_frequency, uint32_t(uint16_t))
DYNALIB_FN(35, hal_gpio, hal_interrupt_detach_ext, int(uint16_t, uint8_t, void*))
DYNALIB_FN(36, hal_gpio, hal_interrupt_set_direct_handler, int(IRQn_Type irqn, hal_interrupt_direct_handler_t handler, uint32_t flags, void* reserved))
DYNALIB_FN(37, hal_gpio, hal_adc_sleep, int(bool, void*))
DYNALIB_FN(38, hal_gpio, hal_pwm_sleep, int(bool, void*))
DYNALIB_FN(39, hal_gpio, hal_gpio_configure, int(hal_pin_t, const hal_gpio_config_t*, void*))
DYNALIB_FN(40, hal_gpio, hal_adc_calibrate, int(uint32_t, void*))
DYNALIB_FN(41, hal_gpio, hal_adc_set_reference, int(uint32_t, void*))
DYNALIB_FN(42, hal_gpio, hal_adc_get_reference, int(void*))

DYNALIB_END(hal_gpio)

#endif	/* HAL_DYNALIB_GPIO_H */

