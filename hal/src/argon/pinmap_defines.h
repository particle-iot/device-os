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

#if PLATFORM_ID == PLATFORM_ARGON

#ifdef __cplusplus

constexpr pin_t TOTAL_PINS = 36;

// Digital pins
constexpr pin_t D0 = 0;
constexpr pin_t D1 = 1;
constexpr pin_t D2 = 2;
constexpr pin_t D3 = 3;
constexpr pin_t D4 = 4;
constexpr pin_t D5 = 5;
constexpr pin_t D6 = 6;
constexpr pin_t D7 = 7;
constexpr pin_t D8 = 8;
constexpr pin_t D9 = 9;
constexpr pin_t D10 = 10;
constexpr pin_t D11 = 11;
constexpr pin_t D12 = 12;
constexpr pin_t D13 = 13;
constexpr pin_t D14 = 14;
constexpr pin_t D15 = 15;
constexpr pin_t D16 = 16;
constexpr pin_t D17 = 17;
constexpr pin_t D18 = 18;
constexpr pin_t D19 = 19;

constexpr pin_t BTN = 20;
constexpr pin_t RGBR = 21;
constexpr pin_t RGBG = 22;
constexpr pin_t RGBB = 23;

// Analog pins
constexpr pin_t TOTAL_ANALOG_PINS = 6;
constexpr pin_t FIRST_ANALOG_PIN = D14;

constexpr pin_t A0 = D19;
constexpr pin_t A1 = D18;
constexpr pin_t A2 = D17;
constexpr pin_t A3 = D16;
constexpr pin_t A4 = D15;
constexpr pin_t A5 = D14;

constexpr pin_t SS = D14;
constexpr pin_t SCK = D13;
constexpr pin_t MISO = D11;
constexpr pin_t MOSI = D12;

constexpr pin_t SDA = D0;
constexpr pin_t SCL = D1;

constexpr pin_t TX = D9;
constexpr pin_t RX = D10;
constexpr pin_t CTS = D3;
constexpr pin_t RTS = D2;

constexpr pin_t WKP = D8;

constexpr pin_t TX1 = 24;
constexpr pin_t RX1 = 25;
constexpr pin_t CTS1 = 26;
constexpr pin_t RTS1 = 27;

constexpr pin_t ESPBOOT = 28;
constexpr pin_t ESPEN = 29;
constexpr pin_t HWAKE = 30;
constexpr pin_t ANTSW1 = 31;
constexpr pin_t ANTSW2 = 32;
constexpr pin_t BATT = 33;
constexpr pin_t PWR = 34;
constexpr pin_t CHG = 35;

#else // !__cplusplus

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

#define BTN 20
#define RGBR 21
#define RGBG 22
#define RGBB 23

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

#define WKP D8

#define TX1 24
#define RX1 25
#define CTS1 26
#define RTS1 27

#define ESPBOOT 28
#define ESPEN 29
#define HWAKE 30
#define ANTSW1 31
#define ANTSW2 32
#define BATT 33
#define PWR 34
#define CHG 35

#endif // !__cplusplus

#endif // PLATFORM_ID == PLATFORM_ARGON

#if PLATFORM_ID == PLATFORM_ASOM

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
#define D24 37

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

#define ESPBOOT 34
#define ESPEN 35
#define HWAKE 36

#endif // PLATFORM_ID == PLATFORM_ASOM
