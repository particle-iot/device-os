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
#include "platforms.h"
#include "hal_platform.h"

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
  AN_OUTPUT,          //Used internally for DAC Output,
  OUTPUT_OPEN_DRAIN = AF_OUTPUT_DRAIN,
  PIN_MODE_NONE=0xFF
} PinMode;

typedef enum {
    PF_NONE,
    PF_DIO,
    PF_TIMER,
    PF_ADC,
    PF_DAC,
    PF_UART,
    PF_PWM,
    PF_SPI,
    PF_I2C
} PinFunction;

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction);
void HAL_Set_Pin_Function(pin_t pin, PinFunction pin_func);


#if HAL_PLATFORM_NRF52840
typedef struct NRF5x_Pin_Info  NRF5x_Pin_Info;
NRF5x_Pin_Info* HAL_Pin_Map(void);
extern const uint8_t NRF_PIN_LOOKUP_TABLE[48];

// FIXME: hack for hal_dynalib_gpio.h
typedef struct NRF5x_Pin_Info STM32_Pin_Info;

#else
typedef struct STM32_Pin_Info  STM32_Pin_Info;
STM32_Pin_Info* HAL_Pin_Map(void);
#endif

/* Exported macros -----------------------------------------------------------*/

#define PIN_INVALID 0xff

/*
* Pin mapping. Borrowed from Wiring
*/
#if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || \
    PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM

#if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
#define TOTAL_ANALOG_PINS   6
#else
#define TOTAL_ANALOG_PINS   8 // SoM
#endif

#define FIRST_ANALOG_PIN    D14

// digital pins
#define D0              0
#define D1              1
#define D2              2
#define D3              3
#define D4              4
#define D5              5
#define D6              6
#define D7              7
#define D8              8
#define D9              9
#define D10             10
#define D11             11
#define D12             12
#define D13             13
#define D14             14
#define D15             15
#define D16             16
#define D17             17
#define D18             18
#define D19             19

#if PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM

#define D20             20
#define D21             21
#define D22             28
#define D23             29

#if PLATFORM_ID == PLATFORM_ASOM
#define D24             37
#endif

#if PLATFORM_ID == PLATFORM_XSOM
#define D24             30
#define D25             31
#define D26             32
#define D27             33
#define D28             34
#define D29             35
#define D30             36
#define D31             37
#endif

#endif

// analog pins
#define A0              D19
#define A1              D18
#define A2              D17
#define A3              D16
#define A4              D15
#define A5              D14

#if PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM
#define A6              D21
#define A7              D20
#endif

// SPI pins
#if PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM
#define SS              D8
#else
#define SS              D14
#endif
#define SCK             D13
#define MISO            D11
#define MOSI            D12

// I2C pins
#define SDA             D0
#define SCL             D1

// uart pins
#define TX              D9
#define RX              D10
#define CTS             D3
#define RTS             D2

#if PLATFORM_ID == PLATFORM_XENON
#define TX1             D4
#define RX1             D5
#define CTS1            D6
#define RTS1            D8
#elif PLATFORM_ID == PLATFORM_XSOM
#define TX1             D25
#define RX1             D26
#define CTS1            D22
#define RTS1            D23
#elif PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
#define TX1             24
#define RX1             25
#define CTS1            26
#define RTS1            27
#else // A SoM and B SoM
#define TX1             30
#define RX1             31
#define CTS1            32
#define RTS1            33
#endif

#if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON

// button pin
#define BTN             20

// RGB LED pins
#define RGBR            21
#define RGBG            22
#define RGBB            23

#else // Mesh SoMs

// button pin
#define BTN             22

// RGB LED pins
#define RGBR            23
#define RGBG            24
#define RGBB            25

#define NFC_PIN1        26
#define NFC_PIN2        27

#endif

// WKP pin
#define WKP             D8

#if PLATFORM_ID == PLATFORM_XENON // Xenon
#define TOTAL_PINS      (31)
#define BATT            24
#define PWR             25
#define CHG             26
#define NFC_PIN1        27
#define NFC_PIN2        28
#define ANTSW1          29
#define ANTSW2          30
#endif

#if PLATFORM_ID == PLATFORM_ARGON // Argon
#define TOTAL_PINS      (36)
#define ESPBOOT         28
#define ESPEN           29
#define HWAKE           30
#define ANTSW1          31
#define ANTSW2          32
#define BATT            33
#define PWR             34
#define CHG             35
#endif

#if PLATFORM_ID == PLATFORM_BORON // Boron
#define TOTAL_PINS      (36)
#define UBPWR           28
#define UBRST           29
#define BUFEN           30
#define ANTSW1          31
#define PMIC_SCL        32
#define PMIC_SDA        33
#define UBVINT          34
#define LOW_BAT_UC      35
#endif

#if PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM
#define LOW_BAT_UC      A6
#endif // PLATFORM_ID == PLATFORM_XSOM || PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM

#if PLATFORM_ID == PLATFORM_XSOM // X SoM
#define TOTAL_PINS      (38)
#endif

#if PLATFORM_ID == PLATFORM_ASOM // A SoM
#define TOTAL_PINS      (38)
#define ESPBOOT         34
#define ESPEN           35
#define HWAKE           36
#endif

#if PLATFORM_ID == PLATFORM_BSOM // B SoM
#define TOTAL_PINS      (38)
#define UBPWR           34
#define UBRST           35
#define BUFEN           36
#define UBVINT          37
#endif

// TODO: Move this to a platform-specific header
#define DEFAULT_PWM_FREQ 500 // 500Hz
#define TIM_PWM_FREQ DEFAULT_PWM_FREQ

#elif PLATFORM_ID != 3
#if PLATFORM_ID == 10 // Electron
#define TOTAL_PINS 47
#elif PLATFORM_ID == 8 // P1
#define TOTAL_PINS 31
#else // Must be Photon
#define TOTAL_PINS 24
#endif

#if PLATFORM_ID == 10 || PLATFORM_ID == 8 || PLATFORM_ID == 6
#define TOTAL_DAC_PINS 2
#else
#define TOTAL_DAC_PINS 0
#endif

#if PLATFORM_ID == 10 // Electron
#define TOTAL_ANALOG_PINS 12
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
#define P1S6    30
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

#else
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

const pin_t SS   = 12;
const pin_t SCK  = 13;
const pin_t MISO = 14;
const pin_t MOSI = 15;

// I2C pins

const pin_t SDA  = 0;
const pin_t SCL  = 1;

// DAC pins on Photon
const pin_t DAC1 = 16;
const pin_t DAC2 = 13;

const uint8_t LSBFIRST = 0;
const uint8_t MSBFIRST = 1;

#endif // PLATFORM_ID==3

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif


#endif  /* __PINMAP_HAL_H */
