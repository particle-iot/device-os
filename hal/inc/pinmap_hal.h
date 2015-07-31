/**
 ******************************************************************************
 * @file    pinmap_hal.h
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    13-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PINMAP_HAL_H
#define __PINMAP_HAL_H

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/

typedef uint16_t pin_t;

typedef enum PinMode {
  INPUT,
  OUTPUT,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  AF_OUTPUT_PUSHPULL, //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
  AF_OUTPUT_DRAIN,    //Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
  AN_INPUT,           //Used internally for ADC Input
  AN_OUTPUT,          //Used internally for DAC Output
  PIN_MODE_NONE=0xFF
} PinMode;

typedef enum {
    PF_NONE,
    PF_DIO,
    PF_TIMER,
    PF_ADC,
  PF_DAC
} PinFunction;

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction);

typedef struct STM32_Pin_Info  STM32_Pin_Info;

STM32_Pin_Info* HAL_Pin_Map(void);

/* Exported macros -----------------------------------------------------------*/

/*
* Pin mapping. Borrowed from Wiring
*/
#if PLATFORM_ID == 10 // Electron
#define TOTAL_PINS 47
#elif PLATFORM_ID == 8 // P1
#define TOTAL_PINS 30
#else // Must be Photon
#define TOTAL_PINS 24
#endif

#if PLATFORM_ID == 10 // Electron
#define TOTAL_ANALOG_PINS 8 /* NOT USED IN CODE, BUT UPDATE THIS LATER ANYWAY! */
#elif PLATFORM_ID == 8 // P1
#define TOTAL_ANALOG_PINS 13
#else // Must be Photon
#define TOTAL_ANALOG_PINS 8
#endif
#define FIRST_ANALOG_PIN 10

#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// todo - this is corev1 specific, needs to go in a conditional define

#define LED1 LED_USER

#define A0 10
#define A1 11
#define A2 12
#define A3 13
#define A4 14
#define A5 15
#define A6 16

// WKP pin is also an ADC on Photon
#define A7 17

#define RX 18
#define TX 19

#define BTN 20

// WKP pin on Photon
#define WKP 17

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
#define SS   12
#define SCK  13
#define MISO 14
#define MOSI 15

// I2C pins
#define SDA  0
#define SCL  1

// DAC pins on Photon
//#define DAC  16 // Would like to define this, but already defined in low level stm32f2xx.h
#define DAC1 16
#define DAC2 13

// RGB LED pins
#define RGBR 21
#define RGBG 22
#define RGBB 23

#if PLATFORM_ID == 8 // P1
// P1 SPARE pins
#define P1S0    24
#define P1S1    25
#define P1S2    26
#define P1S3    27
#define P1S4    28
#define P1S5    29
#endif

#if PLATFORM_ID == 10 // Electron
// ELECTRON pins
#define B0        24
#define B1        25
#define B2        26
#define B3        27
#define B4        28
#define B5        29
#define C0        30
#define C1        31
#define C2        32
#define C3        33
#define C4        34
#define C5        35
// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.
#define TXD_UC      36
#define RXD_UC      37
#define RI_UC       38
#define CTS_UC      39
#define RTS_UC      40
#define PWR_UC      41
#define RESET_UC    42
#define LVLOE_UC    43
#define PM_SDA_UC   44
#define PM_SCL_UC   45
#define LOW_BAT_UC  46
#endif

#define TIM_PWM_FREQ 500 //500Hz

#define SERVO_TIM_PWM_FREQ 50//50Hz                                                                                      //20ms = 50Hz

#define LSBFIRST 0
#define MSBFIRST 1

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif


#endif  /* __PINMAP_HAL_H */
