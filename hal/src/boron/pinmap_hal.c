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

#if PLATFORM_ID == PLATFORM_BORON

static Hal_Pin_Info s_pin_map[TOTAL_PINS] = {
/* D0 / SDA      - 00 */ { NRF_PORT_0, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D1 / SCL      - 01 */ { NRF_PORT_0, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D2 /          - 02 */ { NRF_PORT_1, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 3,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* D3 /          - 03 */ { NRF_PORT_1, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 3,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* D4            - 04 */ { NRF_PORT_1, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* D5            - 05 */ { NRF_PORT_1, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* D6            - 06 */ { NRF_PORT_1, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* D7            - 07 */ { NRF_PORT_1, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* D8            - 08 */ { NRF_PORT_1, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* D9            - 09 */ { NRF_PORT_0, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D10           - 10 */ { NRF_PORT_0, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D11           - 11 */ { NRF_PORT_1, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D12           - 12 */ { NRF_PORT_1, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D13           - 13 */ { NRF_PORT_1, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A5            - 14 */ { NRF_PORT_0, 31, PIN_MODE_NONE, PF_NONE, 7,                3,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* A4            - 15 */ { NRF_PORT_0, 30, PIN_MODE_NONE, PF_NONE, 6,                3,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* A3            - 16 */ { NRF_PORT_0, 29, PIN_MODE_NONE, PF_NONE, 5,                2,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* A2            - 17 */ { NRF_PORT_0, 28, PIN_MODE_NONE, PF_NONE, 4,                2,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* A1            - 18 */ { NRF_PORT_0, 4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* A0            - 19 */ { NRF_PORT_0, 3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* MODE BUTTON   - 20 */ { NRF_PORT_0, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RGBR          - 21 */ { NRF_PORT_0, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* RGBG          - 22 */ { NRF_PORT_0, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* RGBB          - 23 */ { NRF_PORT_0, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* TX1           - 24 */ { NRF_PORT_1, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RX1           - 25 */ { NRF_PORT_1, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* CTS1          - 26 */ { NRF_PORT_1, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RTS1          - 27 */ { NRF_PORT_1, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBPWR         - 28 */ { NRF_PORT_0, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBRST         - 29 */ { NRF_PORT_0, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BUFEN         - 30 */ { NRF_PORT_0, 25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* ANTSW1        - 31 */ { NRF_PORT_0, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* PMIC_SCL      - 32 */ { NRF_PORT_1, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* PMIC_SDA      - 33 */ { NRF_PORT_0, 24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBVINT        - 34 */ { NRF_PORT_0, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* LOW_BAT_UC    - 35 */ { NRF_PORT_0, 5,  PIN_MODE_NONE, PF_NONE, 3,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0}
};

const uint8_t NRF_PIN_LOOKUP_TABLE[48] = {
    PIN_INVALID, PIN_INVALID, 34,          19,          18,          35,          9,           31,          /* P0.00 ~ P0.07 */
    10,          PIN_INVALID, PIN_INVALID, 20,          29,          21,          22,          23,          /* P0.08 ~ P0.15 */
    28,          PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, /* P0.16 ~ P0.23 */
    33,          30,          0,           1,           17,          16,          15,          14,          /* P0.24 ~ P0.31 */
    PIN_INVALID, 2,           3,           8,           25,          24,          26,          27,          /* P1.00 ~ P1.07 */
    4,           32,          5,           6,           7,           12,          11,          13,          /* P1.08 ~ P1.15 */
};

#endif // PLATFORM_ID == PLATFORM_BORON

#if PLATFORM_ID == PLATFORM_BSOM

static Hal_Pin_Info s_pin_map[TOTAL_PINS] = {
/* D0 /          - 00 */ { NRF_PORT_0, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D1 /          - 01 */ { NRF_PORT_0, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D2 /          - 02 */ { NRF_PORT_1, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D3 /          - 03 */ { NRF_PORT_1, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D4            - 04 */ { NRF_PORT_1, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* D5            - 05 */ { NRF_PORT_1, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* D6            - 06 */ { NRF_PORT_1, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* D7            - 07 */ { NRF_PORT_1, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* D8            - 08 */ { NRF_PORT_1, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D9            - 09 */ { NRF_PORT_0, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D10           - 10 */ { NRF_PORT_0, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D11           - 11 */ { NRF_PORT_1, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D12           - 12 */ { NRF_PORT_1, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D13           - 13 */ { NRF_PORT_1, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A5            - 14 */ { NRF_PORT_0, 31, PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A4            - 15 */ { NRF_PORT_0, 30, PIN_MODE_NONE, PF_NONE, 6,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A3            - 16 */ { NRF_PORT_0, 29, PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A2            - 17 */ { NRF_PORT_0, 28, PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* A1            - 18 */ { NRF_PORT_0, 4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 0,                8, EXTI_CHANNEL_NONE, 0},
/* A0            - 19 */ { NRF_PORT_0, 3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* A7            - 20 */ { NRF_PORT_0, 2,  PIN_MODE_NONE, PF_NONE, 0,                2,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* A6            - 21 */ { NRF_PORT_0, 5,  PIN_MODE_NONE, PF_NONE, 3,                2,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* MODE BUTTON   - 22 */ { NRF_PORT_0, 11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RGBR          - 23 */ { NRF_PORT_0, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                8, EXTI_CHANNEL_NONE, 0},
/* RGBG          - 24 */ { NRF_PORT_0, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                8, EXTI_CHANNEL_NONE, 0},
/* RGBB          - 25 */ { NRF_PORT_0, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                8, EXTI_CHANNEL_NONE, 0},
/* NFC_PIN1      - 26 */ { NRF_PORT_0, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* NFC_PIN2      - 27 */ { NRF_PORT_0, 10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D22           - 28 */ { NRF_PORT_0, 24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* D23           - 29 */ { NRF_PORT_1, 9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* TX1           - 30 */ { NRF_PORT_1, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RX1           - 31 */ { NRF_PORT_1, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* CTS1          - 32 */ { NRF_PORT_1, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* RTS1          - 33 */ { NRF_PORT_1, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBPWR         - 34 */ { NRF_PORT_0, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBRST         - 35 */ { NRF_PORT_0, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* BUFEN         - 36 */ { NRF_PORT_0, 25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
/* UBVINT        - 37 */ { NRF_PORT_0, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, 8, EXTI_CHANNEL_NONE, 0},
};

const uint8_t NRF_PIN_LOOKUP_TABLE[48] = {
    PIN_INVALID, PIN_INVALID, 20,          19,          18,          21,          9,           37,          /* P0.00 ~ P0.07 */
    10,          26,          27,          22,          35,          23,          24,          25,          /* P0.08 ~ P0.15 */
    34,          PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, PIN_INVALID, /* P0.16 ~ P0.23 */
    28,          36,          0,           1,           17,          16,          15,          14,          /* P0.24 ~ P0.31 */
    PIN_INVALID, 3,           2,           8,           31,          30,          32,          33,          /* P1.00 ~ P1.07 */
    4,           29,          5,           6,           7,           12,          11,          13,          /* P1.08 ~ P1.15 */
};

#endif // PLATFORM_ID == PLATFORM_BSOM

Hal_Pin_Info* HAL_Pin_Map(void) {
    return s_pin_map;
}
