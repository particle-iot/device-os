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

#if HAL_PLATFORM_MESH

extern const pin_t g_pins[];
extern const char* g_pin_names[];
extern const size_t g_pin_count;

const pin_t g_pins[] = {
    SDA,
    SCL,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    TX,
    RX,
    MISO,
    MOSI,
    SCK,
    A5,
    A4,
    A3,
    A2,
    A1,
    A0,
    BTN
};

const char* g_pin_names[] = {
    "SDA",
    "SCL",
    "D2",
    "D3",
    "D4",
    "D5",
    "D6",
    "D7",
    "D8",
    "TX",
    "RX",
    "MISO",
    "MOSI",
    "SCK",
    "A5",
    "A4",
    "A3",
    "A2",
    "A1",
    "A0",
    "BTN"
};

const size_t g_pin_count = sizeof(g_pins)/sizeof(*g_pins);

#endif // HAL_PLATFORM_MESH
