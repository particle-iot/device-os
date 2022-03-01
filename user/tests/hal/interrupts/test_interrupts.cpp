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
#include "interrupts_hal.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
// Serial1LogHandler logHandler(115200);

static void common_interrupt_handler(void *data)
{
    
}

static void attach_interrupt_handler(void *data)
{
    static bool on = true;
    HAL_Pin_Mode(D7, OUTPUT);
    HAL_GPIO_Write(D7, on);
    on = !on;
}

/* executes once at startup */
void setup() {
    Log.info("Test Start...");

    HAL_InterruptExtraConfiguration config = {0};
    config.keepHandler = false;

    int data = 123;
    
    for (int i = 0; i < 19; i++)
    {
        HAL_Pin_Mode(i, INPUT_PULLUP);
        HAL_Interrupts_Attach(i, common_interrupt_handler, (void *)data, CHANGE, &config);    
    }

    HAL_Interrupts_Detach(2);    
    HAL_Interrupts_Detach(7);    

    HAL_Pin_Mode(D2, INPUT_PULLUP);
    HAL_Interrupts_Attach(D2, attach_interrupt_handler, (void *)data, CHANGE, &config);    
}

/* executes continuously after setup() runs */
void loop() {

}
