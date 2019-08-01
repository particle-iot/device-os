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

#if PLATFORM_ID == PLATFORM_BORON

#define TOTAL_PINS 36
#define TOTAL_ANALOG_PINS 6
#define FIRST_ANALOG_PIN D14

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
#define D16 16
#define D17 17
#define D18 18
#define D19 19

// Analog pins
#define A0 D19
#define A1 D18
#define A2 D17
#define A3 D16
#define A4 D15
#define A5 D14

#define SS D14
#define SCK D13
#define MISO D11
#define MOSI D12

#define SDA D0
#define SCL D1

#define TX D9
#define RX D10
#define CTS D3
#define RTS D2

#define TX1 24
#define RX1 25
#define CTS1 26
#define RTS1 27

#define BTN 20
#define RGBR 21
#define RGBG 22
#define RGBB 23

#define WKP D8

#define UBPWR 28
#define UBRST 29
#define BUFEN 30
#define ANTSW1 31
#define PMIC_SCL 32
#define PMIC_SDA 33
#define UBVINT 34
#define LOW_BAT_UC 35

#endif // PLATFORM_ID == PLATFORM_BORON

#if PLATFORM_ID == PLATFORM_BSOM

#define TOTAL_PINS 38
#define TOTAL_ANALOG_PINS 8
#define FIRST_ANALOG_PIN D14

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
#define D16 16
#define D17 17
#define D18 18
#define D19 19
#define D20 20
#define D21 21
#define D22 28
#define D23 29

// Analog pins
#define A0 D19
#define A1 D18
#define A2 D17
#define A3 D16
#define A4 D15
#define A5 D14
#define A6 D21
#define A7 D20

#define SS D8
#define SCK D13
#define MISO D11
#define MOSI D12

#define SDA D0
#define SCL D1

#define TX D9
#define RX D10
#define CTS D3
#define RTS D2

#define TX1 30
#define RX1 31
#define CTS1 32
#define RTS1 33

#define BTN 22
#define RGBR 23
#define RGBG 24
#define RGBB 25

#define WKP A7

#define NFC_PIN1 26
#define NFC_PIN2 27

#define LOW_BAT_UC A6

#define UBPWR 34
#define UBRST 35
#define BUFEN 36
#define UBVINT 37

#endif // PLATFORM_ID == PLATFORM_BSOM
