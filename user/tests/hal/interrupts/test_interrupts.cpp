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
    hal_gpio_mode(D7, OUTPUT);
    hal_gpio_write(D7, on);
    on = !on;
}

/* executes once at startup */
void setup() {
    Log.info("Test Start...");

    hal_interrupt_extra_configuration_t config = {0};
    config.keepHandler = false;

    int data = 123;
    
    for (int i = 0; i < 19; i++)
    {
        hal_gpio_mode(i, INPUT_PULLUP);
        hal_interrupt_attach(i, common_interrupt_handler, (void *)data, CHANGE, &config);    
    }

    hal_interrupt_detach(2);    
    hal_interrupt_detach(7);    

    hal_gpio_mode(D2, INPUT_PULLUP);
    hal_interrupt_attach(D2, attach_interrupt_handler, (void *)data, CHANGE, &config);    
}

/* executes continuously after setup() runs */
void loop() {

}
