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

#include "logging.h"
#include "tone_hal.h"
#include "pwm_hal.h"
#include "pinmap_impl.h"
#include "concurrent_hal.h"

#define MAX_PWM_NUM                  12

struct {
    os_timer_t  timer;
    uint8_t     pin;
} g_tone[MAX_PWM_NUM] = {nullptr};

static void stop_tone_timer(os_timer_t timer) {
    for (int i = 0; i < MAX_PWM_NUM; i++) {
        if (g_tone[i].timer == timer) {
            os_timer_destroy(g_tone[i].timer, nullptr);
            g_tone[i].timer = nullptr;
            HAL_PWM_Reset_Pin(g_tone[i].pin);
            break;
        }
    }
}

static void tone_timer_timeout_handler(os_timer_t timer) {
    stop_tone_timer(timer);
}

void HAL_Tone_Start(uint8_t pin, uint32_t frequency, uint32_t duration) {
    int ret;

    // no tone for frequency outside of human audible range
    if(frequency < 20 || frequency > 20000) {
        return;
    }

    NRF5x_Pin_Info * pinmap = HAL_Pin_Map();
    if (pin >= TOTAL_PINS || pinmap[pin].pwm_instance == PWM_INSTANCE_NONE) {
        return;
    }

    int timer_index = -1;
    for (int i = 0; i < MAX_PWM_NUM; i++) {
        if (g_tone[i].timer == nullptr) {
            timer_index = i;
            break;
        }
    }

    if (timer_index == -1) {
        return;
    }

    g_tone[timer_index].pin = pin;
    ret = os_timer_create(&g_tone[timer_index].timer, duration, tone_timer_timeout_handler, nullptr, true, nullptr);
    if (ret != 0) {
        return;
    }

    ret = os_timer_change(g_tone[timer_index].timer, OS_TIMER_CHANGE_START, false, 0, 0xffffffff, nullptr);
    if (ret != 0) {
        stop_tone_timer(g_tone[timer_index].timer);
        return;
    }

    HAL_PWM_Set_Resolution(pin, 8);
    HAL_PWM_Write_With_Frequency_Ext(pin, 127, frequency);
}

void HAL_Tone_Stop(uint8_t pin) {
    for (int i = 0; i < MAX_PWM_NUM; i++) {
        if (g_tone[i].timer && g_tone[i].pin == pin) {
            stop_tone_timer(g_tone[i].timer);
            break;
        }
    }
}

uint32_t HAL_Tone_Get_Frequency(uint8_t pin) {
    for (int i = 0; i < MAX_PWM_NUM; i++) {
        if (g_tone[i].timer && g_tone[i].pin == pin) {
            return HAL_PWM_Get_Frequency_Ext(pin);
        }
    }
    return 0;
}

bool HAL_Tone_Is_Stopped(uint8_t pin) {
    for (int i = 0; i < MAX_PWM_NUM; i++) {
        if (g_tone[i].timer && g_tone[i].pin == pin) {
            return false;
        }
    }
    return true;
}
