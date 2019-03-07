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

#include "pinmapping.h"
#include "platforms.h"

#if (PLATFORM_ID >= PLATFORM_SPARK_CORE && PLATFORM_ID <= PLATFORM_ELECTRON_PRODUCTION) && PLATFORM_ID != PLATFORM_GCC

const PinMapping g_pinmap[] = {
    {"D0", D0},
    {"D1", D1},
    {"D2", D2},
    {"D3", D3},
    {"D4", D4},
    {"D5", D5},
    {"D6", D6},
    {"D7", D7},
    {"A0", A0},
    {"A1", A1},
    {"A2", A2},
    {"A3", A3},
    {"A4", A4},
    {"A5", A5},
    {"A6", A6},
    {"A7", A7},
    {"RX", RX},
    {"TX", TX},
    {"BTN", BTN},
    {"WKP", WKP},
    {"SS", SS},
    {"SCK", SCK},
    {"MISO", MISO},
    {"MOSI", MOSI},
    {"SDA", SDA},
    {"SCL", SCL},
    {"DAC1", DAC1},
    {"DAC2", DAC2}

#if PLATFORM_ID == PLATFORM_P1
    ,
    {"P1S0", P1S0},
    {"P1S1", P1S1},
    {"P1S2", P1S2},
    {"P1S3", P1S3},
    {"P1S4", P1S4},
    {"P1S5", P1S5},
    {"P1S6", P1S6}
#endif // PLATFORM_ID == PLATFORM_P1

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    ,
    {"B0", B0},
    {"B1", B1},
    {"B2", B2},
    {"B3", B3},
    {"B4", B4},
    {"B5", B5},
    {"C0", C0},
    {"C1", C1},
    {"C2", C2},
    {"C3", C3},
    {"C4", C4},
    {"C5", C5}
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
};

const size_t g_pin_count = sizeof(g_pinmap) / sizeof(*g_pinmap);

#endif // (PLATFORM_ID >= PLATFORM_SPARK_CORE && PLATFORM_ID <= PLATFORM_ELECTRON_PRODUCTION) && PLATFORM_ID != PLATFORM_GCC
