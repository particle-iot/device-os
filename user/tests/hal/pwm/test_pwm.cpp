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

 #include "application.h"
 #include "adc_hal.h"
 #include "gpio_hal.h"

SYSTEM_MODE(MANUAL);

static void rgb_red()
{
    hal_pwm_write(RGBR,  0);
    hal_pwm_write(RGBG,  255);
    hal_pwm_write(RGBB,  255);
}

static void rgb_green()
{
    hal_pwm_write(RGBR,  255);
    hal_pwm_write(RGBG,  0);
    hal_pwm_write(RGBB,  255);
}

static void rgb_blue()
{
    hal_pwm_write(RGBR,  255);
    hal_pwm_write(RGBG,  255);
    hal_pwm_write(RGBB,  0);
}

/* executes once at startup */
void setup() {
    // PWM Group 

	pinMode(D4, OUTPUT);
	analogWrite(D4, 255);

	hal_pwm_set_resolution(D6, 15);
    hal_pwm_write_with_frequency_ext(D6, 0x3000, 10*1000);
	hal_pwm_set_resolution(D7, 15);
    hal_pwm_write_with_frequency_ext(D7, 0x5000, 10*1000);
	hal_pwm_set_resolution(D8, 15);
    hal_pwm_write_with_frequency_ext(D8, 0x7000, 10*1000);

    // PWM Group, other channels are RGB
	hal_pwm_set_resolution(D5, 15);
    hal_pwm_write_with_frequency_ext(D5,  0x2000, 500);
 
    // PWM Group 
	hal_pwm_set_resolution(A0, 15);
    hal_pwm_write_with_frequency_ext(A0, 0x3000, 80*1000);
	hal_pwm_set_resolution(A1, 15);
    hal_pwm_write_with_frequency_ext(A1, 0x3000, 80*1000);
	hal_pwm_set_resolution(A2, 15);
    hal_pwm_write_with_frequency_ext(A2, 0x3000, 80*1000);
	hal_pwm_set_resolution(A3, 15);
    hal_pwm_write_with_frequency_ext(A3, 0x3000, 80*1000);
}

/* executes continuously after setup() runs */
void loop() {
    delay(1000);
    rgb_red();
    delay(1000);
    rgb_green();
    delay(1000);
    rgb_blue();
}
