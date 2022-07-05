/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

static hal_pin_info_t s_pin_map[TOTAL_PINS] = {
/* D0            - 00 */ { NRF_PORT_0,    26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D1            - 01 */ { NRF_PORT_0,    27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D2            - 02 */ { NRF_PORT_1,     2, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D3            - 03 */ { NRF_PORT_1,    10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D4            - 04 */ { NRF_PORT_0,    12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* D5            - 05 */ { NRF_PORT_0,    24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* D6            - 06 */ { NRF_PORT_1,    4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* D7            - 07 */ { NRF_PORT_0,    13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* D8            - 08 */ { NRF_PORT_0,    7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D9            - 09 */ { NRF_PORT_0,    6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D10           - 10 */ { NRF_PORT_0,    8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D11           - 11 */ { NRF_PORT_1,    8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D12           - 12 */ { NRF_PORT_1,    9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D13           - 13 */ { NRF_PORT_0,    11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A5            - 14 */ { NRF_PORT_0,    31, PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A4            - 15 */ { NRF_PORT_0,    30, PIN_MODE_NONE, PF_NONE, 6,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A3            - 16 */ { NRF_PORT_0,    29, PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A2            - 17 */ { NRF_PORT_0,    28, PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A1            - 18 */ { NRF_PORT_0,    4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* A0            - 19 */ { NRF_PORT_0,    3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* A7            - 20 */ { NRF_PORT_0,    2,  PIN_MODE_NONE, PF_NONE, 0,                2,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* A6            - 21 */ { NRF_PORT_0,    5,  PIN_MODE_NONE, PF_NONE, 3,                2,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* D22           - 22 */ { NRF_PORT_1,    1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D23           - 23 */ { NRF_PORT_1,    3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* MODE BUTTON   - 24 */ { NRF_PORT_0,    25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RGBR          - 25 */ { NRF_PORT_0,    16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* RGBG          - 26 */ { NRF_PORT_0,    15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* RGBB          - 27 */ { NRF_PORT_0,    14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* TX1           - 28 */ { NRF_PORT_1,    15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RX1           - 29 */ { NRF_PORT_1,    14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* CTS1          - 30 */ { NRF_PORT_1,    12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RTS1          - 31 */ { NRF_PORT_1,    13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BGPWR         - 32 */ { NRF_PORT_1,    6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BGRST         - 33 */ { NRF_PORT_1,    7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BGVINT        - 34 */ { NRF_PORT_1,    11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BGDTR         - 35 */ { NRF_PORT_1,    5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* NFC1          - 36 */ { NRF_PORT_0,    9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* NFC2          - 37 */ { NRF_PORT_0,    10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
};

const uint8_t NRF_PIN_LOOKUP_TABLE[48] = {
    /* P0.00     P0.01        P0.02        P0.03        P0.04        P0.05        P0.06        P0.07*/
    PIN_INVALID, PIN_INVALID, 20,          19,          18,          21,          9,           8,            /* P0.00 ~ P0.07 */
    /* P0.08     P0.09        P0.10        P0.11        P0.12        P0.13        P0.14        P0.15*/
    10,          36,          37,          13,          4,           7,           27,          26,           /* P0.08 ~ P0.15 */
    /* P0.16     P0.17        P0.18        P0.19        P0.20        P0.21        P0.22        P0.23*/
    25,          PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID,  /* P0.16 ~ P0.23 */
    /* P0.24     P0.25        P0.26        P0.27        P0.28        P0.29        P0.30        P0.31*/
    5,           24,          0,           1,           17,          16,          15,          14,           /* P0.24 ~ P0.31 */
    /* P1.00     P1.01        P1.02        P1.03        P1.04        P1.05        P1.06        P1.07*/
    PIN_INVALID, 22,          2,           23,          6,           35,          32,          33,           /* P1.00 ~ P1.07 */
    /* P1.08     P1.09        P1.10        P1.11        P1.12        P1.13        P1.14        P1.15*/
    11,          12,          3,           34,          30,          31,          29,          28,           /* P1.08 ~ P1.15 */
};

hal_pin_info_t* hal_pin_map(void) {
    return s_pin_map;
}
