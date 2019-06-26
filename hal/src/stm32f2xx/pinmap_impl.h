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

#include "pinmap_hal.h"
#include "stm32f2xx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hal_Pin_Info {
    GPIO_TypeDef    *gpio_peripheral;
    pin_t           gpio_pin;
    uint8_t         gpio_pin_source;
    uint8_t         adc_channel;
    uint8_t         dac_channel;
    TIM_TypeDef     *timer_peripheral;
    uint16_t        timer_ch;
    PinMode         pin_mode;
    uint16_t        timer_ccr;
    int32_t         user_property;
} Hal_Pin_Info;

Hal_Pin_Info *HAL_Pin_Map(void);

extern void HAL_GPIO_Save_Pin_Mode(uint16_t pin);
extern PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin);

#define CHANNEL_NONE ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE CHANNEL_NONE
#define DAC_CHANNEL_NONE CHANNEL_NONE

#define TIM_PWM_FREQ 500 // 500Hz

#define SERVO_TIM_PWM_FREQ 50 // 50Hz

#if PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION

#define TOTAL_PINS 24
#define TOTAL_DAC_PINS 2
#define TOTAL_ANALOG_PINS 8
#define FIRST_ANALOG_PIN 10

// Digital pins
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// Analog pins
#define A0 10
#define A1 11
#define A2 12
#define A3 13
#define A4 14
#define A5 15
#define A6 16

// WKP pin is also an ADC on Photon
#define A7 17
#define WKP 17

#define RX 18
#define TX 19

#define BTN 20

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

#endif // PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION

#if PLATFORM_ID == PLATFORM_P1

#define TOTAL_PINS 31
#define TOTAL_DAC_PINS 2
#define TOTAL_ANALOG_PINS 13
#define FIRST_ANALOG_PIN 10

// Digital pins
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// Analog pins
#define A0 10
#define A1 11
#define A2 12
#define A3 13
#define A4 14
#define A5 15
#define A6 16

// WKP pin is also an ADC on Photon
#define A7 17
#define WKP 17

#define RX 18
#define TX 19

#define BTN 20

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

// P1 SPARE pins
#define P1S0 24
#define P1S1 25
#define P1S2 26
#define P1S3 27
#define P1S4 28
#define P1S5 29
#define P1S6 30

#endif // PLATFORM_ID == PLATFORM_P1

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#define TOTAL_PINS 47
#define TOTAL_DAC_PINS 2
#define TOTAL_ANALOG_PINS 12
#define FIRST_ANALOG_PIN 10

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

#if PLATFORM_ID == PLATFORM_GCC

const pin_t TOTAL_PINS = 21;
const pin_t TOTAL_ANALOG_PINS = 8;
const pin_t FIRST_ANALOG_PIN = 10;
const pin_t D0 = 0;
const pin_t D1 = 1;
const pin_t D2 = 2;
const pin_t D3 = 3;
const pin_t D4 = 4;
const pin_t D5 = 5;
const pin_t D6 = 6;
const pin_t D7 = 7;

const pin_t A0 = 10;
const pin_t A1 = 11;
const pin_t A2 = 12;
const pin_t A3 = 13;
const pin_t A4 = 14;
const pin_t A5 = 15;
const pin_t A6 = 16;

// WKP pin is also an ADC on Photon
const pin_t A7 = 17;

// RX and TX pins are also ADCs on Photon
const pin_t A8 = 18;
const pin_t A9 = 19;

const pin_t RX = 18;
const pin_t TX = 19;

const pin_t BTN = 20;

// WKP pin on Photon
const pin_t WKP = 17;

// Timer pins

const pin_t TIMER2_CH1 = 10;
const pin_t TIMER2_CH2 = 11;
const pin_t TIMER2_CH3 = 18;
const pin_t TIMER2_CH4 = 19;

const pin_t TIMER3_CH1 = 14;
const pin_t TIMER3_CH2 = 15;
const pin_t TIMER3_CH3 = 16;
const pin_t TIMER3_CH4 = 17;

const pin_t TIMER4_CH1 = 1;
const pin_t TIMER4_CH2 = 0;

// SPI pins

const pin_t SS = 12;
const pin_t SCK = 13;
const pin_t MISO = 14;
const pin_t MOSI = 15;

// I2C pins

const pin_t SDA = 0;
const pin_t SCL = 1;

// DAC pins on Photon
const pin_t DAC1 = 16;
const pin_t DAC2 = 13;

const uint8_t LSBFIRST = 0;
const uint8_t MSBFIRST = 1;

#endif // PLATFORM_ID == PLATFORM_GCC

#ifdef __cplusplus
}
#endif
