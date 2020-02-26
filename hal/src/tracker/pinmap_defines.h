/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#if PLATFORM_ID == PLATFORM_TRACKER

#define TOTAL_PINS              59
#define TOTAL_ANALOG_PINS       8
#define FIRST_ANALOG_PIN        0

// Digital pins
#define D0                      0
#define D1                      1
#define D2                      2
#define D3                      3
#define D4                      4
#define D5                      5
#define D6                      6
#define D7                      7
#define D8                      8
#define D9                      9

// Analog pins
#define A0                      D0
#define A1                      D1
#define A2                      D2
#define A3                      D3
#define A4                      D4
#define A5                      D5
#define A6                      D6
#define A7                      D7

// I2C
#define SDA                     D0
#define SCL                     D1

// UART
#define TX                      D2
#define RX                      D3
#define CTS                     D4
#define RTS                     D5

// SPI
#define SS                      D6
#define SCK                     D7
#define MOSI                    D8
#define MISO                    D9

// Wakeup pin
#define WKP                     D8

// RGB
#define RGBR                    10
#define RGBG                    11
#define RGBB                    12

// Mode Button
#define BTN                     13

// I2C1
#define SDA1                    14
#define SCL1                    15

// SPI1, SS pin not specified
#define SCK1                    16
#define MOSI1                   17
#define MISO1                   18

// UART1
#define TX1                     19
#define RX1                     20
#define RTS1                    21
#define CTS1                    22

// PMIC
#define PMIC_INT                23
#define PMIC_SDA                SDA1
#define PMIC_SCL                SCL1

// Fuel Gauge
#define LOW_BAT_UC              43

// RTC
#define RTC_INT                 24
#define RTC_WDI                 42
#define RTC_SDA                 SDA1
#define RTC_SCL                 SCL1

// Cellular
#define BGRST                   25
#define BGPWR                   26
#define BGVINT                  27
#define BGDTR                   41

// CAN Transciever
#define CAN_INT                 28
#define CAN_RST                 48
#define CAN_PWR                 49
#define CAN_STBY                50
#define CAN_RTS2                51
#define CAN_RTS1                52
#define CAN_RTS0                53
#define CAN_CS                  58

// IO Expander
#define IOE_INT                 29
#define IOE_RST                 30
#define IOE_CS                  55

// Sensor
#define SEN_INT                 31
#define SEN_CS                  56

// NFC
#define NFC_PIN1                32
#define NFC_PIN2                33

// Demux IC
#define DEMUX_A                 36
#define DEMUX_B                 35
#define DEMUX_C                 34

// BLE Antenna Switch
#define BLE_ANT_SW              37

// Wi-Fi
#define WIFI_EN                 38
#define WIFI_INT                39
#define WIFI_BOOT               40
#define WIFI_CS                 57

// GPS
#define GPS_PWR                 44
#define GPS_INT                 45
#define GPS_BOOT                46
#define GPS_RST                 47
#define GPS_CS                  54

#endif // PLATFORM_ID == PLATFORM_TRACKER
