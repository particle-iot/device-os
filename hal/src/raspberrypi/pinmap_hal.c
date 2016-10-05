#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>

RPi_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/*
 * pwm_capable (true if has hardware PWM)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 */

/* 00 */ { false, PIN_MODE_NONE },
/* 01 */ { false, PIN_MODE_NONE },
/* 02 */ { false, PIN_MODE_NONE },
/* 03 */ { false, PIN_MODE_NONE },
/* 04 */ { false, PIN_MODE_NONE },
/* 05 */ { false, PIN_MODE_NONE },
/* 06 */ { false, PIN_MODE_NONE },
/* 07 */ { false, PIN_MODE_NONE },
/* 08 */ { false, PIN_MODE_NONE },
/* 09 */ { false, PIN_MODE_NONE },
/* 10 */ { false, PIN_MODE_NONE },
/* 11 */ { false, PIN_MODE_NONE },
/* 12 */ { true,  PIN_MODE_NONE },
/* 13 */ { true,  PIN_MODE_NONE },
/* 14 */ { false, PIN_MODE_NONE },
/* 15 */ { false, PIN_MODE_NONE },
/* 16 */ { false, PIN_MODE_NONE },
/* 17 */ { false, PIN_MODE_NONE },
/* 18 */ { true,  PIN_MODE_NONE },
/* 19 */ { true,  PIN_MODE_NONE },
/* 20 */ { false, PIN_MODE_NONE },
/* 21 */ { false, PIN_MODE_NONE },
/* 22 */ { false, PIN_MODE_NONE },
/* 23 */ { false, PIN_MODE_NONE },
/* 24 */ { false, PIN_MODE_NONE },
/* 25 */ { false, PIN_MODE_NONE },
/* 26 */ { false, PIN_MODE_NONE },
/* 27 */ { false, PIN_MODE_NONE },
};


RPi_Pin_Info* HAL_Pin_Map() {
    return __PIN_MAP;
}

