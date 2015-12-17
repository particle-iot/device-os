/**
 ******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair
 * @version V1.0.0
 * @date    23-Dec-2014
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
#include "pwm_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define TIM_PWM_COUNTER_CLOCK_FREQ 30000000 //TIM Counter clock = 30MHz

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

uint32_t HAL_PWM_Enable_TIM_Clock(uint16_t pin);
uint16_t HAL_PWM_Calculate_ARR(uint16_t pwm_fequency);
uint16_t HAL_PWM_Calculate_CCR(uint32_t TIM_CLK, uint8_t value);
void HAL_PWM_Configure_TIM(uint32_t TIM_CLK, uint8_t value, uint16_t pin);
void HAL_PWM_Enable_TIM(uint16_t pin);
void HAL_PWM_Update_Duty_Cycle(uint16_t pin, uint16_t value);

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */

void HAL_PWM_Write(uint16_t pin, uint8_t value)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // If PWM has not been initialized, or user has called pinMode(, OUTPUT)
    if (!(pin_info->user_property & PWM_INIT) || pin_info->pin_mode == OUTPUT)
    {
    	// Mark the initialization
    	pin_info->user_property |= PWM_INIT;

    	// Configure TIM GPIO pins
        HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

        // Enable TIM clock
        uint32_t TIM_CLK = HAL_PWM_Enable_TIM_Clock(pin);

        // Configure Timer
    	HAL_PWM_Configure_TIM(TIM_CLK, value, pin);

    	// TIM enable counter
    	HAL_PWM_Enable_TIM(pin);

    	// Set Duty Cycle
    	HAL_PWM_Update_Duty_Cycle(pin, value);
    }
    else
    {
    	HAL_PWM_Update_Duty_Cycle(pin, value);
    }
}


uint16_t HAL_PWM_Get_Frequency(uint16_t pin)
{
    uint16_t TIM_ARR = 0;
    uint16_t PWM_Frequency = 0;

    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if(pin_info->timer_peripheral == TIM1)
    {
        TIM_ARR = pin_info->timer_peripheral->ARR;
    }
    else if(pin_info->timer_peripheral == TIM3)
    {
        TIM_ARR = pin_info->timer_peripheral->ARR;
    }
    else if(pin_info->timer_peripheral == TIM4)
    {
        TIM_ARR = pin_info->timer_peripheral->ARR;
    }
    else if(pin_info->timer_peripheral == TIM5)
    {
        TIM_ARR = pin_info->timer_peripheral->ARR;
    }
#if PLATFORM_ID == 10 // Electron
    else if(pin_info->timer_peripheral == TIM8)
    {
        TIM_ARR = pin_info->timer_peripheral->ARR;
    }
#endif
    else
    {
        return PWM_Frequency;
    }

    PWM_Frequency = (uint16_t)(TIM_PWM_COUNTER_CLOCK_FREQ / (TIM_ARR + 1));

    return PWM_Frequency;
}


uint16_t HAL_PWM_Get_AnalogValue(uint16_t pin)
{
    uint16_t TIM_CCR = 0;
    uint16_t TIM_ARR = 0;
    uint16_t PWM_AnalogValue = 0;

    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if(pin_info->timer_ch == TIM_Channel_1)
    {
        TIM_CCR = pin_info->timer_peripheral->CCR1;
    }
    else if(pin_info->timer_ch == TIM_Channel_2)
    {
        TIM_CCR = pin_info->timer_peripheral->CCR2;
    }
    else if(pin_info->timer_ch == TIM_Channel_3)
    {
        TIM_CCR = pin_info->timer_peripheral->CCR3;
    }
    else if(pin_info->timer_ch == TIM_Channel_4)
    {
        TIM_CCR = pin_info->timer_peripheral->CCR4;
    }
    else
    {
        return PWM_AnalogValue;
    }

    TIM_ARR = pin_info->timer_peripheral->ARR;
    PWM_AnalogValue = (uint16_t)(((TIM_CCR + 1) * 255) / (TIM_ARR + 1));

    return PWM_AnalogValue;
}


void HAL_PWM_Update_Duty_Cycle(uint16_t pin, uint16_t value)
{
	STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

	// Get Clock here
	uint32_t TIM_CLK = SystemCoreClock;
	if (pin_info->timer_peripheral == TIM1) {
		TIM_CLK = SystemCoreClock;
	} else if (pin_info->timer_peripheral == TIM3) {
		TIM_CLK = SystemCoreClock / 2;
	} else if (pin_info->timer_peripheral == TIM4) {
		TIM_CLK = SystemCoreClock / 2;
	} else if (pin_info->timer_peripheral == TIM5) {
		TIM_CLK = SystemCoreClock / 2;
	}

	// Calculate new output compare register value
	uint16_t TIM_CCR = HAL_PWM_Calculate_CCR(TIM_CLK, value);

	// Set output compare register value
	if (pin_info->timer_ch == TIM_Channel_1) {
		TIM_SetCompare1(pin_info->timer_peripheral, TIM_CCR);
	} else if (pin_info->timer_ch == TIM_Channel_2) {
		TIM_SetCompare2(pin_info->timer_peripheral, TIM_CCR);
	} else if (pin_info->timer_ch == TIM_Channel_3) {
		TIM_SetCompare3(pin_info->timer_peripheral, TIM_CCR);
	} else if (pin_info->timer_ch == TIM_Channel_4) {
		TIM_SetCompare4(pin_info->timer_peripheral, TIM_CCR);
	}
}


uint32_t HAL_PWM_Enable_TIM_Clock(uint16_t pin)
{
	STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;
	uint32_t TIM_CLK = SystemCoreClock;

	// TIM clock enable
	if (pin_info->timer_peripheral == TIM1)
	{
		TIM_CLK = SystemCoreClock;
		GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM1);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	}
	else if (pin_info->timer_peripheral == TIM3)
	{
		TIM_CLK = SystemCoreClock / 2;
		GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM3);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if (pin_info->timer_peripheral == TIM4)
	{
		TIM_CLK = SystemCoreClock / 2;
		GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM4);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	}
	else if (pin_info->timer_peripheral == TIM5)
	{
		TIM_CLK = SystemCoreClock / 2;
		GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM5);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	}
#if PLATFORM_ID == 10 // Electron
	else if(pin_info->timer_peripheral == TIM8)
	{
		TIM_CLK = SystemCoreClock;
		GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM8);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	}
#endif


	//PWM Frequency : 500 Hz
	uint16_t TIM_Prescaler = (uint16_t) (TIM_CLK / TIM_PWM_COUNTER_CLOCK_FREQ) - 1;
	uint16_t TIM_ARR = HAL_PWM_Calculate_ARR(TIM_PWM_FREQ);

	// Time base configuration
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = { 0 };
	TIM_TimeBaseStructure.TIM_Period = TIM_ARR;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(pin_info->timer_peripheral, &TIM_TimeBaseStructure);

	return TIM_CLK;
}


uint16_t HAL_PWM_Calculate_CCR(uint32_t TIM_CLK, uint8_t value)
{
	//PWM Frequency : 500 Hz
	uint16_t TIM_ARR = HAL_PWM_Calculate_ARR(TIM_PWM_FREQ);

	// TIM Channel Duty Cycle(%) = (TIM_CCR / TIM_ARR + 1) * 100
	uint16_t TIM_CCR = (uint16_t) (value * (TIM_ARR + 1) / 255);

	return TIM_CCR;
}


uint16_t  HAL_PWM_Calculate_ARR(uint16_t pwm_fequency)
{
	//PWM Frequency : 500 Hz
	 return (uint16_t) (TIM_PWM_COUNTER_CLOCK_FREQ / pwm_fequency) - 1;
}


void HAL_PWM_Configure_TIM(uint32_t TIM_CLK, uint8_t value, uint16_t pin)
{
	STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

	//PWM Duty Cycle
	uint16_t TIM_CCR = HAL_PWM_Calculate_CCR(TIM_CLK, pin);

	// PWM1 Mode configuration
	// Initialize all 8 struct params to 0, fixes randomly inverted RX, TX PWM
	TIM_OCInitTypeDef TIM_OCInitStructure = { 0 };
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse = TIM_CCR;

	// Enable output-compare preload function.  Duty cycle will be updated
	// at end of each counter cycle to prevent glitches.
	if (pin_info->timer_ch == TIM_Channel_1)
	{
		// PWM1 Mode configuration: Channel1
		TIM_OC1Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
	}
	else if (pin_info->timer_ch == TIM_Channel_2)
	{
		// PWM1 Mode configuration: Channel2
		TIM_OC2Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(pin_info->timer_peripheral,TIM_OCPreload_Enable);
	}
	else if (pin_info->timer_ch == TIM_Channel_3)
	{
		// PWM1 Mode configuration: Channel3
		TIM_OC3Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
	}
	else if (pin_info->timer_ch == TIM_Channel_4)
	{
		// PWM1 Mode configuration: Channel4
		TIM_OC4Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
	}

	// Enable Auto-load register preload function.  ARR register or PWM period
	// will be update at end of each counter cycle to prevent glitches.
	TIM_ARRPreloadConfig(pin_info->timer_peripheral, ENABLE);
}


void HAL_PWM_Enable_TIM(uint16_t pin)
{
	STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

	// TIM enable counter
	TIM_Cmd(pin_info->timer_peripheral, ENABLE);

	if ((pin_info->timer_peripheral == TIM1) ||
		(pin_info->timer_peripheral == TIM8))
	{
		/* TIM Main Output Enable - required for TIM1/TIM8 PWM output */
		TIM_CtrlPWMOutputs(pin_info->timer_peripheral, ENABLE);
	}
}
