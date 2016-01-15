/**
 ******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair, Julien Vanier
 * @version V2.0.0
 * @date    4-Dec-2015
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

#if PLATFORM_ID == 0 // Core
#define TIM_PWM_COUNTER_CLOCK_FREQ 24000000 //TIM Counter clock = 24MHz
#else
#define TIM_PWM_COUNTER_CLOCK_FREQ 30000000 //TIM Counter clock = 30MHz
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_PWM_Enable_TIM_Clock(uint16_t pin, uint16_t pwm_frequency);
uint32_t HAL_PWM_Calculate_Prescaled_Clock(uint16_t pwm_frequency);
uint16_t HAL_PWM_Calculate_Period(uint32_t prescaled_clock, uint16_t pwm_frequency);
uint16_t HAL_PWM_Get_Period(uint16_t pin);
uint16_t HAL_PWM_Calculate_Pulse(uint16_t period, uint8_t value);
uint32_t HAL_PWM_Base_Clock(uint16_t pin);
uint16_t HAL_PWM_Calculate_Prescaler(uint32_t clock, uint32_t prescaled_clock);
TIM_TimeBaseInitTypeDef HAL_PWM_Calculate_Time_Base(uint16_t pin, uint16_t pwm_frequency);
void HAL_PWM_Update_Registers(uint16_t pin, FunctionalState new_state);
void HAL_PWM_Configure_TIM(uint16_t pin, uint8_t value);
void HAL_PWM_Enable_TIM(uint16_t pin);
void HAL_PWM_Update_Duty_Cycle(uint16_t pin, uint16_t value);
void HAL_PWM_Update_DC_Frequency(uint16_t pin, uint16_t value, uint16_t pwm_frequency);

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */

void HAL_PWM_Write(uint16_t pin, uint8_t value)
{
    HAL_PWM_Write_With_Frequency(pin, value, TIM_PWM_FREQ);
}


/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 */

void HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency)
{
    if(pwm_frequency == 0)
    {
        return;
    }

    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // If PWM has not been initialized, or user has called pinMode(, OUTPUT)
    if (!(pin_info->user_property & PWM_INIT) || pin_info->pin_mode == OUTPUT)
    {
        // Mark the initialization
        pin_info->user_property |= PWM_INIT;

        // Configure TIM GPIO pins
        HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

        // Enable Timer group clock
        HAL_PWM_Enable_TIM_Clock(pin, pwm_frequency);

        // Configure Timer channel
        HAL_PWM_Configure_TIM(pin, value);

        // TIM enable counter
        HAL_PWM_Enable_TIM(pin);
    }
    else
    {
        HAL_PWM_Update_DC_Frequency(pin, value, pwm_frequency);
    }
}


uint16_t HAL_PWM_Get_Frequency(uint16_t pin)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if(!pin_info->timer_peripheral)
    {
        return 0;
    }

    uint32_t clock = HAL_PWM_Base_Clock(pin);
    uint16_t prescaler = TIM_GetPrescaler(pin_info->timer_peripheral);
    uint32_t prescaled_clock = clock / (prescaler + 1);

    uint16_t period = HAL_PWM_Get_Period(pin);
    uint16_t pwm_frequency = (uint16_t)(prescaled_clock / (period + 1));

    return pwm_frequency;
}


uint16_t HAL_PWM_Get_AnalogValue(uint16_t pin)
{
    uint16_t pulse_width = 0;
    uint16_t period = 0;
    uint16_t pwm_analog_value = 0;

    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if(pin_info->timer_ch == TIM_Channel_1)
    {
        pulse_width = pin_info->timer_peripheral->CCR1;
    }
    else if(pin_info->timer_ch == TIM_Channel_2)
    {
        pulse_width = pin_info->timer_peripheral->CCR2;
    }
    else if(pin_info->timer_ch == TIM_Channel_3)
    {
        pulse_width = pin_info->timer_peripheral->CCR3;
    }
    else if(pin_info->timer_ch == TIM_Channel_4)
    {
        pulse_width = pin_info->timer_peripheral->CCR4;
    }
    else
    {
        return 0;
    }

    period = HAL_PWM_Get_Period(pin);
    pwm_analog_value = (uint16_t)(((pulse_width + 1) * 255) / (period + 1));

    return pwm_analog_value;
}

uint32_t HAL_PWM_Base_Clock(uint16_t pin)
{
#if PLATFORM_ID == 0 // Core
    return SystemCoreClock;
#else
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if(pin_info->timer_peripheral == TIM3 ||
            pin_info->timer_peripheral == TIM4 ||
            pin_info->timer_peripheral == TIM5)
    {
        return SystemCoreClock / 2;
    }
#if PLATFORM_ID == 88 // Duo
    else if(pin_info->timer_peripheral == TIM2 ||
            pin_info->timer_peripheral == TIM13 ||
            pin_info->timer_peripheral == TIM14)
    {
        return SystemCoreClock / 2;
    }
#endif
    else
    {
        return SystemCoreClock;
    }
#endif
}

uint32_t HAL_PWM_Calculate_Prescaled_Clock(uint16_t pwm_frequency)
{
    if(pwm_frequency == 0)
    {
        return 0;
    }
    else if(pwm_frequency >= TIM_PWM_FREQ)
    {
        return TIM_PWM_COUNTER_CLOCK_FREQ;
    }
    else
    {
        // Decrease the clock frequency for lower PWM frequencies to
        // avoid an overflow in the Period calculation
        return TIM_PWM_COUNTER_CLOCK_FREQ / (TIM_PWM_FREQ / pwm_frequency + 1);
    }
}

uint16_t HAL_PWM_Calculate_Prescaler(uint32_t clock, uint32_t prescaled_clock)
{
    return (uint16_t) (clock / prescaled_clock) - 1;
}

uint16_t HAL_PWM_Calculate_Period(uint32_t prescaled_clock, uint16_t pwm_frequency)
{
    return (uint16_t) (prescaled_clock / pwm_frequency) - 1;
}

uint16_t HAL_PWM_Calculate_Pulse(uint16_t period, uint8_t value)
{
    // Duty Cycle(%) = (pulse_width / period + 1) * 100
    uint16_t pulse_width = (uint16_t) (value * (period + 1) / 255);

    return pulse_width;
}

uint16_t HAL_PWM_Get_Period(uint16_t pin)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    return pin_info->timer_peripheral->ARR;
}

TIM_TimeBaseInitTypeDef HAL_PWM_Calculate_Time_Base(uint16_t pin, uint16_t pwm_frequency)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = { 0 };

    uint32_t clock = HAL_PWM_Base_Clock(pin);
    uint32_t prescaled_clock = HAL_PWM_Calculate_Prescaled_Clock(pwm_frequency);
    uint16_t prescaler = HAL_PWM_Calculate_Prescaler(clock, prescaled_clock);
    uint16_t period = HAL_PWM_Calculate_Period(prescaled_clock, pwm_frequency);

    // Time base configuration
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    return TIM_TimeBaseStructure;
}

void HAL_PWM_Enable_TIM_Clock(uint16_t pin, uint16_t pwm_frequency)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

	// AFIO and TIM clock enable

#if PLATFORM_ID == 0 // Core

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	if(pin_info->timer_peripheral == TIM2)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if(pin_info->timer_peripheral == TIM3)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if(pin_info->timer_peripheral == TIM4)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	}

#else

    if (pin_info->timer_peripheral == TIM1)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM1);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    }
    else if (pin_info->timer_peripheral == TIM2)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM2);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    }
    else if (pin_info->timer_peripheral == TIM3)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM3);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    }
    else if (pin_info->timer_peripheral == TIM4)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM4);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    }
    else if (pin_info->timer_peripheral == TIM5)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM5);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    }
#if PLATFORM_ID == 10 // Electron
    else if(pin_info->timer_peripheral == TIM8)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM8);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    }
#endif
#if PLATFORM_ID == 88 // Duo
    else if (pin_info->timer_peripheral == TIM13)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM13);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM13, ENABLE);
    }
    else if (pin_info->timer_peripheral == TIM14)
    {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM14);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
    }
#endif

#endif

    // Time base configuration
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = HAL_PWM_Calculate_Time_Base(pin, pwm_frequency);
    TIM_TimeBaseInit(pin_info->timer_peripheral, &TIM_TimeBaseStructure);
}


void HAL_PWM_Configure_TIM(uint16_t pin, uint8_t value)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    //PWM Duty Cycle
    uint16_t period = HAL_PWM_Get_Period(pin);
    uint16_t pulse_width = HAL_PWM_Calculate_Pulse(period, value);

    // PWM1 Mode configuration
    // Initialize all 8 struct params to 0, fixes randomly inverted RX, TX PWM
    TIM_OCInitTypeDef TIM_OCInitStructure = { 0 };
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = pulse_width;

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


void HAL_PWM_Update_DC_Frequency(uint16_t pin, uint16_t value, uint16_t pwm_frequency)
{
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // Calculate new prescaler, period and output compare register value
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = HAL_PWM_Calculate_Time_Base(pin, pwm_frequency);
    uint16_t pulse_width = HAL_PWM_Calculate_Pulse(TIM_TimeBaseStructure.TIM_Period, value);

    // Disable update events while updating registers
    // In case a PWM period ends, it will keep the current values
    HAL_PWM_Update_Registers(pin, DISABLE);

    // Update output compare register value
    if (pin_info->timer_ch == TIM_Channel_1) {
        TIM_SetCompare1(pin_info->timer_peripheral, pulse_width);
    } else if (pin_info->timer_ch == TIM_Channel_2) {
        TIM_SetCompare2(pin_info->timer_peripheral, pulse_width);
    } else if (pin_info->timer_ch == TIM_Channel_3) {
        TIM_SetCompare3(pin_info->timer_peripheral, pulse_width);
    } else if (pin_info->timer_ch == TIM_Channel_4) {
        TIM_SetCompare4(pin_info->timer_peripheral, pulse_width);
    }

    TIM_SetAutoreload(pin_info->timer_peripheral, TIM_TimeBaseStructure.TIM_Period);
    TIM_PrescalerConfig(pin_info->timer_peripheral, TIM_TimeBaseStructure.TIM_Prescaler, TIM_PSCReloadMode_Update);

    // Re-enable update events
    // At the next update event (end of timer period) the preload
    // registers will be copied to the shadow registers
    HAL_PWM_Update_Registers(pin, ENABLE);
}


void HAL_PWM_Update_Registers(uint16_t pin, FunctionalState new_state) {
    STM32_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // UpdateDisableConfig = ENABLE means updates are disabled!
    TIM_UpdateDisableConfig(pin_info->timer_peripheral, !new_state);
}
