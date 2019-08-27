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

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hal_Pin_Info {
    GPIO_TypeDef    *gpio_peripheral;
    pin_t           gpio_pin;
    uint8_t         gpio_pin_source;
    uint8_t         adc_channel;
    TIM_TypeDef     *timer_peripheral;
    uint16_t        timer_ch;
    PinMode         pin_mode;
    uint16_t        timer_ccr;
    int32_t         user_property;
} Hal_Pin_Info;

// For compatibility
typedef Hal_Pin_Info STM32_Pin_Info;

Hal_Pin_Info *HAL_Pin_Map(void);

extern void HAL_GPIO_Save_Pin_Mode(uint16_t pin);
extern PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin);

#define CHANNEL_NONE ((uint8_t)0xFF)
#define ADC_CHANNEL_NONE CHANNEL_NONE

#define TIM_PWM_FREQ        500  //500Hz

#define SERVO_TIM_PWM_FREQ  50   //50Hz                                                                                      //20ms = 50Hz

#define TOTAL_PINS 24
#define TOTAL_DAC_PINS 0
#define TOTAL_ANALOG_PINS 8
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

#ifdef __cplusplus
}
#endif
