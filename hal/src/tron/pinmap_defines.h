/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#define TOTAL_PINS          28
#define TOTAL_ANALOG_PINS   6
#define FIRST_ANALOG_PIN    12

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

// Analog pins
#define A0                  D12
#define A1                  D13
#define A2                  D14
#define A3                  D0
#define A4                  D1
#define A5                  D15

// RGB and Button
#define RGBR                23
#define RGBG                24
#define RGBB                25
#define BTN                 26

// SPI, Shared with UART1
#define SS                  D5
#define SCK                 D4
#define MISO                D3
#define MOSI                D2

#define SS1                 A2
#define SCK1                A3
#define MISO1               A4
#define MOSI1               A5

// I2C
#define SDA                 D0
#define SCL                 D1

// UART
#define TX                  D9
#define RX                  D10
// Shared with SPI1
#define TX1                 D4
#define RX1                 D5
#define CTS1                D3
#define RTS1                D2

#define WKP                 D11

#define ANTSW               27
