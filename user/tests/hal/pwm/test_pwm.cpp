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
    HAL_PWM_Write(RGBR,  0);
    HAL_PWM_Write(RGBG,  255);
    HAL_PWM_Write(RGBB,  255);
}

static void rgb_green()
{
    HAL_PWM_Write(RGBR,  255);
    HAL_PWM_Write(RGBG,  0);
    HAL_PWM_Write(RGBB,  255);
}

static void rgb_blue()
{
    HAL_PWM_Write(RGBR,  255);
    HAL_PWM_Write(RGBG,  255);
    HAL_PWM_Write(RGBB,  0);
}

/* executes once at startup */
void setup() {
    // PWM Group 

	pinMode(D4, OUTPUT);
	analogWrite(D4, 255);

	HAL_PWM_Set_Resolution(D6, 15);
    HAL_PWM_Write_With_Frequency_Ext(D6, 0x3000, 10*1000);
	HAL_PWM_Set_Resolution(D7, 15);
    HAL_PWM_Write_With_Frequency_Ext(D7, 0x5000, 10*1000);
	HAL_PWM_Set_Resolution(D8, 15);
    HAL_PWM_Write_With_Frequency_Ext(D8, 0x7000, 10*1000);

    // PWM Group, other channels are RGB
	HAL_PWM_Set_Resolution(D5, 15);
    HAL_PWM_Write_With_Frequency_Ext(D5,  0x2000, 500);
 
    // PWM Group 
	HAL_PWM_Set_Resolution(A0, 15);
    HAL_PWM_Write_With_Frequency_Ext(A0, 0x3000, 80*1000);
	HAL_PWM_Set_Resolution(A1, 15);
    HAL_PWM_Write_With_Frequency_Ext(A1, 0x3000, 80*1000);
	HAL_PWM_Set_Resolution(A2, 15);
    HAL_PWM_Write_With_Frequency_Ext(A2, 0x3000, 80*1000);
	HAL_PWM_Set_Resolution(A3, 15);
    HAL_PWM_Write_With_Frequency_Ext(A3, 0x3000, 80*1000);
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
