/******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair, Julien Vanier, Andrey Tolstoy
 * @version V2.1.0
 * @date    02-May-2016
 * @brief
 ******************************************************************************/
/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include "pwm_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

#define DIV_ROUND_CLOSEST(n, d) ((n + d/2)/d)

#if 0/* PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION */
# include <math.h>
# define PWM_USE_FLOATING_POINT_ARITHMETICS
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

/* Private typedef -----------------------------------------------------------*/
typedef struct pwm_state_t {
    uint8_t resolution;
} pwm_state_t;

/* Private define ------------------------------------------------------------*/
#define TIM_PWM_COUNTER_CLOCK_FREQ 30000000 //TIM Counter clock = 30MHz
#define TIM_NUM 6
#define TIM_PERIPHERAL_TO_STATE_IDX(tim) (((uint32_t)tim) >= APB2PERIPH_BASE ? \
                                         ((((uint32_t)tim) - APB2PERIPH_BASE) / (TIM8_BASE - TIM1_BASE)) : \
                                         (((((uint32_t)tim) - APB1PERIPH_BASE) / (TIM3_BASE - TIM2_BASE)) + 2))

/* Private variables ---------------------------------------------------------*/
static pwm_state_t PWM_State[TIM_NUM] = {
    // Initialise all timers to 8-bit resolution
    [0 ... (TIM_NUM - 1)].resolution = 8
};

/* Private function prototypes -----------------------------------------------*/
void enableTimClock(uint16_t pin, uint32_t pwm_frequency);
uint8_t timerResolution(uint16_t pin);
uint16_t calculatePrescaler(uint32_t clock, uint32_t pwm_frequency, uint8_t timer_resolution, uint8_t resolution);
uint32_t calculatePeriod(uint32_t clock, uint16_t prescaler, uint32_t pwm_frequency);
uint32_t getPeriod(uint16_t pin);
uint32_t calculatePulse(uint32_t period, uint32_t value, uint8_t resolution);
uint32_t baseClock(uint16_t pin);
uint32_t calculateMaxFrequency(uint32_t clock, uint8_t resolution);
TIM_TimeBaseInitTypeDef calculateTimeBase(uint16_t pin, uint32_t pwm_frequency);
void updateRegisters(uint16_t pin, FunctionalState new_state);
void configureTim(uint16_t pin, uint32_t value);
void enableTim(uint16_t pin);
void updateDutyCycle(uint16_t pin, uint32_t value);
void updateDcFrequency(uint16_t pin, uint32_t value, uint32_t pwm_frequency);

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void hal_pwm_write(uint16_t pin, uint8_t value) {
    hal_pwm_write_with_frequency_ext(pin, (uint16_t)value, TIM_PWM_FREQ);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void hal_pwm_write_ext(uint16_t pin, uint32_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, TIM_PWM_FREQ);
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 */
void hal_pwm_write_with_frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency) {
    hal_pwm_write_with_frequency_ext(pin, (uint16_t)value, pwm_frequency);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 */
void hal_pwm_write_with_frequency_ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency) {
    if ((pwm_frequency == 0) ||
        (pwm_frequency > hal_pwm_get_max_frequency(pin)) ||
        (value >= (1 << hal_pwm_get_resolution(pin)))) {
        return;
    }

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // If PWM has not been initialized, or user has called pinMode(, OUTPUT)
    if (!(pin_info->user_property & PWM_INIT) || pin_info->pin_mode == OUTPUT) {
        // Mark the initialization
        pin_info->user_property |= PWM_INIT;

        // Configure TIM GPIO pins
        HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

        // Enable Timer group clock
        enableTimClock(pin, pwm_frequency);

        // Configure Timer channel
        configureTim(pin, value);

        // TIM enable counter
        enableTim(pin);
    } else {
        updateDcFrequency(pin, value, pwm_frequency);
    }
}

uint16_t hal_pwm_get_frequency(uint16_t pin) {
    return hal_pwm_get_frequency_ext(pin);
}

uint16_t hal_pwm_get_analog_value(uint16_t pin) {
    return hal_pwm_get_analog_value_ext(pin);
}

uint32_t hal_pwm_get_frequency_ext(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (!pin_info->timer_peripheral) {
        return 0;
    }

    uint32_t clock = baseClock(pin);
    uint16_t prescaler = TIM_GetPrescaler(pin_info->timer_peripheral) + 1;
    uint32_t period = getPeriod(pin);
    uint32_t period_cycles = period * prescaler;
    uint32_t pwm_frequency = DIV_ROUND_CLOSEST(clock, period_cycles);
    return pwm_frequency;
}

uint32_t hal_pwm_get_analog_value_ext(uint16_t pin) {
    uint32_t pulse_width = 0;
    uint32_t period = 0;
    uint32_t pwm_analog_value = 0;

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (pin_info->timer_ch == TIM_Channel_1) {
        pulse_width = pin_info->timer_peripheral->CCR1;
    } else if (pin_info->timer_ch == TIM_Channel_2) {
        pulse_width = pin_info->timer_peripheral->CCR2;
    } else if (pin_info->timer_ch == TIM_Channel_3) {
        pulse_width = pin_info->timer_peripheral->CCR3;
    } else if (pin_info->timer_ch == TIM_Channel_4) {
        pulse_width = pin_info->timer_peripheral->CCR4;
    } else {
        return 0;
    }

    period = getPeriod(pin);
    uint32_t max_value = (1 << hal_pwm_get_resolution(pin)) - 1;
#ifndef PWM_USE_FLOATING_POINT_ARITHMETICS
    pwm_analog_value = pulse_width == period + 1 ? max_value : DIV_ROUND_CLOSEST((uint64_t)pulse_width * max_value, period);
#else
    // Use floating point calculations on Electron to save flash space without involving 64-bit integer division
    pwm_analog_value = pulse_width == period + 1 ? max_value : (uint32_t)ceil(((double)pulse_width/(double)period) * max_value);
#endif

    return pwm_analog_value;
}

uint32_t baseClock(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (pin_info->timer_peripheral == TIM3 || pin_info->timer_peripheral == TIM4 || pin_info->timer_peripheral == TIM5) {
        return SystemCoreClock / 2;
    } else {
        return SystemCoreClock;
    }
}

uint8_t timerResolution(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (pin_info->timer_peripheral == TIM2 || pin_info->timer_peripheral == TIM5) {
        return 32;
    }

    return 16;
}

uint32_t calculateMaxFrequency(uint32_t clock, uint8_t resolution) {
    return clock / (1 << resolution);
}

uint32_t hal_pwm_get_max_frequency(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    if (pin_info->timer_peripheral == NULL) {
        return 0;
    }
    return calculateMaxFrequency(baseClock(pin), hal_pwm_get_resolution(pin));
}

uint16_t calculatePrescaler(uint32_t clock, uint32_t pwm_frequency, uint8_t timer_resolution, uint8_t resolution) {
    if (pwm_frequency == 0 || pwm_frequency > calculateMaxFrequency(clock, resolution)) {
        return 0;
    }
    
    uint32_t period_cycles = clock / pwm_frequency;
    uint16_t prescaler = (period_cycles / ((1 << timer_resolution) - 1)) + 1;
    return prescaler;
}

uint32_t calculatePeriod(uint32_t clock, uint16_t prescaler, uint32_t pwm_frequency) {
    uint32_t period_cycles = clock / pwm_frequency;
    uint32_t period = DIV_ROUND_CLOSEST(period_cycles, prescaler);
    return period;
}

uint32_t calculatePulse(uint32_t period, uint32_t value, uint8_t resolution) {
    // Duty Cycle(%) = (value * period / max_value) * 100
    uint32_t max_value = (1 << resolution) - 1;
#ifndef PWM_USE_FLOATING_POINT_ARITHMETICS
    uint32_t pulse_width = max_value == value ? period + 1 : DIV_ROUND_CLOSEST((uint64_t)value * period, max_value);
#else
    // Use floating point calculations on Electron to save flash space without involving 64-bit integer division
    uint32_t pulse_width = max_value == value ? period + 1 : (uint32_t)ceil(((double)value / (double)max_value) * period);
#endif
    return pulse_width;
}

uint32_t getPeriod(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    return pin_info->timer_peripheral->ARR;
}

TIM_TimeBaseInitTypeDef calculateTimeBase(uint16_t pin, uint32_t pwm_frequency) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = { 0 };

    uint32_t clock = baseClock(pin);
    uint16_t prescaler = calculatePrescaler(clock, pwm_frequency, timerResolution(pin), hal_pwm_get_resolution(pin));
    if (prescaler != 0) {
        uint32_t period = calculatePeriod(clock, prescaler, pwm_frequency);

        // Time base configuration
        TIM_TimeBaseStructure.TIM_Period = period;
        TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    }

    return TIM_TimeBaseStructure;
}

void enableTimClock(uint16_t pin, uint32_t pwm_frequency) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

	// AFIO and TIM clock enable

    if (pin_info->timer_peripheral == TIM1) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM1);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else if (pin_info->timer_peripheral == TIM2) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM2);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    } else if (pin_info->timer_peripheral == TIM3) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM3);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    } else if (pin_info->timer_peripheral == TIM4) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM4);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    } else if (pin_info->timer_peripheral == TIM5) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM5);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    else if (pin_info->timer_peripheral == TIM8) {
        GPIO_PinAFConfig(pin_info->gpio_peripheral, pin_info->gpio_pin_source, GPIO_AF_TIM8);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    }
#endif

    // Time base configuration
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = calculateTimeBase(pin, pwm_frequency);
    TIM_TimeBaseInit(pin_info->timer_peripheral, &TIM_TimeBaseStructure);
}


void configureTim(uint16_t pin, uint32_t value) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    //PWM Duty Cycle
    uint32_t period = getPeriod(pin);
    uint8_t resolution = hal_pwm_get_resolution(pin);
    uint32_t pulse_width = calculatePulse(period, value, resolution);

    // PWM1 Mode configuration
    // Initialize all 8 struct params to 0, fixes randomly inverted RX, TX PWM
    TIM_OCInitTypeDef TIM_OCInitStructure = { 0 };
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = pulse_width;

    // Enable output-compare preload function.  Duty cycle will be updated
    // at end of each counter cycle to prevent glitches.
    if (pin_info->timer_ch == TIM_Channel_1) {
        // PWM1 Mode configuration: Channel1
        TIM_OC1Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
    } else if (pin_info->timer_ch == TIM_Channel_2) {
        // PWM1 Mode configuration: Channel2
        TIM_OC2Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(pin_info->timer_peripheral,TIM_OCPreload_Enable);
    } else if (pin_info->timer_ch == TIM_Channel_3) {
        // PWM1 Mode configuration: Channel3
        TIM_OC3Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
    } else if (pin_info->timer_ch == TIM_Channel_4) {
        // PWM1 Mode configuration: Channel4
        TIM_OC4Init(pin_info->timer_peripheral, &TIM_OCInitStructure);
        TIM_OC4PreloadConfig(pin_info->timer_peripheral, TIM_OCPreload_Enable);
    }

    // Enable Auto-load register preload function.  ARR register or PWM period
    // will be update at end of each counter cycle to prevent glitches.
    TIM_ARRPreloadConfig(pin_info->timer_peripheral, ENABLE);
}


void enableTim(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // TIM enable counter
    TIM_Cmd(pin_info->timer_peripheral, ENABLE);

    if ((pin_info->timer_peripheral == TIM1) || (pin_info->timer_peripheral == TIM8)) {
        /* TIM Main Output Enable - required for TIM1/TIM8 PWM output */
        TIM_CtrlPWMOutputs(pin_info->timer_peripheral, ENABLE);
    }
}


void updateDcFrequency(uint16_t pin, uint32_t value, uint32_t pwm_frequency) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // Calculate new prescaler, period and output compare register value
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = calculateTimeBase(pin, pwm_frequency);
    uint8_t resolution = hal_pwm_get_resolution(pin);
    uint32_t pulse_width = calculatePulse(TIM_TimeBaseStructure.TIM_Period, value, resolution);

    // Disable update events while updating registers
    // In case a PWM period ends, it will keep the current values
    updateRegisters(pin, DISABLE);

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
    updateRegisters(pin, ENABLE);
}


void updateRegisters(uint16_t pin, FunctionalState new_state) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    // UpdateDisableConfig = ENABLE means updates are disabled!
    TIM_UpdateDisableConfig(pin_info->timer_peripheral, !new_state);
}

uint8_t hal_pwm_get_resolution(uint16_t pin) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (pin_info->timer_peripheral) {
        return PWM_State[TIM_PERIPHERAL_TO_STATE_IDX(pin_info->timer_peripheral)].resolution;
    }

    return 0;
}

void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution) {
    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;

    if (pin_info->timer_peripheral) {
        if (resolution > 1 && resolution <= (timerResolution(pin) - 1)) {
            PWM_State[TIM_PERIPHERAL_TO_STATE_IDX(pin_info->timer_peripheral)].resolution = resolution;
        }
    }
}
