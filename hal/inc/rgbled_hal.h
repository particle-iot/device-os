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
// extern const led_config_t HAL_Leds_Default[];
// #else
// extern led_config_t HAL_Leds_Default[];
// #endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

void HAL_Led_Init(uint8_t led, led_config_t* conf, void* reserved);
void HAL_Led_Rgb_Set_Values(uint16_t r, uint16_t g, uint16_t b, void* reserved);
void HAL_Led_Rgb_Get_Values(uint16_t* rgb, void* reserved);
uint32_t HAL_Led_Rgb_Get_Max_Value(void* reserved);
void HAL_Led_User_Set(uint8_t state, void* reserved);
void HAL_Led_User_Toggle(void* reserved);

led_config_t* HAL_Led_Set_Configuration(uint8_t led, led_config_t* conf, void* reserved);
led_config_t* HAL_Led_Get_Configuration(uint8_t led, void* reserved);

// This is the low-level api to the LED
// Deprecated, not exported in HAL
void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b);
void Get_RGB_LED_Values(uint16_t* values);
uint16_t Get_RGB_LED_Max_Value(void);
void Set_User_LED(uint8_t state);
void Toggle_User_LED(void);
uint16_t Get_RGB_LED_Max_Value(void);
void RGB_LED_Uninit();

#ifdef	__cplusplus
}
#endif

#endif	/* RGBLED_HAL_H */
