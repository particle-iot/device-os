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

#include "nrfx_pwm.h"
#include "nrf_gpio.h"
#include "pwm_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "check.h"

#define NRF5X_PWM_COUNT                     4
#define PWM_CHANNEL_NUM                     4
#define MAX_PWM_COUNTERTOP                  0x7FFF    // 15bit
#define MAX_PWM_FREQ                        500000    // 500kHz, base clock 16MHz, resolution value 16
#define MAX_PWM_PERIOD_US                   2         // 2us
#define MIN_PWM_FREQ                        4         // 4Hz
#define MIN_PWM_PERIOD_US                   250000    // 250ms
#define DEFAULT_RESOLUTION_BITS             8
#define MAX_RESOLUTION_BITS                 15


typedef struct pwm_setting_t {
    nrf_pwm_values_common_t                 duty_hwu;    // duty
    nrf_pwm_clk_t                           pwm_clock;   // for base clock, base clock and period_hwu decide the period time
    uint16_t                                period_hwu;  // period hardware unit, for top value
} pwm_setting_t;

typedef struct nrf5x_pwm_info_t {
    nrfx_pwm_t                              pwm;
    uint32_t                                frequency;
    uint8_t                                 pins[4];
    uint32_t                                values[4];

    bool                                    enabled;
    nrf_pwm_values_individual_t             seq_value;
} nrf5x_pwm_info_t;

nrf5x_pwm_info_t pwmMap[NRF5X_PWM_COUNT] = {
    {NRFX_PWM_INSTANCE(0), 500, {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(1), 500, {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(2), 500, {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}},
    {NRFX_PWM_INSTANCE(3), 500, {NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED}}
};

static inline uint8_t getNrfPin(uint8_t pin) {
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (pin == PIN_INVALID) {
        return pin;
    }
    uint8_t p = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
    if (HAL_Get_Pin_Mode(pin) == OUTPUT) {
        // If GPIO is configured as output and is set to high, tell nrfx_pwm
        // that we want this pin to be high when PWM is off
        if (HAL_GPIO_Read(pin) == 1) {
            p |= NRFX_PWM_PIN_INVERTED;
        }
    }
    return p;
}

static bool getClockSetting(uint32_t value, uint32_t frequency, uint8_t resolution, pwm_setting_t *p_setting) {
    uint32_t period_us, duty_us;

    frequency = (frequency > MAX_PWM_FREQ) ? MAX_PWM_FREQ : frequency;
    frequency = (frequency < MIN_PWM_FREQ) ? MIN_PWM_FREQ : frequency;
    period_us = 1000000 / frequency;

    // invert value and convert to 15bits
    value = (1 << resolution) - 1 - value;
    value = (value == 0) ? 0 : (value + 1) * MAX_PWM_COUNTERTOP / (1 << resolution);

    if (value) {
        // when the frequency is too high, keep the duty cycle to approximation
        duty_us = (value + 1) * period_us / (MAX_PWM_COUNTERTOP + 1);
        duty_us = duty_us ? duty_us : 1;
    } else {
        duty_us = 0;
    }

    uint32_t period_hwu = period_us * 16;
    uint32_t duty_hwu = duty_us * 16;

    uint8_t nrf_pwm_clk_index = 0;
    nrf_pwm_clk_t nrf_pwm_clk_buf[8] = {
        NRF_PWM_CLK_16MHz, NRF_PWM_CLK_8MHz,   NRF_PWM_CLK_4MHz,   NRF_PWM_CLK_2MHz,
        NRF_PWM_CLK_1MHz,  NRF_PWM_CLK_500kHz, NRF_PWM_CLK_250kHz, NRF_PWM_CLK_125kHz
    };

    for (uint16_t div = 1; div <= 128 ; div <<= 1) {
        if (MAX_PWM_COUNTERTOP >= period_hwu) {
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

int pwmPinUninit(uint16_t pin) {
    uint32_t         ret_code;
    pwm_setting_t    pwm_setting;

    Hal_Pin_Info*    PIN_MAP = HAL_Pin_Map();
    uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;
    uint8_t          pwm_channel = PIN_MAP[pin].pwm_channel;

    pwmMap[pwm_num].pins[pwm_channel] = NRFX_PWM_PIN_NOT_USED;
    pwmMap[pwm_num].values[pwm_channel] = 0;

    if (pwmMap[pwm_num].enabled) {
        nrfx_pwm_uninit(&pwmMap[pwm_num].pwm);
        pwmMap[pwm_num].enabled = false;
    }

    // reset pin mode, don't call HAL_Set_Pin_Function or will enter a loop
    nrf_gpio_cfg_default(NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin));
    HAL_Set_Pin_Function(pin, PF_NONE);

    if ((pwmMap[pwm_num].pins[0] == NRFX_PWM_PIN_NOT_USED) &&
        (pwmMap[pwm_num].pins[1] == NRFX_PWM_PIN_NOT_USED) &&
        (pwmMap[pwm_num].pins[2] == NRFX_PWM_PIN_NOT_USED) &&
        (pwmMap[pwm_num].pins[3] == NRFX_PWM_PIN_NOT_USED)) {
        return SYSTEM_ERROR_NONE;
    }

    // reconfigure pwm
    for (int i = 0; i < PWM_CHANNEL_NUM; i++) {
        if (pwmMap[pwm_num].pins[i] != NRFX_PWM_PIN_NOT_USED) {
            if (getClockSetting(pwmMap[pwm_num].values[i], pwmMap[pwm_num].frequency, PIN_MAP[pwmMap[pwm_num].pins[i]].pwm_resolution, &pwm_setting) == false) {
                continue;
            }
            // In each value, the most significant bit (15) determines the polarity of the output
            ((uint16_t *)&(pwmMap[pwm_num].seq_value))[i] = pwm_setting.duty_hwu;
        }
    }

    nrfx_pwm_config_t const config = {
        .output_pins = {
            getNrfPin(pwmMap[pwm_num].pins[0]),  // channel 0
            getNrfPin(pwmMap[pwm_num].pins[1]),  // channel 1
            getNrfPin(pwmMap[pwm_num].pins[2]),  // channel 2
            getNrfPin(pwmMap[pwm_num].pins[3])   // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = pwm_setting.pwm_clock,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = pwm_setting.period_hwu,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    ret_code = nrfx_pwm_init(&pwmMap[pwm_num].pwm, &config, nullptr);
    if (ret_code) {
        return -1;
    }
    pwmMap[pwm_num].enabled = true;

    nrf_pwm_sequence_t const seq = {
        .values    = {.p_individual = (nrf_pwm_values_individual_t*) &pwmMap[pwm_num].seq_value},
        .length    = NRF_PWM_VALUES_LENGTH(pwmMap[pwm_num].seq_value),
        .repeats   = 0,
        .end_delay = 0
    };

    ret_code = nrfx_pwm_simple_playback(&pwmMap[pwm_num].pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
    if (ret_code) {
        return -2;
    }

    return SYSTEM_ERROR_NONE;
}

static int pwmPinInit(uint32_t pin, uint32_t value, uint32_t frequency) {
    ret_code_t      ret_code;
    pwm_setting_t   pwm_setting;

    Hal_Pin_Info*   PIN_MAP = HAL_Pin_Map();
    uint8_t         pwm_num = PIN_MAP[pin].pwm_instance;
    uint8_t         pwm_channel = PIN_MAP[pin].pwm_channel;

    // if frequency or pin is changed, pwm module should be reconfigured
    bool reconfig = (pwmMap[pwm_num].pins[pwm_channel] != pin) || (pwmMap[pwm_num].frequency != frequency);
    pwmMap[pwm_num].pins[pwm_channel] = pin;
    pwmMap[pwm_num].values[pwm_channel] = value;
    pwmMap[pwm_num].frequency = frequency;

    // GPIO output mode will cause glitches, configure GPIO to default mode
    nrf_gpio_cfg_default(NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin));

    // Get PWM parameters for each pin
    for (int i = 0; i < PWM_CHANNEL_NUM; i++) {
        if (pwmMap[pwm_num].pins[i] != NRFX_PWM_PIN_NOT_USED) {
            if (getClockSetting(pwmMap[pwm_num].values[i], frequency, PIN_MAP[pwmMap[pwm_num].pins[i]].pwm_resolution, &pwm_setting) == false) {
                continue;
            }
            // In each value, the most significant bit (15) determines the polarity of the output
            ((uint16_t *)&(pwmMap[pwm_num].seq_value))[i] = pwm_setting.duty_hwu;
        }
    }

    if (!pwmMap[pwm_num].enabled || reconfig) {
        if (pwmMap[pwm_num].enabled) {
            nrfx_pwm_uninit(&pwmMap[pwm_num].pwm);
            pwmMap[pwm_num].enabled = false;
        }

        nrfx_pwm_config_t const config = {
            .output_pins = {
                getNrfPin(pwmMap[pwm_num].pins[0]),  // channel 0
                getNrfPin(pwmMap[pwm_num].pins[1]),  // channel 1
                getNrfPin(pwmMap[pwm_num].pins[2]),  // channel 2
                getNrfPin(pwmMap[pwm_num].pins[3])   // channel 3
            },
            .irq_priority = APP_IRQ_PRIORITY_LOWEST,
            .base_clock   = pwm_setting.pwm_clock,
            .count_mode   = NRF_PWM_MODE_UP,
            .top_value    = pwm_setting.period_hwu,
            .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
            .step_mode    = NRF_PWM_STEP_AUTO
        };

        ret_code = nrfx_pwm_init(&pwmMap[pwm_num].pwm, &config, nullptr);
        if (ret_code) {
            return -1;
        }
        pwmMap[pwm_num].enabled = true;
    }

    nrf_pwm_sequence_t const seq = {
        .values = {.p_individual = (nrf_pwm_values_individual_t*) &pwmMap[pwm_num].seq_value},
        .length          = NRF_PWM_VALUES_LENGTH(pwmMap[pwm_num].seq_value),
        .repeats         = 0,
        .end_delay       = 0
    };

    ret_code = nrfx_pwm_simple_playback(&pwmMap[pwm_num].pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
    if (ret_code) {
        return -2;
    }
    HAL_Set_Pin_Function(pin, PF_PWM);

    return SYSTEM_ERROR_NONE;
}

void hal_pwm_reset_pin(uint16_t pin) {
    // Reset pwm resolution
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (pin >= TOTAL_PINS || PIN_MAP[pin].pwm_instance == PWM_INSTANCE_NONE) {
        return;
    }

    pwmPinUninit(pin);
    PIN_MAP[pin].pwm_resolution = DEFAULT_RESOLUTION_BITS;
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * default PWM frequent is set at 500 Hz
 */
void hal_pwm_write(uint16_t pin, uint8_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 */
void hal_pwm_write_with_frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency) {
    hal_pwm_write_with_frequency_ext(pin, value, pwm_frequency);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%.
 * DEFAULT_PWM_FREQ is set at 500 Hz
 */
void hal_pwm_write_ext(uint16_t pin, uint32_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

/*
 * @brief Should take an integer within the limits of set resolution (8-bit or 16-bit)
 * and create a PWM signal with a duty cycle from 0-100%
 * and a specified frequency.
 *
 * frequency range: 4Hz ~ 500KHz, frequency higher than 500KHz will adjust to 500KHz,
 *                 frequency lower than 4Hz will adjust to 4Hz
 */
void hal_pwm_write_with_frequency_ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency) {
    if ((pin >= TOTAL_PINS)            ||
        (pwm_frequency > MAX_PWM_FREQ) ||
        (pwm_frequency < MIN_PWM_FREQ) ||
        (value > (uint32_t)(1 << hal_pwm_get_resolution(pin)))) {
        return;
    }

    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (PIN_MAP[pin].pwm_instance == PWM_INSTANCE_NONE) {
        return;
    }

    if(pwmPinInit(pin, value, pwm_frequency)) {
        return;
    }
}

uint16_t hal_pwm_get_frequency(uint16_t pin) {
    return hal_pwm_get_frequency_ext(pin);
}

uint16_t hal_pwm_get_analog_value(uint16_t pin) {
    return hal_pwm_get_analog_value_ext(pin);
}

uint32_t hal_pwm_get_frequency_ext(uint16_t pin) {
    if (pin >= TOTAL_PINS) {
        return 0;
    }

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    uint8_t pwm_num = pin_info->pwm_instance;
    if (pwm_num == PWM_INSTANCE_NONE) {
        return 0;
    }

    return pwmMap[pwm_num].frequency;
}

uint32_t hal_pwm_get_analog_value_ext(uint16_t pin) {
    if (pin >= TOTAL_PINS) {
        return 0;
    }

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    uint8_t pwm_num = pin_info->pwm_instance;
    uint8_t pwm_channel = pin_info->pwm_channel;
    if (pwm_num == PWM_INSTANCE_NONE) {
        return 0;
    }

    if (!pwmMap[pwm_num].enabled) {
        return 0;
    }

    return pwmMap[pwm_num].values[pwm_channel];
}

uint32_t hal_pwm_get_max_frequency(uint16_t pin) {
    return MAX_PWM_FREQ;
}

void hal_pwm_update_duty_cycle(uint16_t pin, uint16_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

void hal_pwm_update_duty_cycle_ext(uint16_t pin, uint32_t value) {
    hal_pwm_write_with_frequency_ext(pin, value, DEFAULT_PWM_FREQ);
}

uint8_t hal_pwm_get_resolution(uint16_t pin) {
    if (pin >= TOTAL_PINS) {
        return 0;
    }

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    return pin_info->pwm_resolution;
}

void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution) {
    if (pin >= TOTAL_PINS || resolution > MAX_RESOLUTION_BITS || resolution < 1) {
        return;
    }

    Hal_Pin_Info* pin_info = HAL_Pin_Map() + pin;
    pin_info->pwm_resolution = resolution;
}
