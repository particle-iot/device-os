/**
 ******************************************************************************
 * @file    spark_wiring.h
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring.c module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_H
#define SPARK_WIRING_H
#include "stm32f10x.h"
#include "config.h"
#include "spark_macros.h"
#include "debug.h"
#include "platform_config.h"
#include "spark_utilities.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_printable.h"
#include "spark_wiring_ipaddress.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_character.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_random.h"

/*
* Basic variables
*/

#if !defined(min)
#   define min(a,b)                ((a)<(b)?(a):(b))
#endif
#if !defined(max)
#   define max(a,b)                ((a)>(b)?(a):(b))
#endif
#if !defined(constrain)
#   define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif
#if !defined(round)
#   define round(x)                ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif

#define HIGH 0x1
#define LOW 0x0

#define boolean bool

//#define NULL ((void *)0)
#define NONE ((uint8_t)0xFF)

/*
* Pin mapping. Borrowed from Wiring
*/

#define TOTAL_PINS 21
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

#define LED1 LED_USER

#define A0 10
#define A1 11
#define A2 12
#define A3 13
#define A4 14
#define A5 15
#define A6 16
#define A7 17

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

#define ADC1_DR_ADDRESS		((uint32_t)0x4001244C)
#define ADC_DMA_BUFFERSIZE	10
#define ADC_SAMPLING_TIME	ADC_SampleTime_7Cycles5	//Allowed values: 1.5, 7.5 and 13.5 for "Dual slow interleaved mode"

#define TIM_PWM_FREQ 500 //500Hz

#define LSBFIRST 0
#define MSBFIRST 1

typedef unsigned char byte;

typedef enum PinMode {
  OUTPUT,
  INPUT,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  AF_OUTPUT_PUSHPULL,	//Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
  AF_OUTPUT_DRAIN,		//Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
  AN_INPUT  			//Used internally for ADC Input
} PinMode;

typedef struct STM32_Pin_Info {
  GPIO_TypeDef* gpio_peripheral;
  uint16_t gpio_pin;
  uint8_t adc_channel;
  TIM_TypeDef* timer_peripheral;
  uint16_t timer_ch;
  PinMode pin_mode;
  uint16_t timer_ccr;
  int32_t user_property;
} STM32_Pin_Info;

extern STM32_Pin_Info PIN_MAP[];
#ifdef __cplusplus
extern "C" {
#endif

/*
* ADC
*/
void setADCSampleTime(uint8_t ADC_SampleTime);

/*
* GPIO
*/
void pinMode(uint16_t pin, PinMode mode);
void digitalWrite(uint16_t pin, uint8_t value);
int32_t digitalRead(uint16_t pin);
int32_t analogRead(uint16_t pin);
void analogWrite(uint16_t pin, uint8_t value);

/*
* Timing
*/
system_tick_t millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

long map(long value, long fromStart, long fromEnd, long toStart, long toEnd);

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

#ifdef __cplusplus
}
#endif

#endif /* SPARK_WIRING_H_ */
