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

#include "nrfx_pwm.h"
#include "nrf_gpio.h"
#include "pwm_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"

#define NRF5X_PWM_COUNT                     4
#define PWM_CHANNEL_NUM                     4
#define MAX_PWM_COUNTERTOP                  0x7FFF    // 15bit
#define DEFAULT_PWM_FREQ                    500       // 500Hz
#define MAX_PWM_FREQ                        500000    // 500kHz, base clock 16MHz, resolution value 16
#define MAX_PWM_PERIOD_US                   2         // 2us
#define MIN_PWM_FREQ                        4         // 4Hz
#define MIN_PWM_PERIOD_US                   250000    // 250ms

typedef struct {
    nrf_pwm_values_common_t                 duty_hwu;    // duty
    nrf_pwm_clk_t                           pwm_clock;   // for base clock, base clock and period_hwu decide the period time
    uint16_t                                period_hwu;  // period hardware unit, for top value
} pwm_setting_t;

typedef struct NRF5x_PWM_Info {
    nrfx_pwm_t                              pwm;
    uint8_t                                 nrf_pins[4];
    uint32_t                                values[4];

    bool                                    enabled;
    uint32_t                                frequency;
    nrf_pwm_values_individual_t             seq_value;
} NRF5x_PWM_Info;

NRF5x_PWM_Info PWM_MAP[NRF5X_PWM_COUNT] = {
    {NRFX_PWM_INSTANCE(0), {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(1), {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(2), {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(3), {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}}
};


static bool get_pwm_clock_setting(uint32_t value, uint32_t frequency, pwm_setting_t *p_setting)
{
    // adjust parameter to supported range
    uint32_t period_us;
    uint32_t duty_us;

    frequency = (frequency > MAX_PWM_FREQ) ? MAX_PWM_FREQ : frequency;
    frequency = (frequency < MIN_PWM_FREQ) ? MIN_PWM_FREQ : frequency;
    period_us = 1000000 / frequency;

    if (value)
    {
        // when the frequency is too high, keep the duty cycle to approximation
        duty_us = (value + 1) * period_us / (MAX_PWM_COUNTERTOP + 1);
        duty_us = duty_us ? duty_us : 1;
    }
    else
    {
        duty_us = 0;
    }

    uint32_t period_hwu = period_us * 16;
    uint32_t duty_hwu = duty_us * 16;

    uint8_t nrf_pwm_clk_index = 0;
    nrf_pwm_clk_t nrf_pwm_clk_buf[8] = {
        NRF_PWM_CLK_16MHz, NRF_PWM_CLK_8MHz, NRF_PWM_CLK_4MHz, NRF_PWM_CLK_2MHz,
        NRF_PWM_CLK_1MHz, NRF_PWM_CLK_500kHz, NRF_PWM_CLK_250kHz, NRF_PWM_CLK_125kHz
    };

    for(uint16_t div = 1; div <= 128 ; div <<= 1)
    {
        if (MAX_PWM_COUNTERTOP >= period_hwu)
        {
            p_setting->duty_hwu   = duty_hwu;
            p_setting->period_hwu = period_hwu;
            p_setting->pwm_clock  = nrf_pwm_clk_buf[nrf_pwm_clk_index];

            return true;
        }

        period_hwu >>= 1;
        duty_hwu >>= 1;
        nrf_pwm_clk_index++;
    }

    return false;
}

int uninit_pwm_pin(uint16_t pin)
{
    uint32_t         ret_code;
    pwm_setting_t    pwm_setting;

    NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
    uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;
    uint8_t          pwm_channel = PIN_MAP[pin].pwm_channel;

    PWM_MAP[pwm_num].nrf_pins[pwm_channel] = NRFX_PWM_PIN_NOT_USED;
    PWM_MAP[pwm_num].values[pwm_channel] = 0;

    if (PWM_MAP[pwm_num].enabled)
    {
        nrfx_pwm_uninit(&PWM_MAP[pwm_num].pwm);
    }

    // reset pin mode
    nrf_gpio_cfg_default(NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin));
    PIN_MAP[pin].pin_func = PF_NONE;

    // reconfigure pwm
    for (int i = 0; i < PWM_CHANNEL_NUM; i++)
    {
        if (PWM_MAP[pwm_num].nrf_pins[i] != NRFX_PWM_PIN_NOT_USED)
        {
            if (get_pwm_clock_setting(PWM_MAP[pwm_num].values[i], PWM_MAP[pwm_num].frequency, &pwm_setting) == false)
            {
                continue;
            }

            // In each value, the most significant bit (15) determines the polarity of the output
            ((uint16_t *)&(PWM_MAP[pwm_num].seq_value))[i] = pwm_setting.duty_hwu;
        }
    }

    nrfx_pwm_config_t const config = {
        .output_pins =
        {
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[0] | NRFX_PWM_PIN_INVERTED),   // channel 0
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[1] | NRFX_PWM_PIN_INVERTED),   // channel 1
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[2] | NRFX_PWM_PIN_INVERTED),   // channel 2
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[3] | NRFX_PWM_PIN_INVERTED),   // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = pwm_setting.pwm_clock,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = pwm_setting.period_hwu,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    ret_code = nrfx_pwm_init(&PWM_MAP[pwm_num].pwm, &config, NULL);
    if (ret_code)
    {
        return -1;
    }
    PWM_MAP[pwm_num].enabled = true;

    nrf_pwm_sequence_t const seq = {
        .values = {.p_individual = (nrf_pwm_values_individual_t*) &PWM_MAP[pwm_num].seq_value},
        .length          = NRF_PWM_VALUES_LENGTH(PWM_MAP[pwm_num].seq_value),
        .repeats         = 0,
        .end_delay       = 0
    };

    ret_code = nrfx_pwm_simple_playback(&PWM_MAP[pwm_num].pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
    if (ret_code)
    {
        return -2;
    }

    return 0;
}

static int init_pwm_pin(uint32_t pin, uint32_t value, uint32_t frequency)
{
    ret_code_t      ret_code;
    pwm_setting_t   pwm_setting;

    NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
    uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;
    uint8_t          pwm_channel = PIN_MAP[pin].pwm_channel;

    PWM_MAP[pwm_num].nrf_pins[pwm_channel] = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
    PWM_MAP[pwm_num].values[pwm_channel] = value;

    if (PWM_MAP[pwm_num].enabled)
    {
        nrfx_pwm_uninit(&PWM_MAP[pwm_num].pwm);
    }

    // if frequency is changed, all the pins in the same pwm module should be reconfigured
    for (int i = 0; i < 4; i++)
    {
        if (PWM_MAP[pwm_num].nrf_pins[i] != NRFX_PWM_PIN_NOT_USED)
        {
            if (get_pwm_clock_setting(PWM_MAP[pwm_num].values[i], frequency, &pwm_setting) == false)
            {
                continue;
            }

            // In each value, the most significant bit (15) determines the polarity of the output
            ((uint16_t *)&(PWM_MAP[pwm_num].seq_value))[i] = pwm_setting.duty_hwu;
        }
    }

    PWM_MAP[pwm_num].frequency = frequency;

    nrfx_pwm_config_t const config = {
        .output_pins =
        {
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[0] | NRFX_PWM_PIN_INVERTED),   // channel 0
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[1] | NRFX_PWM_PIN_INVERTED),   // channel 1
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[2] | NRFX_PWM_PIN_INVERTED),   // channel 2
            (uint8_t)(PWM_MAP[pwm_num].nrf_pins[3] | NRFX_PWM_PIN_INVERTED),   // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = pwm_setting.pwm_clock,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = pwm_setting.period_hwu,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    ret_code = nrfx_pwm_init(&PWM_MAP[pwm_num].pwm, &config, NULL);
    if (ret_code)
    {
        return -2;
    }
    PWM_MAP[pwm_num].enabled = true;

    nrf_pwm_sequence_t const seq = {
        .values = {.p_individual = (nrf_pwm_values_individual_t*) &PWM_MAP[pwm_num].seq_value},
        .length          = NRF_PWM_VALUES_LENGTH(PWM_MAP[pwm_num].seq_value),
        .repeats         = 0,
        .end_delay       = 0
    };


    ret_code = nrfx_pwm_simple_playback(&PWM_MAP[pwm_num].pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
    if (ret_code)
    {
        return -3;
    }
    HAL_Set_Pin_Function(pin, PF_PWM);

    return 0;
}

void HAL_PWM_Reset_Pin(uint16_t pin)
{
    if (pin >= TOTAL_PINS)
    {
        return;
    }

    uninit_pwm_pin(pin);
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * default PWM frequent is set at 500 Hz
 */
void HAL_PWM_Write(uint16_t pin, uint8_t value)
{
    uint32_t raw_value = (value == 0) ? 0 : (value + 1) * MAX_PWM_COUNTERTOP / 256;
    HAL_PWM_Write_With_Frequency_Ext(pin, raw_value, DEFAULT_PWM_FREQ);
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 */
void HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency)
{
    uint32_t raw_value = (value == 0) ? 0 : (value + 1) * MAX_PWM_COUNTERTOP / 256;
    HAL_PWM_Write_With_Frequency_Ext(pin, raw_value, pwm_frequency);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%.
 * DEFAULT_PWM_FREQ is set at 500 Hz
 */
void HAL_PWM_Write_Ext(uint16_t pin, uint32_t value)
{
    HAL_PWM_Write_With_Frequency_Ext(pin, value, DEFAULT_PWM_FREQ);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 *
 * frequency range: 4Hz ~ 500KHz, frequency higher than 500KHz will adjust to 500KHz,
 *                 frequency lower than 4Hz will adjust to 4Hz
 */
void HAL_PWM_Write_With_Frequency_Ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency)
{
    if ((pin >= TOTAL_PINS)            ||
        (pwm_frequency > MAX_PWM_FREQ) ||
        (pwm_frequency < MIN_PWM_FREQ) ||
        (value > MAX_PWM_COUNTERTOP))
    {
        return;
    }

    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (PIN_MAP[pin].pwm_instance == PWM_INSTANCE_NONE)
    {
        return;
    }

    if(init_pwm_pin(pin, value, pwm_frequency))
    {
        return;
    }
}

uint16_t HAL_PWM_Get_Frequency(uint16_t pin)
{
    return HAL_PWM_Get_Frequency_Ext(pin);
}

uint16_t HAL_PWM_Get_AnalogValue(uint16_t pin)
{
    return HAL_PWM_Get_AnalogValue_Ext(pin);
}

uint32_t HAL_PWM_Get_Frequency_Ext(uint16_t pin)
{
    if (pin >= TOTAL_PINS)
    {
        return 0;
    }

    NRF5x_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    uint8_t pwm_num = pin_info->pwm_instance;
    if (pwm_num == PWM_INSTANCE_NONE)
    {
        return 0;
    }

    return PWM_MAP[pwm_num].frequency;
}

uint32_t HAL_PWM_Get_AnalogValue_Ext(uint16_t pin)
{
    if (pin >= TOTAL_PINS)
    {
        return 0;
    }

    NRF5x_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    uint8_t pwm_num = pin_info->pwm_instance;
    uint8_t pwm_channel = pin_info->pwm_channel;
    if (pwm_num == PWM_INSTANCE_NONE)
    {
        return 0;
    }

    if (!PWM_MAP[pwm_num].enabled)
    {
        return 0;
    }

    return PWM_MAP[pwm_num].values[pwm_channel];
}

uint32_t HAL_PWM_Get_Max_Frequency(uint16_t pin)
{
    return MAX_PWM_FREQ;
}

void HAL_PWM_UpdateDutyCycle(uint16_t pin, uint16_t value)
{
    HAL_PWM_Write_With_Frequency_Ext(pin, value, DEFAULT_PWM_FREQ);
}

void HAL_PWM_UpdateDutyCycle_Ext(uint16_t pin, uint32_t value)
{
    HAL_PWM_Write_With_Frequency_Ext(pin, value, DEFAULT_PWM_FREQ);
}

uint8_t HAL_PWM_Get_Resolution(uint16_t pin)
{
    // Use the highest resolution, 15 bits
    return 15;
}

void HAL_PWM_Set_Resolution(uint16_t pin, uint8_t resolution)
{
    // Deprecated
}
