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

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#define TOTAL_PINS 47
#define TOTAL_DAC_PINS 2
#define TOTAL_ANALOG_PINS 12
#define FIRST_ANALOG_PIN 10
#define TOTAL_ESSENTIAL_PINS 20
#define HAS_EXTRA_PINS 1
#define FIRST_EXTRA_PIN 24
#define LAST_EXTRA_PIN 35

#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

#define A0 10
#define A1 11
#define A2 12
#define A3 13
#define A4 14
#define A5 15
#define A6 16

#define A7 17
#define WKP 17

#define RX 18
#define TX 19

#define BTN 20

// WKP pin on Photon

// Timer pins
#define TIMER2_CH1 10
#define TIMER2_CH2 11
#define TIMER2_CH3 18
#define TIMER2_CH4 19

#define TIMER3_CH1 14
#define TIMER3_CH2 15
#define TIMER3_CH3 16
#define TIMER3_CH4 17

#define TIMER4_CH1 1
#define TIMER4_CH2 0

// SPI pins
#define SS 12
#define SCK 13
#define MISO 14
#define MOSI 15

// I2C pins
#define SDA 0
#define SCL 1

// DAC pins on Photon
#define DAC1 16
#define DAC2 13

// RGB LED pins
#define RGBR 21
#define RGBG 22
#define RGBB 23

// ELECTRON pins
#define B0 24
#define B1 25
#define B2 26
#define B3 27
#define B4 28
#define B5 29
#define C0 30
#define C1 31
#define C2 32
#define C3 33
#define C4 34
#define C5 35

// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.
#define TXD_UC 36
#define RXD_UC 37
#define RI_UC 38
#define CTS_UC 39
#define RTS_UC 40
#define PWR_UC 41
#define RESET_UC 42
#define LVLOE_UC 43
#define PM_SDA_UC 44
#define PM_SCL_UC 45
#define LOW_BAT_UC 46

#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
