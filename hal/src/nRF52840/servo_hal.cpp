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

#include "servo_hal.h"
#include "pwm_hal.h"
#include "pinmap_impl.h"

#define SERVO_PWM_RESOLUTION        15
#define SERVO_PWM_MAX_VALUE         ((1 << SERVO_PWM_RESOLUTION) - 1)

void HAL_Servo_Attach(uint16_t pin) {
    // Set PWM frequency to 50Hz
    hal_pwm_write_with_frequency(pin, 0, SERVO_TIM_PWM_FREQ);
}

void HAL_Servo_Detach(uint16_t pin) {
    hal_pwm_reset_pin(pin);
}

void HAL_Servo_Write_Pulse_Width(uint16_t pin, uint16_t pulseWidth) {
    hal_pwm_set_resolution(pin, SERVO_PWM_RESOLUTION);
    hal_pin_info_t *pin_map = hal_pin_map();
    pin_map[pin].user_data = pulseWidth;

    uint32_t period_us = 1000000 / SERVO_TIM_PWM_FREQ;
    uint32_t pwm_duty_value = pulseWidth * SERVO_PWM_MAX_VALUE / period_us;
    hal_pwm_write_with_frequency_ext(pin, pwm_duty_value, SERVO_TIM_PWM_FREQ);
}

uint16_t HAL_Servo_Read_Pulse_Width(uint16_t pin) {
    hal_pin_info_t* pin_map = hal_pin_map();
    return pin_map[pin].user_data;
}

uint16_t HAL_Servo_Read_Frequency(uint16_t pin) {
    return hal_pwm_get_frequency(pin);
}
