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
#include "gpio_hal.h"

#define RTL_PWM_INSTANCE_NUM        2
#define KM0_PWM_INSTANCE            0
#define KM4_PWM_INSTANCE            1

#define KM0_PWM_CHANNEL_NUM         6
#define KM4_PWM_CHANNEL_NUM         18
#define MAX_PWM_CHANNEL_NUM         KM4_PWM_CHANNEL_NUM

#define RTL_XTAL_CLOCK_HZ           40000000    // XTAL frequency: 40MHz
#define RTL_PWM_TIM_MAX_RESOLUTION  16          // 16-bits timer
#define RTL_PWM_TIM_MIN_RESOLUTION  2           // As per: https://docs.particle.io/reference/device-os/firmware/#analogwriteresolution-pwm-

#define MAX_PRESCALER               256

#define DEFAULT_RESOLUTION          8

namespace {

typedef enum pwm_state_t {
    PWM_STATE_UNKNOWN,
    PWM_STATE_DISABLED,
    PWM_STATE_ENABLED,
    PWM_STATE_SUSPENDED
} pwm_state_t;

typedef struct rtl_pwm_channel_state_t {
    uint8_t resolution;  // Desired resolution in bits
    uint8_t pin;         // Index in pinmap for this channel
    uint32_t value;      // Desired PWM value, in relation to the resolution
    pwm_state_t state;
} rtl_pwm_channel_state_t;

typedef struct rtl_pwm_info_t {
    RTIM_TypeDef*           tim;
    IRQn                    irqn;
    uint8_t                 timIdx;
    pwm_state_t             state;
    uint32_t                frequency; // Desired output PWM frequency in Hz
    uint32_t                arr;       // 16-bits Auto-Reload register
    rtl_pwm_channel_state_t channels[MAX_PWM_CHANNEL_NUM];
} rtl_pwm_info_t;

rtl_pwm_info_t pwmInfo[RTL_PWM_INSTANCE_NUM] = {
//   tim,    irqn,       timIdx, state,             frequency,        arr, channels
    {TIMM05, TIMER5_IRQ, 5,      PWM_STATE_UNKNOWN, DEFAULT_PWM_FREQ, 0,   {}},
    {TIM5,   TIMER5_IRQ, 5,      PWM_STATE_UNKNOWN, DEFAULT_PWM_FREQ, 0,   {}}
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

void calculateArrAndPrescaler(uint32_t frequency, uint32_t* arr, uint8_t* prescaler) {
    // The PWM output frequency is calculated by the following formula
    // Frequency = (Xtal/prescaler) / arr
    *arr = RTL_XTAL_CLOCK_HZ / frequency;
    uint8_t calculated_prescaler = 0;

    // If the calculated ARR value is larger than will fit in the 16 bit ARR register
    // Increase the prescaler and recalculate until we get a value that fits
    while (*arr > ((1 << RTL_PWM_TIM_MAX_RESOLUTION) - 1) && (calculated_prescaler < MAX_PRESCALER-1)) {
        calculated_prescaler++;
        *arr = (RTL_XTAL_CLOCK_HZ / (calculated_prescaler+1)) / frequency;
    }

    *prescaler = calculated_prescaler;
}

uint32_t minFrequency() {
    return (RTL_XTAL_CLOCK_HZ / MAX_PRESCALER / (1 << RTL_PWM_TIM_MAX_RESOLUTION)) + 1;
}

uint32_t maxFrequency() {
    return RTL_XTAL_CLOCK_HZ / (1 << RTL_PWM_TIM_MIN_RESOLUTION);
}

int pwmTimBaseInit(rtl_pwm_info_t* info) {
    if (info->state == PWM_STATE_ENABLED) {
        return 0;
    }

    // Enable APB clock for the timer
    RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);

    if (info->state == PWM_STATE_UNKNOWN) {
        for( int i = 0; i < MAX_PWM_CHANNEL_NUM; i++){
            info->channels[i].pin = PIN_INVALID;
            info->channels[i].resolution = DEFAULT_RESOLUTION;
            info->channels[i].state = PWM_STATE_DISABLED;
        }
        // Just in case
        RTIM_Cmd(info->tim, DISABLE);
    }

    RTIM_TimeBaseInitTypeDef baseInitStruct = {};
	RTIM_TimeBaseStructInit(&baseInitStruct);
	baseInitStruct.TIM_Idx = info->timIdx;

	RTIM_TimeBaseInit(info->tim, &baseInitStruct, info->irqn, nullptr, (uint32_t)&baseInitStruct);
	RTIM_Cmd(info->tim, ENABLE);

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
    const pwm_state_t channelState = pwmInfo[instance].channels[channel].state;

    if (channelState == PWM_STATE_DISABLED || channelState == PWM_STATE_SUSPENDED) {
        TIM_CCInitTypeDef ccInitStruct = {};
        RTIM_CCStructInit(&ccInitStruct);
        RTIM_CCxInit(pwmInfo[instance].tim, &ccInitStruct, channel);
        pwmInfo[instance].channels[channel].pin = pin;
        pwmInfo[instance].channels[channel].state = PWM_STATE_ENABLED;

        const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
        if (instance == KM0_PWM_INSTANCE) {
            Pinmux_Config(rtlPin, PINMUX_FUNCTION_PWM_LP);
        } else {
            Pinmux_Config(rtlPin, PINMUX_FUNCTION_PWM_HS);
        }
        hal_gpio_set_drive_strength(pin, HAL_GPIO_DRIVE_HIGH);
        hal_pin_set_function(pin, PF_PWM);
    }
    return 0;
}

int pwmPinDeinit(uint16_t pin) {
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    const uint8_t channel = pinInfo->pwm_channel;
    const pwm_state_t channelState = pwmInfo[instance].channels[channel].state;

    if (channelState == PWM_STATE_ENABLED) {
        const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
        Pinmux_Config(rtlPin, PINMUX_FUNCTION_GPIO);
        hal_gpio_set_drive_strength(pin, HAL_GPIO_DRIVE_DEFAULT);

        pwmInfo[instance].channels[channel].state = PWM_STATE_DISABLED;
        hal_pin_set_function(pin, PF_NONE);
    }
    return 0;
}

// Set the CCRx register (duty cycle) according to the user value and resolution
void pwmSetCcr(uint8_t instance, uint8_t channel, uint32_t userValue) {
    uint32_t expectedPeriodCycles = (1 << pwmInfo[instance].channels[channel].resolution) - 1;
    uint32_t timCcr = ((float)userValue / (float)expectedPeriodCycles) * pwmInfo[instance].arr;
    RTIM_CCRxSet(pwmInfo[instance].tim, timCcr, channel);
}

void pwmSetArr(uint8_t instance, uint32_t arr) {
    RTIM_ChangePeriod(pwmInfo[instance].tim, arr - 1); // Minus 1 so that CCRx can be set to aar to output 100% duty cycle.
    pwmInfo[instance].arr = arr;

    // Make sure the other enabled channels' duty cycle is consistent
    for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
        if (pwmInfo[instance].channels[ch].state == PWM_STATE_ENABLED) {
            pwmSetCcr(instance, ch, pwmInfo[instance].channels[ch].value);
        }
    }
}

} // anonymous

/*
 * Rresolution always takes the priority. If the user frequency is below the
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

    if (frequency < minFrequency() || frequency > maxFrequency()) {
        return;
    }

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }

    pwmPinInit(pin);

    pwmInfo[instance].frequency = frequency;
    if (value > ((uint32_t)(1 << pwmInfo[instance].channels[channel].resolution) - 1)) {
        value = (uint32_t)((1 << pwmInfo[instance].channels[channel].resolution) - 1);
    }
    pwmInfo[instance].channels[channel].value = value;

    // WARN: Changing the prescaler, the frequency of other channels those are using the same TIM
    // will be changed to this frequency automatically.
    uint32_t arr;
    uint8_t prescaler;
    calculateArrAndPrescaler(frequency, &arr, &prescaler);
    RTIM_PrescalerConfig(pwmInfo[instance].tim, prescaler, TIM_PSCReloadMode_Update);
    // Set resolution
    pwmSetArr(instance, arr);

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

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }

    if (value > ((uint32_t)(1 << pwmInfo[instance].channels[channel].resolution) - 1)) {
        value = (uint32_t)((1 << pwmInfo[instance].channels[channel].resolution) - 1);
    }
    pwmInfo[instance].channels[channel].value = value;

    // Set the pulse width
    pwmSetCcr(instance, channel, pwmInfo[instance].channels[channel].value);
}

void hal_pwm_update_duty_cycle(uint16_t pin, uint16_t value) {
    hal_pwm_update_duty_cycle_ext(pin, value);
}

//TODO: implement function to get minimum frequency
uint32_t hal_pwm_get_max_frequency(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    return maxFrequency();
}

uint32_t hal_pwm_get_frequency_ext(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    // All of the channels using the same TIM are outputing the same frequency.
    // And we should return the current actual output frequency
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    return pwmInfo[instance].frequency;
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

    return pwmInfo[instance].channels[channel].value;
}

uint8_t hal_pwm_get_resolution(uint16_t pin) {
    if (!isPwmPin(pin)) {
        return 0;
    }
    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    uint8_t channel = pinInfo->pwm_channel;
    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return 0;
        }
    }
    return pwmInfo[instance].channels[channel].resolution;
}

void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution) {
    if (!isPwmPin(pin) || resolution > RTL_PWM_TIM_MAX_RESOLUTION || resolution < RTL_PWM_TIM_MIN_RESOLUTION) {
        return;
    }

    const hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    const uint8_t instance = pinInfo->pwm_instance;
    uint8_t channel = pinInfo->pwm_channel;

    if (pwmInfo[instance].state != PWM_STATE_ENABLED) {
        if (pwmTimBaseInit(&pwmInfo[instance]) != 0) {
            return;
        }
    }

    uint32_t prePeriodCycles = (1 << pwmInfo[instance].channels[channel].resolution);
    uint32_t newPeriodCycles = (1 << resolution);
    pwmInfo[instance].channels[channel].resolution = resolution;
    pwmInfo[instance].channels[channel].value = ((float)pwmInfo[instance].channels[channel].value / prePeriodCycles) * newPeriodCycles;
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
        if (pwmInfo[instance].channels[ch].state == PWM_STATE_ENABLED) {
            disableTim = false;
            break;
        }
    }
    if (disableTim) {
        pwmTimBaseDeinit(&pwmInfo[instance]);
    }
}

int hal_pwm_sleep(bool sleep, void* reserved) {
    if (sleep) {
        for (uint8_t instance = 0; instance < RTL_PWM_INSTANCE_NUM; instance++) {
            if (pwmInfo[instance].state == PWM_STATE_ENABLED) {
                for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
                    if (pwmInfo[instance].channels[ch].state == PWM_STATE_ENABLED) {
                        // Stop PWM
                        RTIM_CCxCmd(pwmInfo[instance].tim, ch, TIM_CCx_Disable);
                        // Configure as GPIO
                        pwmPinDeinit(pwmInfo[instance].channels[ch].pin);
                        pwmInfo[instance].channels[ch].state = PWM_STATE_SUSPENDED;
                    }
                }
                // Disable timer
                pwmTimBaseDeinit(&pwmInfo[instance]);
                pwmInfo[instance].state = PWM_STATE_SUSPENDED;
            }
        }
    } else {
        for (uint8_t instance = 0; instance < RTL_PWM_INSTANCE_NUM; instance++) {
            if (pwmInfo[instance].state == PWM_STATE_SUSPENDED) {
                // Start timer
                pwmTimBaseInit(&pwmInfo[instance]);
                for (uint8_t ch = 0; ch < MAX_PWM_CHANNEL_NUM; ch++) {
                    if (pwmInfo[instance].channels[ch].state == PWM_STATE_SUSPENDED) {
                        // Re-start PWM
                        hal_pwm_write_with_frequency_ext(pwmInfo[instance].channels[ch].pin, pwmInfo[instance].channels[ch].value, pwmInfo[instance].frequency);
                    }
                }
            }
        }
    }
    return 0;
}
