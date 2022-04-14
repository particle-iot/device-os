/**
 ******************************************************************************
 * @file    gpio_hal.h
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    13-Sept-2014
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

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __GPIO_HAL_H
#define __GPIO_HAL_H

/* Includes -----------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported types -----------------------------------------------------------*/

/* Exported constants -------------------------------------------------------*/

/* Exported macros ----------------------------------------------------------*/

/* Exported functions -------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hal_gpio_drive_t {
    HAL_GPIO_DRIVE_DEFAULT = 0,
    HAL_GPIO_DRIVE_HIGH = 1,
    HAL_GPIO_DRIVE_STANDARD = 2
} hal_gpio_drive_t;

typedef struct {
    uint16_t size;
    uint16_t version;

    PinMode mode;

    // Flags
    uint32_t set_value : 1;

    // Output value to set if set_value = 1
    uint8_t value;

    // Drive strength
    uint8_t drive_strength;
} hal_gpio_config_t;

#define HAL_GPIO_VERSION_1 (1)
#define HAL_GPIO_VERSION   (HAL_GPIO_VERSION_1)

void HAL_Pin_Mode(pin_t pin, PinMode mode);
int HAL_Pin_Configure(pin_t pin, const hal_gpio_config_t* conf, void* reserved);
PinMode HAL_Get_Pin_Mode(pin_t pin);
void HAL_GPIO_Write(pin_t pin, uint8_t value);
int32_t HAL_GPIO_Read(pin_t pin);
uint32_t HAL_Pulse_In(pin_t pin, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif  /* __GPIO_HAL_H */
