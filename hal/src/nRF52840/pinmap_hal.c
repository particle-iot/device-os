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

#include "pinmap_hal.h"
#include "pwm_hal.h"
#include "module_info.h"

void hal_pin_set_function(hal_pin_t pin, PinFunction pin_func) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pin >= TOTAL_PINS) {
        return;
    }

    hal_pin_info_t* PIN_MAP = hal_pin_map();

    // Release peripheral resource
    if (pin_func != PIN_MAP[pin].pin_func) {
        switch (PIN_MAP[pin].pin_func) {
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

    PIN_MAP[pin].pin_func = pin_func;
#endif
}