/**
 ******************************************************************************
 * @file    gpio_hal.c
 * @authors Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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

/* Includes -----------------------------------------------------------------*/
#include "gpio_hal.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/* Extern variables ---------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/

PinMode hal_gpio_get_mode(hal_pin_t pin)
{
    return PIN_MODE_NONE;
}

PinFunction hal_pin_validate_function(hal_pin_t pin, PinFunction pinFunction)
{
    return PF_DIO;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void hal_gpio_mode(hal_pin_t pin, PinMode setMode)
{
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(PinMode mode)
{
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode()
{
  return PIN_MODE_NONE;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void hal_gpio_write(uint16_t pin, uint8_t value)
{
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t hal_gpio_read(uint16_t pin)
{
    return 0;
}

int hal_gpio_configure(hal_pin_t pin, const hal_gpio_config_t* conf, void* reserved)
{
    return 0;
}
