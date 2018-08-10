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

#ifndef PINMAP_IMPL_H
#define	PINMAP_IMPL_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct Pin_Info {
    uint8_t     gpio_port;      // port0: 0; port: 1;
    uint8_t     gpio_pin;       // range: 0~31;
    PinMode     pin_mode;       // GPIO pin mode
    PinFunction pin_func;       
    uint8_t     adc_channel;
    uint8_t     pwm_instance;   // 4 instances on nRF52, range: 0~3
    uint8_t     pwm_channel;    // 4 channels in each instance, range: 0~3
    uint8_t     exti_channel;   // 16 channels
} NRF5x_Pin_Info;

#define NRF_PORT_NONE       ((uint8_t)(0xFF))
#define NRF_PORT_0          ((uint8_t)(0))
#define NRF_PORT_1          ((uint8_t)(1))

#define CHANNEL_NONE        ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE    CHANNEL_NONE
#define DAC_CHANNEL_NONE    CHANNEL_NONE
#define PWM_INSTANCE_NONE   ((uint8_t)(0xFF))
#define PWM_CHANNEL_NONE    CHANNEL_NONE
#define EXTI_CHANNEL_NONE   CHANNEL_NONE

NRF5x_Pin_Info* HAL_Pin_Map(void);
extern const uint8_t NRF_PIN_LOOKUP_TABLE[48];

#define TOTAL_ANALOG_PINS   6
#define FIRST_ANALOG_PIN    D14

// Common Pins
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

// button pin       
#define BTN                 20

// RGB LED pins     
#define RGBR                21
#define RGBG                22
#define RGBB                23

// analog pins      
#define A0                  D19
#define A1                  D18
#define A2                  D17
#define A3                  D16
#define A4                  D15
#define A5                  D14

// SPI pins     
#define SCK                 D13
#define MISO                D11
#define MOSI                D12

// I2C pins     
#define SDA                 D0
#define SCL                 D1

// uart pins        
#define TX                  D9
#define RX                  D10
#define CTS                 D3
#define RTS                 D2

#if PLATFORM_ID == PLATFORM_XENON 
#define TOTAL_PINS          31
#define TX1                 D4
#define RX1                 D5
#define CTS1                D6
#define RTS1                D8

#define BATT                24
#define PWR                 25
#define CHG                 26
#define NFC_PIN1            27
#define NFC_PIN2            28
#define ANTSW1              29
#define ANTSW2              30

// WKP pin on Xenon
#define WKP                 D8   // FIXME: 
#define A7                  A5   // FIXME: A7 is used in spark_wiring_wifitester.cpp

#elif PLATFORM_ID == PLATFORM_ARGON
#define TOTAL_PINS          36
#define TX1                 24
#define RX1                 25
#define CTS1                26
#define RTS1                27
#define ESPBOOT             28
#define ESPEN               29
#define HWAKE               30
#define ANTSW1              31
#define ANTSW2              32
#define BATT                33
#define PWR                 34
#define CHG                 35
#elif PLATFORM_ID == PLATFORM_BORON
#define TOTAL_PINS          34
#define TX1                 24
#define RX1                 25
#define CTS1                26
#define RTS1                27
#define UBPWR               28
#define UBRST               29
#define BUFEN               30
#define ANTSW1              31
#define PMIC_SCL            32
#define PMIC_SDA            33
#endif

#ifdef	__cplusplus
}
#endif

#endif
