/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
#ifdef __cplusplus
}
#endif
#include <cstring>
#include "pwm_hal.h"

#define RTL_PWM_INSTANCE_NUM    2
#define KM0_PWM_INSTANCE        0
#define KM4_PWM_INSTANCE        1

#define KM0_PWM_CHANNEL_NUM     6
#define KM4_PWM_CHANNEL_NUM     18
#define MAX_PWM_CHANNEL_NUM     KM4_PWM_CHANNEL_NUM

#define RTL_XTAL_CLOCK_HZ       40000000 // XTAL frequency: 40MHz
#define RTL_PWM_TIM_RESOLUTION  16 // 16-bits timer

#define MAX_PRESCALER           255

namespace {

typedef enum pwm_state_t {
    PWM_STATE_UNKNOWN,
    PWM_STATE_DISABLED,
    PWM_STATE_ENABLED,
    PWM_STATE_SUSPENDED
} pwm_state_t;

typedef struct rtl_pwm_info_t {
    RTIM_TypeDef*           tim;
    IRQn                    irqn;
    uint8_t                 timIdx;
    uint8_t                 resolution;
    pwm_state_t             state;
    uint32_t                frequency; // Actual output PWM frequency
    uint32_t                desiredFreqs[MAX_PWM_CHANNEL_NUM]; // Desired frequency
    uint8_t                 pins[MAX_PWM_CHANNEL_NUM];
    uint32_t                values[MAX_PWM_CHANNEL_NUM];
} rtl_pwm_info_t;

rtl_pwm_info_t pwmInfo[RTL_PWM_INSTANCE_NUM] = {
//   tim,    irqn,       timIdx, resolution,  state,            frequency
    {TIMM05, TIMER5_IRQ, 5,      10,          PWM_STATE_UNKNOWN, 0},
    {TIM5,   TIMER5_IRQ, 5,      10,          PWM_STATE_UNKNOWN, 0}
};

bool isPwmPin(uint16_t pin) {
    if (!hal_pin_is_valid(pin)) {
        return false;
    }
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    uint8_t instance = pinInfo->pwm_instance;
    if (instance == PWM_INSTANCE_NONE || instance >= RTL_PWM_INSTANCE_NUM) {
        return false;
    }
    uint8_t channel = pinInfo->pwm_channel;
    if ((instance == KM0_PWM_INSTANCE && channel >= KM0_PWM_CHANNEL_NUM) ||
        (instance == KM4_PWM_INSTANCE && channel >= KM4_PWM_CHANNEL_NUM)) {
        return false;
    }
    return true;
}

uint8_t calculatePrescaler(uint16_t resolution, uint32_t frequency) {
    uint32_t periodCycles = RTL_XTAL_CLOCK_HZ / frequency;
    uint8_t prescaler = periodCycles / (1 << resolution);
    if (prescaler != 0) {
        prescaler -= 1;
    }
    return prescaler;
}

uint32_t calculateMinFrequency(uint16_t resolution) {
    return RTL_XTAL_CLOCK_HZ / MAX_PRESCALER / (1 << resolution);
}

uint32_t calculateMaxFrequency(uint16_t resolution) {
    return RTL_XTAL_CLOCK_HZ / (1 << resolution);
}

int pwmTimBaseInit(rtl_pwm_info_t* info) {
    // Enable APB clock for the timer
    RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);

    if (info->state == PWM_STATE_UNKNOWN) {
        memset(info->pins, PIN_INVALID, sizeof(info->pins));
        // Juts in case
        RTIM_Cmd(info->tim, DISABLE);
        info->state = PWM_STATE_DISABLED;
    }

    RTIM_TimeBaseInitTypeDef baseInitStruct = {};
	RTIM_TimeBaseStructInit(&baseInitStruct);
	baseInitStruct.TIM_Idx = info->timIdx;

	RTIM_TimeBaseInit(info->tim, &baseInitStruct, info->irqn, nullptr, (uint32_t)&baseInitStruct);
	RTIM_Cmd(info->tim, ENABLE);

    // Set the resolution
    RTIM_ChangePeriod(info->tim, (1 << info->resolution) - 1);

	info->state = PWM_STATE_ENABLED;
    return 0;
}

int pwmTimBaseDeinit(rtl_pwm_info_t* info) {
    RTIM_Cmd(info->tim, DISABLE);
    // This may further disbale clock for the timer
    RTIM_DeInit(info->tim);
    info->state = PWM_STATE_DISABLED;
    return 0;
}

int pwmPinInit(uint16_t pin) {
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint8_t channel = pinInfo->pwm_channel;

    if (pwmInfo[instance].pins[channel] == PIN_INVALID) {
        TIM_CCInitTypeDef ccInitStruct = {};
        RTIM_CCStructInit(&ccInitStruct);
        RTIM_CCxInit(pwmInfo[instance].tim, &ccInitStruct, channel);

        const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
        if (instance == KM0_PWM_INSTANCE) {
            Pinmux_Config(rtlPin, PINMUX_FUNCTION_PWM_LP);
        } else {
            Pinmux_Config(rtlPin, PINMUX_FUNCTION_PWM_HS);
        }

        pwmInfo[instance].pins[channel] = pin;
        hal_pin_set_function(pin, PF_PWM);
    }
    return 0;
}

int pwmPinDeinit(uint16_t pin) {
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint8_t channel = pinInfo->pwm_channel;

    if (pwmInfo[instance].pins[channel] != PIN_INVALID) {
        const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
        Pinmux_Config(rtlPin, PINMUX_FUNCTION_GPIO);

        pwmInfo[instance].pins[channel] = PIN_INVALID;
        hal_pin_set_function(pin, PF_NONE);
    }
    return 0;
}

} // anonymous

/*
 * Rresolution always takes the priority. If the desired frequency is below the
 * maximum allowed frequency based on the current resolution, the highest
 * frequency should apply to all other channels, but make sure their duty
 * cycle is consistent.
 */
void hal_pwm_write_with_frequency_ext(uint16_t pin, uint32_t value, uint32_t frequency) {
    if (!isPwmPin(pin)) {
        return;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    uint8_t instance = pinInfo->pwm_instance;
    uint8_t channel = pinInfo->pwm_channel;

    pwmInfo[instance].values[channel] = value;
    pwmInfo[instance].desiredFreqs[channel] = frequency;

    if (value > (uint32_t)(1 << pwmInfo[instance].resolution)) {
        // FIXME: to not break the user experience we simply return.
        // But can we instead round the value to the maximum value?
        return;
    }

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }

    pwmPinInit(pin);

    uint32_t maxDesiredFreq = 0;
    for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
        if (pwmInfo[instance].pins[ch] == PIN_INVALID) {
            continue;
        }
        if (pwmInfo[instance].desiredFreqs[ch] > maxDesiredFreq) {
            maxDesiredFreq = pwmInfo[instance].desiredFreqs[ch];
        }
    }
    uint32_t minFreq = calculateMinFrequency(pwmInfo[instance].resolution);
    if (maxDesiredFreq < minFreq) {
        maxDesiredFreq = minFreq;
    }
    uint32_t maxFreq = calculateMaxFrequency(pwmInfo[instance].resolution);
    if (maxDesiredFreq > maxFreq) {
        maxDesiredFreq = maxFreq;
    }

    if (maxDesiredFreq != pwmInfo[instance].frequency) {
        // WARN: Changing the prescaler, the frequency of other channels those use the same TIM
        // is changed to this frequency automatically.
        uint8_t prescaler = calculatePrescaler(pwmInfo[instance].resolution, maxDesiredFreq);
        RTIM_PrescalerConfig(pwmInfo[instance].tim, prescaler, TIM_PSCReloadMode_Update);
        pwmInfo[instance].frequency = maxDesiredFreq;
    }
    
    // Set the pulse width
	RTIM_CCRxSet(pwmInfo[instance].tim, value, channel);

    // Start PWM
    RTIM_CCxCmd(pwmInfo[instance].tim, channel, TIM_CCx_Enable);
}

void hal_pwm_write(uint16_t pin, uint8_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

void hal_pwm_write_ext(uint16_t pin, uint32_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

void hal_pwm_write_with_frequency(uint16_t pin, uint8_t value, uint16_t frequency) {
    hal_pwm_write_with_frequency_ext(pin, value, frequency);
}

void hal_pwm_update_duty_cycle_ext(uint16_t pin, uint32_t value) {
    if (!isPwmPin(pin)) {
        return;
    }
    
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint32_t channel = pinInfo->pwm_channel;

    if (value > (uint32_t)(1 << pwmInfo[instance].resolution)) {
        value = (uint32_t)(1 << pwmInfo[instance].resolution);
    }

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }
    RTIM_CCRxSet(pwmInfo[instance].tim, value, channel);
    pwmInfo[instance].values[channel] = value;
}

void hal_pwm_update_duty_cycle(uint16_t pin, uint16_t value) {
    hal_pwm_update_duty_cycle_ext(pin, value);
}

//TODO: implement function to get minimum frequency
uint32_t hal_pwm_get_max_frequency(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    return calculateMaxFrequency(pwmInfo[instance].resolution);
}

uint32_t hal_pwm_get_frequency_ext(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    // All of the channels using the same TIM are outputing the same frequency.
    // And we should return the current actual output frequency
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint8_t channel = pinInfo->pwm_channel;
    return pwmInfo[instance].desiredFreqs[channel];
}

uint16_t hal_pwm_get_frequency(uint16_t pin) {
    return hal_pwm_get_frequency_ext(pin);
}

uint16_t hal_pwm_get_analog_value(uint16_t pin) {
    return hal_pwm_get_analog_value_ext(pin);
}

uint32_t hal_pwm_get_analog_value_ext(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    uint8_t instance = pinInfo->pwm_instance;
    uint8_t channel = pinInfo->pwm_channel;

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        return 0;
    }

    return pwmInfo[instance].values[channel];
}

uint8_t hal_pwm_get_resolution(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    return pwmInfo[instance].resolution;
}

void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution) {
    if (!isPwmPin(pin)) {
        return;
    }
    
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }
    
    uint32_t prePeriodCycles = (1 << pwmInfo[instance].resolution);
    uint32_t currPeriodCycles = (1 << resolution);

    RTIM_ChangePeriod(pwmInfo[instance].tim, (1 << resolution) - 1);
    pwmInfo[instance].resolution = resolution;

    // Make sure the frequency is consistent and meet the minimum/maximum frequency requirement
    uint32_t newFrequency = pwmInfo[instance].frequency;
    uint32_t minFrequency = calculateMinFrequency(pwmInfo[instance].resolution);
    uint32_t maxFrequency = calculateMaxFrequency(pwmInfo[instance].resolution);
    if (pwmInfo[instance].frequency < minFrequency) {
        newFrequency = minFrequency;
    } else if (pwmInfo[instance].frequency > maxFrequency) {
        newFrequency = maxFrequency;
    }
    if (newFrequency != pwmInfo[instance].frequency) {
        pwmInfo[instance].frequency = newFrequency;
        uint8_t prescaler = calculatePrescaler(pwmInfo[instance].resolution, newFrequency);
        RTIM_PrescalerConfig(pwmInfo[instance].tim, prescaler, TIM_PSCReloadMode_Update);
    }

    // Make sure the duty cycle is consistent
    for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
        if (pwmInfo[instance].pins[ch] != PIN_INVALID) {
            uint32_t value = ((float)pwmInfo[instance].values[ch] / prePeriodCycles) * currPeriodCycles;
            hal_pwm_update_duty_cycle_ext(pwmInfo[instance].pins[ch], value);
        }
    }
}

void hal_pwm_reset_pin(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return;
    }
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint8_t channel = pinInfo->pwm_channel;

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        return;
    }

    // Stop PWM
    RTIM_CCxCmd(pwmInfo[instance].tim, channel, TIM_CCx_Disable);

    pwmPinDeinit(pin);

    // Disable the timer if all channels are deactivated
    bool disableTim = true;
    for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
        if (pwmInfo[instance].pins[ch] != PIN_INVALID) {
            disableTim = false;
            break;
        }
    }
    if (disableTim) {
        pwmTimBaseDeinit(&pwmInfo[instance]);
    }
}

// TODO: verify it
int hal_pwm_sleep(bool sleep, void* reserved) {
    if (sleep) {
        for (uint8_t instance = 0; instance < 1; instance++) {
            if (pwmInfo[instance].state == PWM_STATE_ENABLED) {
                for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
                    if (pwmInfo[instance].pins[ch] != PIN_INVALID) {
                        // Stop PWM
                        RTIM_CCxCmd(pwmInfo[instance].tim, ch, TIM_CCx_Disable);
                        // Configure as GPIO
                        pwmPinDeinit(pwmInfo[instance].pins[ch]);
                    }
                }
                // Disable timer
                pwmTimBaseDeinit(&pwmInfo[instance]);
                pwmInfo[instance].state = PWM_STATE_SUSPENDED;
            }
        }
    } else {
        for (uint8_t instance = 0; instance < 1; instance++) {
            if (pwmInfo[instance].state == PWM_STATE_SUSPENDED) {
                // Start timer
                pwmTimBaseInit(&pwmInfo[instance]);
                for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
                    if (pwmInfo[instance].pins[ch] != PIN_INVALID) {
                        // Configure as GPIO
                        pwmPinInit(pwmInfo[instance].pins[ch]);
                        // Start PWM
                        RTIM_CCxCmd(pwmInfo[instance].tim, ch, TIM_CCx_Disable);
                    }
                }
            }
        }
    }
    return 0;
}
