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

#include "platform_headers.h"
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
#include "check.h"

#define PANIC_LED_COLOR RGB_COLOR_RED

LOG_SOURCE_CATEGORY("panic");

static void panic_internal(const ePanicCode code, const char* text);

static PanicHook panicHook = NULL;

#define PANIC_DATA_MARKER (0xb33fc0ff)

retained_system struct {
    uint32_t marker;
    PanicData data;
} g_retainedPanicData;

/****************************************************************************
* Public Functions
****************************************************************************/

void panic_set_hook(const PanicHook panicHookFunction, void* reserved)
{
    //store the new hook
    panicHook = panicHookFunction;
}

void panic_(const ePanicCode code, const char* text, void* unused) {
    PANIC_COMPAT(code, text);
}

void panic_ext(const PanicData* data, void* reserved)
{
    #if HAL_PLATFORM_CORE_ENTER_PANIC_MODE
        HAL_Core_Enter_Panic_Mode(NULL);
    #else
        HAL_disable_irq();
    #endif // HAL_PLATFORM_CORE_ENTER_PANIC_MODE

    memset(&g_retainedPanicData, 0, sizeof(g_retainedPanicData));
    memcpy(&g_retainedPanicData.data, data, sizeof(PanicData));
    g_retainedPanicData.marker = PANIC_DATA_MARKER;

    //run the panic hook (if present)! this func doens't have to
    //return and can handle the system state / exit method on its own if required
    if( panicHook ) {
        panicHook(data->code, data->text);
    }

    //always run the internal function if the user func returns
    //this flashes the LEDs in the fixed panic pattern
    panic_internal(data->code, data->text);

    #if defined(RELEASE_BUILD) || PANIC_BUT_KEEP_CALM == 1
        HAL_Core_System_Reset_Ex(RESET_REASON_PANIC, data->code, NULL);
    #endif
}

int panic_get_last_panic_data(PanicData* panic, void* reserved) {
    CHECK_TRUE(panic, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(g_retainedPanicData.marker == PANIC_DATA_MARKER && g_retainedPanicData.data.size > 0, SYSTEM_ERROR_NOT_FOUND);
    memcpy(panic, &g_retainedPanicData.data, panic->size <= g_retainedPanicData.data.size ? panic->size : g_retainedPanicData.data.size);
    return 0;
}

void panic_set_last_panic_data_handled(void* reserved) {
    g_retainedPanicData.data.flags |= PANIC_DATA_FLAG_HANDLED;
}

/****************************************************************************
* Private Functions
****************************************************************************/

static void panic_led_flash(const uint32_t loopCount, const uint32_t onMS, const uint32_t ofMS, const uint32_t pauseMS) 
{
    for (uint32_t c = loopCount; c; c--) {
        LED_On(PARTICLE_LED_RGB);
        HAL_Delay_Microseconds(onMS);
        LED_Off(PARTICLE_LED_RGB);
        HAL_Delay_Microseconds(ofMS);
    }
    HAL_Delay_Microseconds(pauseMS);
}

static void panic_internal(const ePanicCode code, const char* text)
{
    LED_SetRGBColor(RGB_COLOR_RED);
    LED_SetBrightness(DEFAULT_LED_RGB_BRIGHTNESS);
    LED_Signaling_Stop();
    LED_Off(PARTICLE_LED_RGB);
    LED_SetRGBColor(PANIC_LED_COLOR);

    for(int i = 0; i < 2; i++)
    {
        // preamble - SOS pattern
        panic_led_flash( 3, MS2u(150), MS2u(100), MS2u(100) );
        panic_led_flash( 3, MS2u(300), MS2u(100), MS2u(100) );
        panic_led_flash( 3, MS2u(150), MS2u(100), MS2u(900) );
        //flash the CODE in pulses
        panic_led_flash( code, MS2u(300), MS2u(300), MS2u(800) );
    }
}
