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

#define INTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS
#include <stdint.h>
#include "spark_macros.h"
#include "panic.h"
#include "debug.h"
#include "rgbled.h"
#include "delay_hal.h"
#include "watchdog_hal.h"
#include "interrupts_hal.h"
#include "hal_platform.h"
#include "core_hal.h"
#include "delay_hal.h"

#define PANIC_LED_COLOR RGB_COLOR_RED

LOG_SOURCE_CATEGORY("panic");

static void panic_internal(const ePanicCode code, const void* extraInfo);

static PanicHook panicHook = panic_internal;

/****************************************************************************
* Public Functions
****************************************************************************/

void panic_hook_set(const PanicHook overrideFunc)
{
    //store the new hook
    panicHook = (overrideFunc != NULL) ? overrideFunc : panic_internal;
}

void panic_do(const ePanicCode code, void* extraInfo, void(*dummy)(uint32_t))
{
    #if HAL_PLATFORM_CORE_ENTER_PANIC_MODE
        HAL_Core_Enter_Panic_Mode(NULL);
    #else
        HAL_disable_irq();
    #endif // HAL_PLATFORM_CORE_ENTER_PANIC_MODE

    //run the panic!
    panicHook(code, extraInfo);

    //if it returns, run this code!
    #if defined(RELEASE_BUILD) || PANIC_BUT_KEEP_CALM == 1
        HAL_Core_System_Reset_Ex(RESET_REASON_PANIC, code, NULL);
    #endif
}

/****************************************************************************
* Private Functions
****************************************************************************/

static void panic_led_flash(const uint32_t onMS, const uint32_t ofMS, const uint32_t pauseMS) 
{
    for (uint16_t c = 3; c; c--) {
        LED_On(PARTICLE_LED_RGB);
        HAL_Delay_Microseconds(onMS);
        LED_Off(PARTICLE_LED_RGB);
        HAL_Delay_Microseconds(ofMS);
    }
    HAL_Delay_Microseconds(pauseMS);
}

static void panic_internal(const ePanicCode code, const void* extraInfo)
{
    LED_SetRGBColor(RGB_COLOR_RED);
    LED_SetBrightness(DEFAULT_LED_RGB_BRIGHTNESS);
    LED_Signaling_Stop();
    LOG_PRINT(TRACE, "!");
    LED_Off(PARTICLE_LED_RGB);
    LED_SetRGBColor(PANIC_LED_COLOR);

    int loops = 2;
    while(loops) {
        // preamble
        panic_led_flash( MS2u(150), MS2u(100), MS2u(100) );
        panic_led_flash( MS2u(300), MS2u(100), MS2u(100) );
        panic_led_flash( MS2u(150), MS2u(100), MS2u(900) );
        panic_led_flash( MS2u(300), MS2u(300), MS2u(800) );
        loops--;
    }
}
