/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */


#ifndef FAST_PIN_H
#define	FAST_PIN_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "pinmap_hal.h"

#ifndef STM32F10X
    #if defined(STM32F10X_MD) || defined(STM32F10X_HD)
        #define STM32F10X
    #endif
#endif

#ifdef STM32F10X

inline void pinSetFast(pin_t _pin)
{
    PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin;
}

inline void pinResetFast(pin_t _pin)
{
    PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin;
}

#elif defined(STM32F2XX)
static STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

inline void pinSetFast(pin_t _pin)
{
    PIN_MAP[_pin].gpio_peripheral->BSRRL = PIN_MAP[_pin].gpio_pin;
}

inline void pinResetFast(pin_t _pin)
{
    PIN_MAP[_pin].gpio_peripheral->BSRRH = PIN_MAP[_pin].gpio_pin;
}


#else
    #error "*** MCU architecture not supported by this library. ***"
#endif


inline void digitalWriteFast(pin_t pin, uint8_t value)
{
    if (value)
        pinSetFast(pin);
    else
        pinResetFast(pin);
}



#ifdef	__cplusplus
}
#endif

#endif	/* FAST_PIN_H */

