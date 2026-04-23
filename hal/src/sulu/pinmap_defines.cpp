/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

namespace {

// NOTE: remember to update following driver once the pinmap is updated
// 1. gpio_hal.cpp, cachePins[CACHE_PIN_COUNT]
static hal_pin_info_t pinmap[TOTAL_PINS] = {
/* User space */
/* D0 / SDA      - 00 */ { RTL_PORT_B,      0, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D1 / SCL      - 01 */ { RTL_PORT_A,     31, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D2 / RTS      - 02 */ { RTL_PORT_A,     14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D3 / CTS      - 03 */ { RTL_PORT_A,     15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D4            - 04 */ { RTL_PORT_B,     18, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 10,               0},
/* D5            - 05 */ { RTL_PORT_B,     19, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 11,               0},
/* D6            - 06 */ { RTL_PORT_B,     20, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 12,               0},
/* D7  / LED     - 07 */ { RTL_PORT_B,     21, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 13,               0},
/* D8  / WKP     - 08 */ { RTL_PORT_A,     19, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D9  / TX      - 09 */ { RTL_PORT_A,     12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                  0,               0},
/* D10 / RX      - 10 */ { RTL_PORT_A,     13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                  1,               0},
/* D11 / MISO    - 11 */ { RTL_PORT_A,     17, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D12 / MOSI    - 12 */ { RTL_PORT_A,     16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D13 / SCK     - 13 */ { RTL_PORT_A,     18, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D14 / A5      - 14 */ { RTL_PORT_B,      7, PIN_MODE_NONE, PF_NONE, 3,                1,                 17,               0},
/* D15 / A4      - 15 */ { RTL_PORT_B,      2, PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D16 / A3      - 16 */ { RTL_PORT_B,      1, PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D17 / A2      - 17 */ { RTL_PORT_B,      6, PIN_MODE_NONE, PF_NONE, 2,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D18 / A1      - 18 */ { RTL_PORT_B,      5, PIN_MODE_NONE, PF_NONE, 1,                1,                 9,                0},
/* D19 / A0      - 19 */ { RTL_PORT_B,      4, PIN_MODE_NONE, PF_NONE, 0,                1,                 8,                0},
/* D20 / TX1     - 20 */ { RTL_PORT_A,      7, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D21 / RX1     - 21 */ { RTL_PORT_A,      8, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},

/* System space */
/* RGBR          - 22 */ { RTL_PORT_A,     30, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                0},
/* RGBG          - 23 */ { RTL_PORT_B,     23, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                0},
/* RGBB          - 24 */ { RTL_PORT_B,     22, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                0},
/* MODE BUTTON   - 25 */ { RTL_PORT_A,     11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* TX2  (NCP)    - 26 */ { RTL_PORT_A,     21, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* RX2  (NCP)    - 27 */ { RTL_PORT_A,     22, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* RTS2 (NCP)    - 28 */ { RTL_PORT_A,     23, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* CTS2 (NCP)    - 29 */ { RTL_PORT_A,     24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* BGPWR         - 30 */ { RTL_PORT_B,     28, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* BGRST         - 31 */ { RTL_PORT_B,     29, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* BGVINT        - 32 */ { RTL_PORT_B,     31, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* BGDTR         - 33 */ { RTL_PORT_B,     30, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* LOW_BAT_UC    - 34 */ { RTL_PORT_A,      0, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* SWD_CLK       - 35 */ { RTL_PORT_B,      3, PIN_MODE_NONE, PF_NONE, 6,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* SWD_DIO       - 36 */ { RTL_PORT_A,     27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},

/* Reserved */
/* UNUSED_PIN1   - 37 */ { RTL_PORT_A,      1, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN2   - 38 */ { RTL_PORT_A,      2, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN3   - 39 */ { RTL_PORT_A,      4, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN4   - 40 */ { RTL_PORT_A,      5, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN5   - 41 */ { RTL_PORT_A,      6, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN6   - 42 */ { RTL_PORT_A,      9, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN7   - 43 */ { RTL_PORT_A,     10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* UNUSED_PIN8   - 44 */ { RTL_PORT_A,     20, PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
};

} // anonymous

hal_pin_info_t* hal_pin_map(void) {
    return pinmap;
}
