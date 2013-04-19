/*
 * spark_wiring.h
 *
 *  Created on: Apr 15, 2013
 *      Author: zsupalla
 */

#ifndef SPARK_WIRING_H_
#define SPARK_WIRING_H_

#include "stm32f10x.h"

/*
 * Basic variables
 */

#define HIGH 0x1
#define LOW 0x0

#define true 0x1
#define false 0x0

#define STM32_DELAY_US_MULT 12 // TODO: Fix this.

#define NULL ((void *)0)
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

#define LED1 8
#define LED2 9

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

#define SS 7
#define SCK 6
#define MISO 5
#define MOSI 4

#define ADC_SAMPLING_TIME	ADC_SampleTime_1Cycles5	//ADC_SampleTime_239Cycles5
#define TIM_PWM_FREQ		500 //500Hz
#define NONE				((uint8_t)0xFF)

typedef enum PinMode {
  OUTPUT,
  INPUT,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  AF_OUTPUT,	//Used internally for Alternate Function Output(TIM, UART, SPI etc)
  AN_INPUT		//Used internally for ADC Input
} PinMode;

typedef struct STM32_Pin_Info {
  GPIO_TypeDef* gpio_peripheral;
  uint16_t gpio_pin;
  uint8_t adc_channel;
  TIM_TypeDef* timer_peripheral;
  uint16_t timer_ch;
  PinMode pin_mode;
} STM32_Pin_Info;

/*
 * GPIO
 */


void pinMode(uint16_t pin, PinMode mode);

void digitalWrite(uint16_t pin, uint8_t value);

int32_t digitalRead(uint16_t pin);

void analogWrite(uint16_t pin, uint8_t value);

int32_t analogRead(uint16_t pin);

/*
 * TIMING
 */

uint32_t millis();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

extern void Delay(__IO uint32_t nTime);

#endif /* SPARK_WIRING_H_ */
