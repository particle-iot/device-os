/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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
#define TOTAL_ANALOG_PINS   7
#define FIRST_ANALOG_PIN    19

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
#define D22                 22
#define D23                 23
#define D24                 24
#define D25                 25
#define D26                 26
#define D27                 27
#define D28                 28
#define D29                 29

// Analog pins
#define A0                  D19
#define A1                  D18
#define A2                  D17
#define A3                  D16
#define A4                  D15
#define A5                  D14
#define A6                  D29
#define A7                  D28

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
#define TX1                 D24
#define RX1                 D25
// UART (NCP)
#define TX2                 30
#define RX2                 31
#define CTS2                33
#define RTS2                32

#define WKP                 D28

// RGB and Button
#define RGBR                34
#define RGBG                35
#define RGBB                36
#define BTN                 37

// Cellular
#define BGPWR               38
#define BGRST               39
#define BGVINT              40
#define BGDTR               41
#define GNSS_ANT_PWR        42

#define UNUSED_PIN1         43
#define UNUSED_PIN2         44

// EVT
#define LOW_BAT_DEPRECATED  A5
// DVT
#define LOW_BAT_UC          A6

// Set it to PIN_INVALID if not present
#define SWD_DAT             D27
#define SWD_CLK             D14
