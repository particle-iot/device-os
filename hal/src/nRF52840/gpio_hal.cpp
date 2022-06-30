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
#include "check.h"
#include "system_error.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
#if HAL_PLATFORM_DEMUX
#include "demux.h"
#endif
using namespace particle;
#endif

PinMode hal_gpio_get_mode(hal_pin_t pin) {
    return (!is_valid_pin(pin)) ? PIN_MODE_NONE : hal_pin_map()[pin].pin_mode;
}

PinFunction hal_pin_validate_function(hal_pin_t pin, PinFunction pinFunction) {
    hal_pin_info_t* PIN_MAP = hal_pin_map();

    if (!is_valid_pin(pin)) {
        return PF_NONE;
    }
    if (pinFunction==PF_ADC && PIN_MAP[pin].adc_channel != ADC_CHANNEL_NONE) {
        return PF_ADC;
    }
    // Compatible with STM32 for wiring layer
    if (pinFunction==PF_TIMER && PIN_MAP[pin].pwm_instance != PWM_INSTANCE_NONE) {
        return PF_TIMER;
    }

    return PF_DIO;
}

int hal_gpio_configure(hal_pin_t pin, const hal_gpio_config_t* conf, void* reserved) {
    CHECK_TRUE(is_valid_pin(pin), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(conf, SYSTEM_ERROR_INVALID_ARGUMENT);

    hal_pin_info_t* PIN_MAP = hal_pin_map();

    PinMode mode = conf->mode;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t nrfPin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

        // Set pin function may reset nordic gpio configuration, should be called before the re-configuration
        if (mode != PIN_MODE_NONE) {
            hal_pin_set_function(pin, PF_DIO);
        } else {
            hal_pin_set_function(pin, PF_NONE);
        }

        // Pre-set the output value if requested to avoid a glitch
        if (conf->set_value && (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN || mode == OUTPUT_OPEN_DRAIN_PULLUP)) {
            if (conf->value) {
                nrf_gpio_pin_set(nrfPin);
            } else {
                nrf_gpio_pin_clear(nrfPin);
            }
        }

        switch (mode) {
            case OUTPUT: {
                auto driveStrength = NRF_GPIO_PIN_S0S1;
                if (conf->version >= HAL_GPIO_VERSION_1) {
                    if (conf->drive_strength == HAL_GPIO_DRIVE_HIGH) {
                        driveStrength = NRF_GPIO_PIN_H0H1;
                    }
                }
                nrf_gpio_cfg(nrfPin,
                    NRF_GPIO_PIN_DIR_OUTPUT,
                    NRF_GPIO_PIN_INPUT_DISCONNECT,
                    NRF_GPIO_PIN_NOPULL,
                    driveStrength,
                    NRF_GPIO_PIN_NOSENSE);
                break;
            }
            case INPUT: {
                nrf_gpio_cfg_input(nrfPin, NRF_GPIO_PIN_NOPULL);
                break;
            }
            case INPUT_PULLUP: {
                nrf_gpio_cfg_input(nrfPin, NRF_GPIO_PIN_PULLUP);
                break;
            }
            case INPUT_PULLDOWN: {
                nrf_gpio_cfg_input(nrfPin, NRF_GPIO_PIN_PULLDOWN);
                break;
            }
            case OUTPUT_OPEN_DRAIN: {
                auto driveStrength = NRF_GPIO_PIN_S0D1;
                if (conf->version >= HAL_GPIO_VERSION_1) {
                    if (conf->drive_strength == HAL_GPIO_DRIVE_HIGH) {
                        driveStrength = NRF_GPIO_PIN_H0D1;
                    }
                }
                nrf_gpio_cfg(nrfPin,
                    NRF_GPIO_PIN_DIR_OUTPUT,
                    NRF_GPIO_PIN_INPUT_CONNECT,
                    NRF_GPIO_PIN_NOPULL,
                    driveStrength,
                    NRF_GPIO_PIN_NOSENSE);
                break;
            }
            case OUTPUT_OPEN_DRAIN_PULLUP: {
                auto driveStrength = NRF_GPIO_PIN_S0D1;
                if (conf->version >= HAL_GPIO_VERSION_1) {
                    if (conf->drive_strength == HAL_GPIO_DRIVE_HIGH) {
                        driveStrength = NRF_GPIO_PIN_H0D1;
                    }
                }
                nrf_gpio_cfg(nrfPin,
                    NRF_GPIO_PIN_DIR_OUTPUT,
                    NRF_GPIO_PIN_INPUT_CONNECT,
                    NRF_GPIO_PIN_PULLUP,
                    driveStrength,
                    NRF_GPIO_PIN_NOSENSE);
                break;
            }
            case PIN_MODE_NONE: {
                nrf_gpio_cfg_default(nrfPin);
                break;
            }
            default: {
                return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
        }

        PIN_MAP[pin].pin_mode = mode;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        CHECK_TRUE(mode == INPUT || mode == INPUT_PULLUP || mode == OUTPUT, SYSTEM_ERROR_INVALID_ARGUMENT);

        // Set pin function may reset nordic gpio configuration, should be called before the re-configuration
        if (mode != PIN_MODE_NONE) {
            hal_pin_set_function(pin, PF_DIO);
        } else {
            hal_pin_set_function(pin, PF_NONE);
        }

        // Pre-set the output value if requested to avoid a glitch
        if (conf->set_value && mode == OUTPUT) {
            CHECK(Mcp23s17::getInstance().writePinValue(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin, conf->value));
        }

        CHECK(Mcp23s17::getInstance().setPinMode(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin, mode));
        PIN_MAP[pin].pin_mode = mode;
    }
#endif // HAL_PLATFORM_MCP23S17
#if HAL_PLATFORM_DEMUX
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_DEMUX) {
        CHECK_TRUE(mode == OUTPUT, SYSTEM_ERROR_INVALID_ARGUMENT);
        if (conf->set_value) {
            Demux::getInstance().write(PIN_MAP[pin].gpio_pin, conf->value);
        }
        PIN_MAP[pin].pin_mode = mode;
    }
#endif // HAL_PLATFORM_DEMUX
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif

    return 0;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void hal_gpio_mode(hal_pin_t pin, PinMode setMode) {
    const hal_gpio_config_t c = {
        .size = sizeof(c),
        .version = HAL_GPIO_VERSION,
        .mode = setMode,
        .set_value = 0,
        .value = 0,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    hal_gpio_configure(pin, &c, nullptr);
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(uint16_t pin) {
    // deprecated
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin) {
    // deprecated
    return PIN_MODE_NONE;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void hal_gpio_write(uint16_t pin, uint8_t value) {
    if (!is_valid_pin(pin)) {
        return;
    }

    hal_pin_info_t* PIN_MAP = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

        // PWM have conflict with GPIO OUTPUT mode on nRF52
        if (PIN_MAP[pin].pin_func == PF_PWM) {
            hal_gpio_mode(pin, OUTPUT);
        }

        if(value == 0) {
            nrf_gpio_pin_clear(nrf_pin);
        } else {
            nrf_gpio_pin_set(nrf_pin);
        }
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        Mcp23s17::getInstance().writePinValue(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin, value);
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_DEMUX) {
        Demux::getInstance().write(PIN_MAP[pin].gpio_pin, value);
    }
#endif
#endif
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t hal_gpio_read(uint16_t pin) {
    if (!is_valid_pin(pin)) {
        return 0;
    }

    hal_pin_info_t* PIN_MAP = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

        if ((PIN_MAP[pin].pin_mode == INPUT) ||
            (PIN_MAP[pin].pin_mode == INPUT_PULLUP) ||
            (PIN_MAP[pin].pin_mode == INPUT_PULLDOWN) ||
            (PIN_MAP[pin].pin_mode == OUTPUT_OPEN_DRAIN) ||
            (PIN_MAP[pin].pin_mode == OUTPUT_OPEN_DRAIN_PULLUP))
        {
            return nrf_gpio_pin_read(nrf_pin);
        } else if (PIN_MAP[pin].pin_mode == OUTPUT) {
            return nrf_gpio_pin_out_read(nrf_pin);
        } else {
            return 0;
        }
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        if ((PIN_MAP[pin].pin_mode == INPUT) ||
            (PIN_MAP[pin].pin_mode == INPUT_PULLUP) ||
            (PIN_MAP[pin].pin_mode == OUTPUT)) {
            uint8_t value = 0x00;
            Mcp23s17::getInstance().readPinValue(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin, &value);
            return value;
        } else if (PIN_MAP[pin].pin_mode == OUTPUT) {
            // TODO: read output value
        } else {
            return 0;
        }
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_DEMUX) {
        return Demux::getInstance().read(PIN_MAP[pin].gpio_pin);
    }
#endif
    else {
        return 0;
    }
#endif

    return 0;
}

/*
* @brief   blocking call to measure a high or low pulse
* @returns uint32_t pulse width in microseconds up to 3 seconds,
*          returns 0 on 3 second timeout error, or invalid pin.
*/
uint32_t hal_gpio_pulse_in(hal_pin_t pin, uint16_t value) {
    #define FAST_READ(pin)  ((reg->IN >> pin) & 1UL)

    if (!is_valid_pin(pin)) {
        return 0;
    }

    hal_pin_info_t* PIN_MAP = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
        NRF_GPIO_Type *reg = nrf_gpio_pin_port_decode(&nrf_pin);

        volatile uint32_t timeout_start = SYSTEM_TICK_COUNTER;

        /* If already on the value we want to measure, wait for the next one.
        * Time out after 3 seconds so we don't block the background tasks
        */
        // while (nrf_gpio_pin_read(gpio_pin_map) == value)
        while (FAST_READ(nrf_pin) == value) {
            if (SYSTEM_TICK_COUNTER - timeout_start > 192000000UL) {
                return 0;
            }
        }

        /* Wait until the start of the pulse.
        * Time out after 3 seconds so we don't block the background tasks
        */
        while (FAST_READ(nrf_pin) != value) {
            if (SYSTEM_TICK_COUNTER - timeout_start > 192000000UL) {
                return 0;
            }
        }

        /* Wait until this value changes, this will be our elapsed pulse width.
        * Time out after 3 seconds so we don't block the background tasks
        */
        volatile uint32_t pulse_start = SYSTEM_TICK_COUNTER;
        while (FAST_READ(nrf_pin) == value) {
            if (SYSTEM_TICK_COUNTER - timeout_start > 192000000UL) {
                return 0;
            }
        }

        return (SYSTEM_TICK_COUNTER - pulse_start) / SYSTEM_US_TICKS;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    } else {
        return 0;
    }
#endif
}
