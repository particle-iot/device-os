/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "pinmap_hal.h"
#include "pwm_hal.h"

uint32_t hal_pin_to_rtl_pin(hal_pin_t pin) {
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    return ((pinInfo->gpio_port << 5) | pinInfo->gpio_pin) & 0x0000003F;
}

PinFunction hal_pin_validate_function(hal_pin_t pin, PinFunction pin_function) {
    if (!hal_pin_is_valid(pin)) {
        return PF_NONE;
    }
    hal_pin_info_t* pinmap = hal_pin_map();
    if (pin_function == PF_ADC && pinmap[pin].adc_channel != ADC_CHANNEL_NONE) {
        return PF_ADC;
    }
    if (pin_function == PF_TIMER && pinmap[pin].pwm_instance != PWM_INSTANCE_NONE) {
        return PF_TIMER;
    }
    return PF_DIO;
}

void hal_pin_set_function(hal_pin_t pin, PinFunction pin_func) {
    if (!hal_pin_is_valid(pin)) {
        return;
    }
    hal_pin_info_t* pinmap = hal_pin_map();
    // Release peripheral resource
    if (pin_func != pinmap[pin].pin_func) {
        switch (pinmap[pin].pin_func) {
            case PF_PWM: {
                hal_pwm_reset_pin(pin);
                break;
            }
            case PF_ADC: {
                // TODO: reset ADC pin
                break;
            }
            default:
                break;
        }
    }

    pinmap[pin].pin_func = pin_func;
}
