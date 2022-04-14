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
#include "gpio_hal.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
Serial1LogHandler logHandler(115200);

void test_gpio_blink(void)
{
    HAL_Pin_Mode(D7, OUTPUT);
    delay(500);
    HAL_GPIO_Write(D7, 1);
    delay(500);
    HAL_GPIO_Write(D7, 0);
}

void test_pulse_width(void)
{
    uint32_t pulse_width = HAL_Pulse_In(D2, 0);
    if (pulse_width)
    {
        Log.info("Pulse Width: %dms", pulse_width);
    }
}

void test_gpio_input(void)
{
    HAL_Pin_Mode(D2, INPUT_PULLUP);
    HAL_Pin_Mode(D7, OUTPUT);

    while (true)
    {
        if (HAL_GPIO_Read(D2))
        {
            HAL_GPIO_Write(D7, 1);
        }
        else
        {
            HAL_GPIO_Write(D7, 0);
        }
    }
}
/* executes once at startup */
void setup() {
    Log.info("GPIO Test Start...");
    
}

/* executes continuously after setup() runs */
void loop() {
    // test_gpio_blink();
    // test_pulse_width();
    test_gpio_input();
}
