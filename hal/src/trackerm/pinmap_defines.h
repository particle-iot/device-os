/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#define TOTAL_PINS          50
#define TOTAL_ANALOG_PINS   6
#define FIRST_ANALOG_PIN    11

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

#define S0                  D15
#define S1                  D16
#define S2                  D17
#define S3                  D18
#define S4                  D19
#define S5                  D20
#define S6                  D21

// Analog pins
#define A0                  D11
#define A1                  D12
#define A2                  D13
#define A3                  D0
#define A4                  D1
#define A5                  D14

// RGB and Button
#define RGBR                22
#define RGBG                23
#define RGBB                24
#define BTN                 25

// SPI, Shared with UART2
#define SS                  A2
#define SCK                 S2
#define MISO                S1
#define MOSI                S0

#define SS1                 D5
#define SCK1                D4
#define MISO1               D3
#define MOSI1               D2

// IO Expander
#define IOE_INT             D10 // Shared with PMIC_INT
#define IOE_RST             43
#define IOE_CS              44

#define CS0                 S3
#define CS1                 S5
#define CS2                 S6

#define DEMUX_A             CS0
#define DEMUX_B             CS1
#define DEMUX_C             CS2

// I2C
#define SDA                 D0
#define SCL                 D1

// PMIC
#define PMIC_SDA            SDA
#define PMIC_SCL            SCL
#define PMIC_INT            D10 // Shared with IOE_INT

// UART (Serial1)
#define TX                  D8
#define RX                  D9
// UART (Serial2) Shared with SPI
#define TX1                 S0
#define RX1                 S1
#define CTS1                D10
#define RTS1                S2
// UART (NCP) Shared with SPI1
#define TX2                 D4
#define RX2                 D5
#define CTS2                D3
#define RTS2                D2

#define WKP                 D10

#define ANTSW               26
#define LOW_BAT_UC          27
#define DCDC_EN             28
#define GNSS_PWR_EN         29
#define IO_2V8_EN           30
#define SHT_ALERT           31
#define GNSS_RST            32
#define PGOOD               33
#define P2_CELL_DTR         34
#define IO_EXP_B0           35
#define IO_EXP_B1           36
#define IO_EXP_B2           37
#define IO_EXP_B3           38
#define P2_CELL_RST         39
#define P2_CELL_PWR         40
#define P2_CELL_STATUS      41
#define GNSS_GEOFENCE       42

#define Y3                  45
#define Y4                  46
#define Y5                  47
#define Y6                  48
#define Y7                  49

// Cellular
#define BGRST               P2_CELL_RST
#define BGPWR               P2_CELL_PWR
#define BGVINT              P2_CELL_STATUS
#define BGDTR               P2_CELL_DTR

// Set it to PIN_INVALID if not present
#define SWD_DAT             D7
#define SWD_CLK             D6

