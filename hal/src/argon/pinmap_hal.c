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

#include "pinmap_hal.h"

#if PLATFORM_ID == PLATFORM_ARGON

static Hal_Pin_Info s_pin_map[TOTAL_PINS] = {
/* D0 / SDA      - 00 */ { NRF_PORT_0, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D1 / SCL      - 01 */ { NRF_PORT_0, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D2 /          - 02 */ { NRF_PORT_1, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 3,                 0,                8, EXTI_CHANNEL_NONE},
/* D3 /          - 03 */ { NRF_PORT_1, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 3,                 1,                8, EXTI_CHANNEL_NONE},
/* D4            - 04 */ { NRF_PORT_1, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                8, EXTI_CHANNEL_NONE},
/* D5            - 05 */ { NRF_PORT_1, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                8, EXTI_CHANNEL_NONE},
/* D6            - 06 */ { NRF_PORT_1, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2,                8, EXTI_CHANNEL_NONE},
/* D7            - 07 */ { NRF_PORT_1, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 0,                8, EXTI_CHANNEL_NONE},
/* D8            - 08 */ { NRF_PORT_1, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 3,                8, EXTI_CHANNEL_NONE},
/* D9            - 09 */ { NRF_PORT_0, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D10           - 10 */ { NRF_PORT_0, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D11           - 11 */ { NRF_PORT_1, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D12           - 12 */ { NRF_PORT_1, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D13           - 13 */ { NRF_PORT_1, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A5            - 14 */ { NRF_PORT_0, 31, PIN_MODE_NONE, PF_NONE, 7,                3,                 2,                8, EXTI_CHANNEL_NONE},
/* A4            - 15 */ { NRF_PORT_0, 30, PIN_MODE_NONE, PF_NONE, 6,                3,                 3,                8, EXTI_CHANNEL_NONE},
/* A3            - 16 */ { NRF_PORT_0, 29, PIN_MODE_NONE, PF_NONE, 5,                2,                 0,                8, EXTI_CHANNEL_NONE},
/* A2            - 17 */ { NRF_PORT_0, 28, PIN_MODE_NONE, PF_NONE, 4,                2,                 1,                8, EXTI_CHANNEL_NONE},
/* A1            - 18 */ { NRF_PORT_0, 4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 2,                8, EXTI_CHANNEL_NONE},
/* A0            - 19 */ { NRF_PORT_0, 3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 3,                8, EXTI_CHANNEL_NONE},
/* MODE BUTTON   - 20 */ { NRF_PORT_0, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RGBR          - 21 */ { NRF_PORT_0, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                8, EXTI_CHANNEL_NONE},
/* RGBG          - 22 */ { NRF_PORT_0, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                8, EXTI_CHANNEL_NONE},
/* RGBB          - 23 */ { NRF_PORT_0, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                8, EXTI_CHANNEL_NONE},
/* TX1           - 24 */ { NRF_PORT_1, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RX1           - 25 */ { NRF_PORT_1, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* CTS1          - 26 */ { NRF_PORT_1, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RTS1          - 27 */ { NRF_PORT_1, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ESPBOOT       - 28 */ { NRF_PORT_0, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ESPEN         - 29 */ { NRF_PORT_0, 24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* HWAKE         - 30 */ { NRF_PORT_0, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ANTSW1        - 31 */ { NRF_PORT_0, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ANTSW2        - 32 */ { NRF_PORT_0, 25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* BATT          - 33 */ { NRF_PORT_0, 5,  PIN_MODE_NONE, PF_NONE, 3,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* PWR           - 34 */ { NRF_PORT_0, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* CHG           - 35 */ { NRF_PORT_1, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
};

const uint8_t NRF_PIN_LOOKUP_TABLE[48] = {
    PIN_INVALID, PIN_INVALID, 31,          19,          18,          33,          9,           30,          /* P0.00 ~ P0.07 */
    10,          PIN_INVALID, PIN_INVALID, 20,          34,          21,          22,          23,          /* P0.08 ~ P0.15 */
    28,          PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, /* P0.16 ~ P0.23 */
    29,          32,          0,           1,           17,          16,          15,          14,          /* P0.24 ~ P0.31 */
    PIN_INVALID, 2,           3,           8,           25,          24,          26,          27,          /* P1.00 ~ P1.07 */
    4,           35,          5,           6,           7,           12,          11,          13,          /* P1.08 ~ P1.15 */
};

#endif // PLATFORM_ID == PLATFORM_ARGON

#if PLATFORM_ID == PLATFORM_ASOM

static Hal_Pin_Info s_pin_map[TOTAL_PINS] = {
/* D0            - 00 */ { NRF_PORT_1, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D1            - 01 */ { NRF_PORT_0, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D2            - 02 */ { NRF_PORT_0, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D3            - 03 */ { NRF_PORT_0, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D4            - 04 */ { NRF_PORT_0, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                8, EXTI_CHANNEL_NONE},
/* D5            - 05 */ { NRF_PORT_1, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                8, EXTI_CHANNEL_NONE},
/* D6            - 06 */ { NRF_PORT_1, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2,                8, EXTI_CHANNEL_NONE},
/* D7            - 07 */ { NRF_PORT_1, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 0,                8, EXTI_CHANNEL_NONE},
/* D8            - 08 */ { NRF_PORT_0, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D9            - 09 */ { NRF_PORT_0, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D10           - 10 */ { NRF_PORT_0, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D11           - 11 */ { NRF_PORT_1, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D12           - 12 */ { NRF_PORT_0, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D13           - 13 */ { NRF_PORT_0, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A5            - 14 */ { NRF_PORT_0, 31, PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A4            - 15 */ { NRF_PORT_0, 30, PIN_MODE_NONE, PF_NONE, 6,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A3            - 16 */ { NRF_PORT_0, 29, PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A2            - 17 */ { NRF_PORT_0, 28, PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* A1            - 18 */ { NRF_PORT_0, 4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 0,                8, EXTI_CHANNEL_NONE},
/* A0            - 19 */ { NRF_PORT_0, 3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 1,                8, EXTI_CHANNEL_NONE},
/* A7            - 20 */ { NRF_PORT_0, 2,  PIN_MODE_NONE, PF_NONE, 0,                2,                 2,                8, EXTI_CHANNEL_NONE},
/* A6            - 21 */ { NRF_PORT_0, 5,  PIN_MODE_NONE, PF_NONE, 3,                2,                 3,                8, EXTI_CHANNEL_NONE},
/* MODE BUTTON   - 22 */ { NRF_PORT_0, 25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RGBR          - 23 */ { NRF_PORT_1, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                8, EXTI_CHANNEL_NONE},
/* RGBG          - 24 */ { NRF_PORT_1, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                8, EXTI_CHANNEL_NONE},
/* RGBB          - 25 */ { NRF_PORT_1, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                8, EXTI_CHANNEL_NONE},
/* NFC_PIN1      - 26 */ { NRF_PORT_0, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* NFC_PIN2      - 27 */ { NRF_PORT_0, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D22           - 28 */ { NRF_PORT_1, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D23           - 29 */ { NRF_PORT_1, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* TX1           - 30 */ { NRF_PORT_1, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RX1           - 31 */ { NRF_PORT_1, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* CTS1          - 32 */ { NRF_PORT_1, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* RTS1          - 33 */ { NRF_PORT_1, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ESPBOOT       - 34 */ { NRF_PORT_0, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* ESPEN         - 35 */ { NRF_PORT_0, 24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* HWAKE         - 36 */ { NRF_PORT_0, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
/* D24           - 37 */ { NRF_PORT_1, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE},
};

const uint8_t NRF_PIN_LOOKUP_TABLE[48] = {
    PIN_INVALID, PIN_INVALID, 20,          19,          18,          21,          9,           12,          /* P0.00 ~ P0.07 */
    10,          26,          27,          8,           1,           2,           3,           36,          /* P0.08 ~ P0.15 */
    34,          PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, /* P0.16 ~ P0.23 */
    35,          22,          4,           13,          17,          16,          15,          14,          /* P0.24 ~ P0.31 */
    PIN_INVALID, 28,          37,          29,          31,          30,          32,          33,          /* P1.00 ~ P1.07 */
    11,          0,           5,           6,           7,           25,          24,          23,          /* P1.08 ~ P1.15 */
};

#endif // PLATFORM_ID == PLATFORM_ASOM

Hal_Pin_Info* HAL_Pin_Map(void) {
    return s_pin_map;
}
