/**
 ******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "pwm_hal.h"
#include "gpio_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void HAL_PWM_Write(uint16_t pin, uint8_t value)
{

  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  //PWM Frequency : 500 Hz
  uint16_t TIM_Prescaler = (uint16_t)(SystemCoreClock / 24000000) - 1;//TIM Counter clock = 24MHz
  uint16_t TIM_ARR = (uint16_t)(24000000 / TIM_PWM_FREQ) - 1;

  // TIM Channel Duty Cycle(%) = (TIM_CCR / TIM_ARR + 1) * 100
  uint16_t TIM_CCR = (uint16_t)(value * (TIM_ARR + 1) / 255);

  // AFIO clock enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

  // TIM clock enable
  if(PIN_MAP[pin].timer_peripheral == TIM2)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  }
  else if(PIN_MAP[pin].timer_peripheral == TIM3)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  }
  else if(PIN_MAP[pin].timer_peripheral == TIM4)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  }

  // Time base configuration
  TIM_TimeBaseStructure.TIM_Period = TIM_ARR;
  TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

  // PWM1 Mode configuration
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_Pulse = TIM_CCR;

  if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
  {
    // PWM1 Mode configuration: Channel1
    TIM_OC1Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
  {
    // PWM1 Mode configuration: Channel2
    TIM_OC2Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
  {
    // PWM1 Mode configuration: Channel3
    TIM_OC3Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
  {
    // PWM1 Mode configuration: Channel4
    TIM_OC4Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
  }

  TIM_ARRPreloadConfig(PIN_MAP[pin].timer_peripheral, ENABLE);

  // TIM enable counter
  TIM_Cmd(PIN_MAP[pin].timer_peripheral, ENABLE);
}