/**
 ******************************************************************************
 * @file    servo_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
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

/* Includes ------------------------------------------------------------------*/
#include "servo_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SERVO_TIM_PWM_COUNTER_CLOCK 1000000 //TIM Counter clock = 1MHz

#define SERVO_TIM_PRESCALER     (uint16_t)(SystemCoreClock / SERVO_TIM_PWM_COUNTER_CLOCK) - 1      //To get TIM counter clock = 1MHz
#define SERVO_TIM_ARR           (uint16_t)(SERVO_TIM_PWM_COUNTER_CLOCK / SERVO_TIM_PWM_FREQ) - 1   //To get PWM period = 20ms

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_Servo_Attach(uint16_t pin)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  // AFIO clock enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

  // TIM clock enable
  if(PIN_MAP[pin].timer_peripheral == TIM2)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  else if(PIN_MAP[pin].timer_peripheral == TIM3)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  else if(PIN_MAP[pin].timer_peripheral == TIM4)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  // Time base configuration
  TIM_TimeBaseStructure.TIM_Period = SERVO_TIM_ARR;
  TIM_TimeBaseStructure.TIM_Prescaler = SERVO_TIM_PRESCALER;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

  // PWM1 Mode configuration
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_Pulse = 0x0000;

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

  // Main Output Enable
  TIM_CtrlPWMOutputs(PIN_MAP[pin].timer_peripheral, ENABLE);
}

void HAL_Servo_Detach(uint16_t pin)
{
  const STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

  // Disable timer's channel
  switch (pin_info->timer_ch)
  {
    case TIM_Channel_1:
      pin_info->timer_peripheral->CCER &= ~TIM_CCER_CC1E;
      break;
    case TIM_Channel_2:
      pin_info->timer_peripheral->CCER &= ~TIM_CCER_CC2E;
      break;
    case TIM_Channel_3:
      pin_info->timer_peripheral->CCER &= ~TIM_CCER_CC3E;
      break;
    case TIM_Channel_4:
      pin_info->timer_peripheral->CCER &= ~TIM_CCER_CC4E;
      break;
    default:
      break;
  }

  // Disable timer if none of its channels are enabled
  if (!(pin_info->timer_peripheral->CCER & (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E)))
  {
    TIM_Cmd(pin_info->timer_peripheral, DISABLE);
  }
}

void HAL_Servo_Write_Pulse_Width(uint16_t pin, uint16_t pulseWidth)
{
  //SERVO_TIM_CCR = pulseWidth * (SERVO_TIM_ARR + 1) * SERVO_TIM_PWM_FREQ / 1000000;
  uint16_t SERVO_TIM_CCR = pulseWidth;

  if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
  {
    TIM_SetCompare1(PIN_MAP[pin].timer_peripheral, SERVO_TIM_CCR);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
  {
    TIM_SetCompare2(PIN_MAP[pin].timer_peripheral, SERVO_TIM_CCR);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
  {
    TIM_SetCompare3(PIN_MAP[pin].timer_peripheral, SERVO_TIM_CCR);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
  {
    TIM_SetCompare4(PIN_MAP[pin].timer_peripheral, SERVO_TIM_CCR);
  }
}

uint16_t HAL_Servo_Read_Pulse_Width(uint16_t pin)
{
  uint16_t SERVO_TIM_CCR = 0x0000;

  if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
  {
    SERVO_TIM_CCR = TIM_GetCapture1(PIN_MAP[pin].timer_peripheral);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
  {
    SERVO_TIM_CCR = TIM_GetCapture2(PIN_MAP[pin].timer_peripheral);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
  {
    SERVO_TIM_CCR = TIM_GetCapture3(PIN_MAP[pin].timer_peripheral);
  }
  else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
  {
    SERVO_TIM_CCR = TIM_GetCapture4(PIN_MAP[pin].timer_peripheral);
  }

  //pulseWidth = (SERVO_TIM_CCR * 1000000) / ((SERVO_TIM_ARR + 1) * SERVO_TIM_PWM_FREQ);
  return SERVO_TIM_CCR;
}

uint16_t HAL_Servo_Read_Frequency(uint16_t pin)
{
    uint16_t TIM_ARR = 0;
    uint16_t Servo_Frequency = 0;

    if(PIN_MAP[pin].timer_peripheral == TIM2)
    {
        TIM_ARR = PIN_MAP[pin].timer_peripheral->ARR;
    }
    else if(PIN_MAP[pin].timer_peripheral == TIM3)
    {
        TIM_ARR = PIN_MAP[pin].timer_peripheral->ARR;
    }
    else if(PIN_MAP[pin].timer_peripheral == TIM4)
    {
        TIM_ARR = PIN_MAP[pin].timer_peripheral->ARR;
    }
    else
    {
        return Servo_Frequency;
    }

    Servo_Frequency = (uint16_t)(SERVO_TIM_PWM_COUNTER_CLOCK / (TIM_ARR + 1));

    return Servo_Frequency;
}
