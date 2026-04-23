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

#pragma once

#define TOTAL_PINS          45
#define TOTAL_ANALOG_PINS   6
#define FIRST_ANALOG_PIN    14

// Digital pins
#define D0                  0
#define D1                  1
#define D2                  2
#define D3                  3
#define D4                  4
#define D5                  5
#define D6                  6
#define D7                  7
#define D8                  8
#define D9                  9
#define D10                 10
#define D11                 11
#define D12                 12
#define D13                 13
#define D14                 14
#define D15                 15
#define D16                 16
#define D17                 17
#define D18                 18
#define D19                 19
#define D20                 20
#define D21                 21

// Analog pins
#define A0                  D19
#define A1                  D18
#define A2                  D17
#define A3                  D16
#define A4                  D15
#define A5                  D14

// SPI
#define SS                  D8
#define SCK                 D13
#define MISO                D11
#define MOSI                D12

#define SS1                 D3
#define SCK1                D2
#define MISO1               D10
#define MOSI1               D9

// I2C
#define SDA                 D0
#define SCL                 D1

// UART (Serial1)
#define TX                  D9
#define RX                  D10
#define CTS                 D3
#define RTS                 D2
// UART (Serial2)
#define TX1                 D20
#define RX1                 D21
// UART (NCP)
#define TX2                 26
#define RX2                 27
#define CTS2                29
#define RTS2                28

#define WKP                 D8

// RGB and Button
#define RGBR                22
#define RGBG                23
#define RGBB                24
#define BTN                 25

// Cellular
#define BGPWR               30
#define BGRST               31
#define BGVINT              32
#define BGDTR               33

#define LOW_BAT_UC          34

// Set it to PIN_INVALID if not present
#define SWD_DAT             36
#define SWD_CLK             35
