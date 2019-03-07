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
#include "hal_platform.h"
#include "platforms.h"

#if HAL_PLATFORM_MESH

const PinMapping g_pinmap[] = {
    {"D0", D0},
    {"D1", D1},
    {"D2", D2},
    {"D3", D3},
    {"D4", D4},
    {"D5", D5},
    {"D6", D6},
    {"D7", D7},
    {"D8", D8},
    {"D9", D9},
    {"D10", D10},
    {"D11", D11},
    {"D12", D12},
    {"D13", D13},
    {"D14", D14},
    {"D15", D15},
    {"D16", D16},
    {"D17", D17},
    {"D18", D18},
    {"D19", D19},
    {"A0", A0},
    {"A1", A1},
    {"A2", A2},
    {"A3", A3},
    {"A4", A4},
    {"A5", A5},
    {"SCK", SCK},
    {"MISO", MISO},
    {"MOSI", MOSI},
    {"SDA", SDA},
    {"SCL", SCL},
    {"TX", TX},
    {"RX", RX},
    {"BTN", BTN}
#if PLATFORM_ID == PLATFORM_XENON_SOM || PLATFORM_ID == PLATFORM_ARGON_SOM || PLATFORM_ID == PLATFORM_BORON_SOM
    ,
    {"D20", D20},
    {"D21", D21},
    {"D22", D22},
    {"D23", D23}
#if PLATFORM_ID == PLATFORM_ARGON_SOM
    ,
    {"D24", D24}
#elif PLATFORM_ID == PLATFORM_XENON_SOM
    ,
    {"D24", D24},
    {"D24", D25},
    {"D24", D26},
    {"D24", D27},
    {"D24", D28},
    {"D24", D29},
    {"D24", D30},
    {"D24", D31}
#endif // PLATFORM_ID == PLATFORM_XENON_SOM
    ,
    {"A6", A6},
    {"A7", A7}
#endif // PLATFORM_ID == PLATFORM_XENON_SOM || PLATFORM_ID == PLATFORM_ARGON_SOM || PLATFORM_ID == PLATFORM_BORON_SOM
};

const size_t g_pin_count = sizeof(g_pinmap) / sizeof(*g_pinmap);

#endif // HAL_PLATFORM_MESH
