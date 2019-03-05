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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10

extern const pin_t g_pins[];
extern const char* g_pin_names[];
extern const size_t g_pin_count;

/* Pins by Source
 *
 * GPIO_PinSource0: A7 (WKP), P1S0, P1S2, B2, B4
 * GPIO_PinSource1: RGBR, P1S1, P1S5, B3, B5
 * GPIO_PinSource2: A2, RGBG, C0, PWR_UC
 * GPIO_PinSource3: D4, A1, RGBB
 * GPIO_PinSource4: D3, A6 (DAC/DAC1), P1S3, RESET_UC
 * GPIO_PinSource5: D2, A0, A3 (DAC2)
 * GPIO_PinSource6: D1, A4, B1
 * GPIO_PinSource7: D0, A5, SETUP_BUTTON
 * GPIO_PinSource8: B0, C5, PM_SCL_UC
 * GPIO_PinSource9: TX, C4, PM_SDA_UC
 * GPIO_PinSource10: RX, C3, TXD_UC
 * GPIO_PinSource11: C2, RXD_UC
 * GPIO_PinSource12: C1, RI_UC
 * GPIO_PinSource13: D7, P1S4, CTS_UC, LOW_BAT_UC
 * GPIO_PinSource14: D6, RTS_UC
 * GPIO_PinSource15: D5, LVLOE_UC
 */

/* COMMON TO PHOTON, P1 and ELECTRON */
/* D0            - 00  { GPIOB, GPIO_Pin_7, GPIO_PinSource7, NONE, NONE, TIM4, TIM_Channel_2, PIN_MODE_NONE, 0, 0 }, */
/* D1            - 01  { GPIOB, GPIO_Pin_6, GPIO_PinSource6, NONE, NONE, TIM4, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
/* D2            - 02  { GPIOB, GPIO_Pin_5, GPIO_PinSource5, NONE, NONE, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 }, */
/* D3            - 03  { GPIOB, GPIO_Pin_4, GPIO_PinSource4, NONE, NONE, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
/* D4            - 04  { GPIOB, GPIO_Pin_3, GPIO_PinSource3, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* D5            - 05  { GPIOA, GPIO_Pin_15, GPIO_PinSource15, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* D6            - 06  { GPIOA, GPIO_Pin_14, GPIO_PinSource14, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* D7            - 07  { GPIOA, GPIO_Pin_13, GPIO_PinSource13, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* NOT USED      - 08  { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE }, */
/* NOT USED      - 09  { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE }, */
/* A0            - 10  { GPIOC, GPIO_Pin_5, GPIO_PinSource5, ADC_Channel_15, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* A1            - 11  { GPIOC, GPIO_Pin_3, GPIO_PinSource3, ADC_Channel_13, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* A2            - 12  { GPIOC, GPIO_Pin_2, GPIO_PinSource2, ADC_Channel_12, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* DAC2, A3      - 13  { GPIOA, GPIO_Pin_5, GPIO_PinSource5, ADC_Channel_5, DAC_Channel_2, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* A4            - 14  { GPIOA, GPIO_Pin_6, GPIO_PinSource6, ADC_Channel_6, NONE, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
/* A5            - 15  { GPIOA, GPIO_Pin_7, GPIO_PinSource7, ADC_Channel_7, NONE, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 }, */
/* DAC, DAC1, A6 - 16  { GPIOA, GPIO_Pin_4, GPIO_PinSource4, ADC_Channel_4, DAC_Channel_1, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* WKP, A7       - 17  { GPIOA, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_0, NONE, TIM5, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
/* RX            - 18  { GPIOA, GPIO_Pin_10, GPIO_PinSource10, NONE, NONE, TIM1, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* TX            - 19  { GPIOA, GPIO_Pin_9, GPIO_PinSource9, NONE, NONE, TIM1, TIM_Channel_2, PIN_MODE_NONE, 0, 0 }, */
/* SETUP BUTTON  - 20  { GPIOC, GPIO_Pin_7, GPIO_PinSource7, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* RGBR          - 21  { GPIOA, GPIO_Pin_1, GPIO_PinSource1, NONE, NONE, TIM2, TIM_Channel_2, PIN_MODE_NONE, 0, 0 }, */
/* RGBG          - 22  { GPIOA, GPIO_Pin_2, GPIO_PinSource2, NONE, NONE, TIM2, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* RGBB          - 23  { GPIOA, GPIO_Pin_3, GPIO_PinSource3, NONE, NONE, TIM2, TIM_Channel_4, PIN_MODE_NONE, 0, 0 } */
#if PLATFORM_ID == 8 // P1
/* P1S0          - 24 ,{ GPIOB, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_8, NONE, TIM3, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* P1S1          - 25  { GPIOB, GPIO_Pin_1, GPIO_PinSource1, ADC_Channel_9, NONE, TIM3, TIM_Channel_4, PIN_MODE_NONE, 0, 0 }, */
/* P1S2          - 26  { GPIOC, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_10, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* P1S3          - 27  { GPIOC, GPIO_Pin_4, GPIO_PinSource4, ADC_Channel_14, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* P1S4          - 28  { GPIOC, GPIO_Pin_13, GPIO_PinSource13, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* P1S5          - 29  { GPIOC, GPIO_Pin_1, GPIO_PinSource1, ADC_Channel_11, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* P1S6          - 30  { GPIOA, GPIO_Pin_8, GPIO_PinSource8, NONE, NONE, TIM1, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
#endif

#if PLATFORM_ID == 10 // Electron
/* B0            - 24 ,{ GPIOC, GPIO_Pin_8, GPIO_PinSource8, NONE, NONE, TIM8, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* B1            - 25  { GPIOC, GPIO_Pin_6, GPIO_PinSource6, NONE, NONE, TIM8, TIM_Channel_1, PIN_MODE_NONE, 0, 0 }, */
/* B2            - 26  { GPIOB, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_8, NONE, TIM3, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* B3            - 27  { GPIOB, GPIO_Pin_1, GPIO_PinSource1, ADC_Channel_9, NONE, TIM3, TIM_Channel_4, PIN_MODE_NONE, 0, 0 }, */
/* B4            - 28  { GPIOC, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_10, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* B5            - 29  { GPIOC, GPIO_Pin_1, GPIO_PinSource1, ADC_Channel_11, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* C0            - 30  { GPIOD, GPIO_Pin_2, GPIO_PinSource2, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* C1            - 31  { GPIOC, GPIO_Pin_12, GPIO_PinSource12, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* C2            - 32  { GPIOC, GPIO_Pin_11, GPIO_PinSource11, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* C3            - 33  { GPIOC, GPIO_Pin_10, GPIO_PinSource10, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* C4            - 34  { GPIOB, GPIO_Pin_9, GPIO_PinSource9, NONE, NONE, TIM4, TIM_Channel_4, PIN_MODE_NONE, 0, 0 }, */
/* C5            - 35  { GPIOB, GPIO_Pin_8, GPIO_PinSource8, NONE, NONE, TIM4, TIM_Channel_3, PIN_MODE_NONE, 0, 0 }, */
/* TXD_UC        - 36  { GPIOB, GPIO_Pin_10, GPIO_PinSource10, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* RXD_UC        - 37  { GPIOB, GPIO_Pin_11, GPIO_PinSource11, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* RI_UC         - 38  { GPIOB, GPIO_Pin_12, GPIO_PinSource12, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* CTS_UC        - 39  { GPIOB, GPIO_Pin_13, GPIO_PinSource13, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* RTS_UC        - 40  { GPIOB, GPIO_Pin_14, GPIO_PinSource14, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* PWR_UC        - 41  { GPIOB, GPIO_Pin_2, GPIO_PinSource2, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* RESET_UC      - 42  { GPIOC, GPIO_Pin_4, GPIO_PinSource4, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* LVLOE_UC      - 43  { GPIOB, GPIO_Pin_15, GPIO_PinSource15, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* PM_SDA_UC     - 44  { GPIOC, GPIO_Pin_9, GPIO_PinSource9, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* PM_SCL_UC     - 45  { GPIOA, GPIO_Pin_8, GPIO_PinSource8, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
/* LOW_BAT_UC    - 46  { GPIOC, GPIO_Pin_13, GPIO_PinSource13, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, */
#endif

const pin_t g_pins[] = {
    A7,
#if PLATFORM_ID == 10
    B3,
#endif
#if PLATFORM_ID == 8
    P1S1,
#endif
    A2,
    D4,
    D3,
    D2,
    D1,
    BTN,
#if PLATFORM_ID == 10
    B0,
#endif
    TX,
    RX,
#if PLATFORM_ID == 10
    C2,
    C1,
#endif
#if PLATFORM_ID == 8
    P1S4,
#endif
    D6,
    D5
};

const char* g_pin_names[] = {
    "A7",
#if PLATFORM_ID == 10
    "B3",
#endif
#if PLATFORM_ID == 8
    "P1S1",
#endif
    "A2",
    "D4",
    "D3",
    "D2",
    "D1",
    "SETUP button",
#if PLATFORM_ID == 10
    "B0",
#endif
    "TX",
    "RX",
#if PLATFORM_ID == 10
    "C2",
    "C1",
#endif
#if PLATFORM_ID == 8
    "P1S4"
#endif
    "D6",
    "D5"
};

const size_t g_pin_count = sizeof(g_pins)/sizeof(*g_pins);

#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10
