/*
 * pnaic.c
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */

#include <stdint.h>
#include "hw_config.h"
#include "spark_macros.h"
#include "panic.h"
#include "debug.h"


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

void panic_(ePanicCode code)
{
        __disable_irq();
        // Flush any serial message to help the poor bugger debug this;
        flash_codes_t pcd = flash_codes[code];
        LED_SetRGBColor(RGB_COLOR_RED);
        LED_On(LED_RGB);
        uint16_t c;
        int loops = 2;
        if (debug_output_)(debug_output_("!"));
        LED_Off(LED_RGB);
        while(loops) {
                // preamble
            KICK_WDT();
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                Delay_Microsecond(MS2u(150));
                LED_Off(LED_RGB);
                Delay_Microsecond(MS2u(100));
            }

            Delay_Microsecond(MS2u(100));
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                Delay_Microsecond(MS2u(300));
                LED_Off(LED_RGB);
                Delay_Microsecond(MS2u(100));
            }
            Delay_Microsecond(MS2u(100));

            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                Delay_Microsecond(MS2u(150));
                LED_Off(LED_RGB);
                Delay_Microsecond(MS2u(100));
            }

                // pause
                Delay_Microsecond(MS2u(900));
                // play code
                for (c = code; c; c--) {
                    LED_SetRGBColor(pcd.led);
                    LED_On(LED_RGB);
                    Delay_Microsecond(MS2u(300));
                    LED_Off(LED_RGB);
                    Delay_Microsecond(MS2u(300));
                }
                // pause
                Delay_Microsecond(MS2u(800));
#ifdef RELEASE_BUILD
                if (--loops == 0) NVIC_SystemReset();
#endif
        }

}
