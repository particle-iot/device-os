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

namespace {

static hal_pin_info_t pinmap[TOTAL_PINS] = {
/* User space */
/* D0 / A3       - 00 */ { RTL_PORT_B, 6,  PIN_MODE_NONE, PF_NONE, 2,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D1 / A4       - 01 */ { RTL_PORT_B, 5,  PIN_MODE_NONE, PF_NONE, 1,                1,                 9,                0},
/* D2            - 02 */ { RTL_PORT_A, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D3            - 03 */ { RTL_PORT_A, 17, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D4            - 04 */ { RTL_PORT_A, 18, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D5            - 05 */ { RTL_PORT_A, 19, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D6            - 06 */ { RTL_PORT_B, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D7            - 07 */ { RTL_PORT_A, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D8            - 08 */ { RTL_PORT_A, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D9            - 09 */ { RTL_PORT_A, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* D10           - 10 */ { RTL_PORT_A, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* A0            - 11 */ { RTL_PORT_B, 1,  PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* A1            - 12 */ { RTL_PORT_B, 2,  PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* A2            - 13 */ { RTL_PORT_B, 7,  PIN_MODE_NONE, PF_NONE, 3,                1,                 17,               0},
/* A5            - 14 */ { RTL_PORT_B, 4,  PIN_MODE_NONE, PF_NONE, 0,                1,                 8,                0},
/* S0            - 15 */ { RTL_PORT_A, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                0},
/* S1            - 16 */ { RTL_PORT_A, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                0},
/* S2            - 17 */ { RTL_PORT_A, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* S3            - 18 */ { RTL_PORT_B, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* S4            - 19 */ { RTL_PORT_A, 0,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* S5            - 20 */ { RTL_PORT_B, 29, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* S6            - 21 */ { RTL_PORT_B, 31, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},

/* System space */
/* RGBR          - 22 */ { RTL_PORT_A,    30, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                0},
/* RGBG          - 23 */ { RTL_PORT_B,    23, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                0},
/* RGBB          - 24 */ { RTL_PORT_B,    22, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                0},
/* MODE BUTTON   - 25 */ { RTL_PORT_A,    4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* ANTSW         - 26 */ { RTL_PORT_A,    2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
/* VBAT_MEAS/A6  - 27 */ { RTL_PORT_NONE, 0,  PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 0},
};

} // anonymous

hal_pin_info_t* hal_pin_map(void) {
    return pinmap;
}
