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

#pragma once

#if PLATFORM_ID == PLATFORM_QSOM

#define TOTAL_PINS 38
#define TOTAL_ANALOG_PINS 8
#define FIRST_ANALOG_PIN A0

// Digital pins
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define D14 14
#define D15 15

// Analog pins
#define A0 16
#define A1 17
#define A2 18
#define A3 19
#define A4 20
#define A5 21
#define A6 22
#define A7 23

#define BTN 24
#define RGBR 25
#define RGBG 26
#define RGBB 27

#define TX 9
#define RX 10
#define CTS 3
#define RTS 2

#define SS D5
#define SCK D2
#define MISO D4
#define MOSI D3

#define SDA D0
#define SCL D1

#define WKP A7

#define TX1 28
#define RX1 29
#define CTS1 30
#define RTS1 31

#define BGPWR 32
#define BGRST 33
#define BGVINT 34
#define BGDTR 35

#define NFC_PIN1 36
#define NFC_PIN2 37

#endif // PLATFORM_ID == PLATFORM_QSOM
