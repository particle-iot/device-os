/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "nrf_gpio.h" 
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "hw_ticks.h"
#include <stddef.h>

inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin)
{
    return pin < TOTAL_PINS;
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return (!is_valid_pin(pin)) ? PIN_MODE_NONE : HAL_Pin_Map()[pin].pin_mode;
}

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (!is_valid_pin(pin))
        return PF_NONE;
    if (pinFunction==PF_ADC && PIN_MAP[pin].adc_channel != ADC_CHANNEL_NONE)
        return PF_ADC;
    // Compatible with STM32 for wiring layer
    if (pinFunction==PF_TIMER && PIN_MAP[pin].pwm_instance != PWM_INSTANCE_NONE)
        return PF_TIMER;
    return PF_DIO;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void HAL_Pin_Mode(pin_t pin, PinMode setMode)
{
    if (!is_valid_pin(pin))
    {
        return;
    }
    
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t gpio_pin_map = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
    
    switch (setMode)
    {
        case OUTPUT:
            nrf_gpio_cfg_output(gpio_pin_map);
            PIN_MAP[pin].pin_mode = OUTPUT;
            HAL_Set_Pin_Function(pin, PF_DIO);
            break;

        case INPUT:
            nrf_gpio_cfg_input(gpio_pin_map, NRF_GPIO_PIN_NOPULL);
            PIN_MAP[pin].pin_mode = INPUT;
            HAL_Set_Pin_Function(pin, PF_DIO);
            break;

        case INPUT_PULLUP:
            nrf_gpio_cfg_input(gpio_pin_map, NRF_GPIO_PIN_PULLUP);
            PIN_MAP[pin].pin_mode = INPUT_PULLUP;
            HAL_Set_Pin_Function(pin, PF_DIO);
            break;

        case INPUT_PULLDOWN:
            nrf_gpio_cfg_input(gpio_pin_map, NRF_GPIO_PIN_PULLDOWN);
            PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
            HAL_Set_Pin_Function(pin, PF_DIO);
            break;

        case PIN_MODE_NONE:
            nrf_gpio_cfg_default(gpio_pin_map);
            HAL_Set_Pin_Function(pin, PF_NONE);
        default:
            break;
    }

    
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(uint16_t pin)
{
    // deprecated
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin)
{
    // deprecated
    return PIN_MODE_NONE;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void HAL_GPIO_Write(uint16_t pin, uint8_t value)
{
    if (!is_valid_pin(pin))
    {
        return;
    }
    
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t gpio_pin_map = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

    if(value == 0)
    {
        nrf_gpio_pin_clear(gpio_pin_map);
    }
    else
    {
        nrf_gpio_pin_set(gpio_pin_map);
    }
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    if (!is_valid_pin(pin))
    {
        return 0;
    }
    
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t gpio_pin_map = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
    
    if ((PIN_MAP[pin].pin_mode == INPUT) ||
        (PIN_MAP[pin].pin_mode == INPUT_PULLUP) ||
        (PIN_MAP[pin].pin_mode == INPUT_PULLDOWN))
    {
        return nrf_gpio_pin_read(gpio_pin_map);
    }
    else
    {
        return 0;
    }
}

/*
* @brief   blocking call to measure a high or low pulse
* @returns uint32_t pulse width in microseconds up to 3 seconds,
*          returns 0 on 3 second timeout error, or invalid pin.
*/
uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
    if (!is_valid_pin(pin))
    {
        return 0;
    }
    
    volatile uint32_t timeout_start = GetSystem1MsTick(); 

    HAL_Pin_Mode(pin, INPUT);

    /* If already on the value we want to measure, wait for the next one.
     * Time out after 3 seconds so we don't block the background tasks
     */
    while (HAL_GPIO_Read(pin) == value)
    {
        if (GetSystem1MsTick() - timeout_start > 3000)
        {
            return 0;
        }
    }

    /* Wait until the start of the pulse.
     * Time out after 3 seconds so we don't block the background tasks
     */
    while (HAL_GPIO_Read(pin) != value)
    {
        if (GetSystem1MsTick() - timeout_start > 3000)
        {
            return 0;
        }
    }
    
    /* Wait until this value changes, this will be our elapsed pulse width.
     * Time out after 3 seconds so we don't block the background tasks
     */
    volatile uint32_t pulse_start = GetSystem1MsTick();
    while (HAL_GPIO_Read(pin) == value)
    {
        if (GetSystem1MsTick() - timeout_start > 3000)
        {
            return 0;
        }
    }

    return GetSystem1MsTick() - pulse_start;
}




