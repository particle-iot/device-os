/*
 * pnaic.c
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
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


#define LOOPSPERMSEC 5483

typedef struct {
        uint32_t  led;
        uint16_t  count;
} flash_codes_t;


#define def_panic_codes(_class,led,code) {led, code},
static const flash_codes_t flash_codes[] = {
                 {0,0}, //NotUsedPanicCode
#include "panic_codes.h"
};
#undef def_panic_codes

/****************************************************************************
* Public Functions
****************************************************************************/

void panic_(ePanicCode code, void* extraInfo, void (*HAL_Delay_Microseconds)(uint32_t))
{

#if HAL_PLATFORM_CORE_ENTER_PANIC_MODE
        HAL_Core_Enter_Panic_Mode(NULL);
#else
        HAL_disable_irq();
#endif // HAL_PLATFORM_CORE_ENTER_PANIC_MODE

        // Flush any serial message to help the poor bugger debug this;
        flash_codes_t pcd = flash_codes[code];
        LED_SetRGBColor(RGB_COLOR_RED);
        LED_SetBrightness(DEFAULT_LED_RGB_BRIGHTNESS);
        LED_Signaling_Stop();
        uint16_t c;
        int loops = 2;
        LOG_PRINT(TRACE, "!");
        LED_Off(LED_RGB);
        while(loops) {
                // preamble
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(150));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }

            HAL_Delay_Microseconds(MS2u(100));
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }
            HAL_Delay_Microseconds(MS2u(100));

            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(150));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }

            // pause
            HAL_Delay_Microseconds(MS2u(900));
            // play code
            for (c = code; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
            }
            // pause
            HAL_Delay_Microseconds(MS2u(800));
#if defined(RELEASE_BUILD) || PANIC_BUT_KEEP_CALM == 1
            if (--loops == 0) HAL_Core_System_Reset_Ex(RESET_REASON_PANIC, code, NULL);
#endif
        }

}
