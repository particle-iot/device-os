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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platforms.h"
#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_pin_info_t hal_pin_info_t;

typedef uint16_t hal_pin_t;
typedef hal_pin_t pin_t; // __attribute__((deprecated("Use hal_pin_t instead")));

#if HAL_PLATFORM_IO_EXTENSION
typedef enum hal_pin_type_t {
    HAL_PIN_TYPE_UNKNOWN,
    HAL_PIN_TYPE_MCU,
    HAL_PIN_TYPE_IO_EXPANDER,
    HAL_PIN_TYPE_DEMUX,
    HAL_PIN_TYPE_MAX
} hal_pin_type_t;
#endif

typedef enum PinMode {
    INPUT = 0,
    OUTPUT = 1,
    INPUT_PULLUP = 2,
    INPUT_PULLDOWN = 3,
    AF_OUTPUT_PUSHPULL = 4, // Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
    AF_OUTPUT_DRAIN = 5,    // Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
    AN_INPUT = 6,           // Used internally for ADC Input
    AN_OUTPUT = 7,          // Used internally for DAC Output,
    OUTPUT_OPEN_DRAIN = AF_OUTPUT_DRAIN,
    OUTPUT_OPEN_DRAIN_PULLUP = 8,
    PIN_MODE_SWD = 9,
    PIN_MODE_NONE = 0xFF
} PinMode;

typedef enum PinFunction {
    PF_NONE,
    PF_DIO,
    PF_TIMER,
    PF_ADC,
    PF_DAC,
    PF_UART,
    PF_PWM,
    PF_SPI,
    PF_I2C
} PinFunction;

hal_pin_info_t* hal_pin_map(void);
PinFunction hal_pin_validate_function(hal_pin_t pin, PinFunction pinFunction);
void hal_pin_set_function(hal_pin_t pin, PinFunction pin_func);

#define PIN_INVALID 0xff

#include "pinmap_impl.h"

#define hal_pin_is_valid(pin) ((pin) < TOTAL_PINS)

typedef hal_pin_info_t Hal_Pin_Info __attribute__((deprecated("Use hal_pin_info_t instead")));
#if HAL_PLATFORM_IO_EXTENSION
typedef hal_pin_type_t Hal_Pin_Type __attribute__((deprecated("Use hal_pin_type_t instead")));
#endif // #if HAL_PLATFORM_IO_EXTENSION

static inline PinFunction __attribute__((deprecated("Use hal_pin_validate_function() instead"), always_inline))
HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction) {
    return hal_pin_validate_function(pin, pinFunction);
}

static inline Hal_Pin_Info* __attribute__((deprecated("Use hal_pin_map() instead"), always_inline))
Hal_Pin_Map(void) {
    return hal_pin_map();
}


#ifdef __cplusplus
}
#endif
