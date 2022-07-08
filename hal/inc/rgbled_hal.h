/**
 ******************************************************************************
 * @authors Matthew McGowan
 * @date    27 April 2015
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

#ifndef RGBLED_HAL_H
#define	RGBLED_HAL_H

#include <stdint.h>
#include "platform_config.h"

#ifdef	__cplusplus
extern "C" {
#endif

#include "rgbled_hal_impl.h"

// #if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
// extern const hal_led_config_t HAL_Leds_Default[];
// #else
// extern hal_led_config_t HAL_Leds_Default[];
// #endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

void hal_led_init(uint8_t led, hal_led_config_t* conf, void* reserved);
void hal_led_set_rgb_values(uint16_t r, uint16_t g, uint16_t b, void* reserved);
void hal_led_get_rgb_values(uint16_t* rgb, void* reserved);
uint32_t hal_led_get_max_rgb_values(void* reserved);
void hal_led_set_user(uint8_t state, void* reserved);
void hal_led_toggle_user(void* reserved);
hal_led_config_t* hal_led_set_configuration(uint8_t led, hal_led_config_t* conf, void* reserved);
hal_led_config_t* hal_led_get_configuration(uint8_t led, void* reserved);

// This is the low-level api to the LED
// Deprecated, not exported in HAL
void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b);
void Get_RGB_LED_Values(uint16_t* values);
uint16_t Get_RGB_LED_Max_Value(void);
void Set_User_LED(uint8_t state);
void Toggle_User_LED(void);
void RGB_LED_Uninit();

#ifdef	__cplusplus
}
#endif

#endif	/* RGBLED_HAL_H */
