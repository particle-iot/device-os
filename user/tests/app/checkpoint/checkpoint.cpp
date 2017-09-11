/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "application.h"

SYSTEM_THREAD(ENABLED);

static os_mutex_t s_mutex = {0};

void hardfault_event_handler(const char* event, const char* data) {
    HAL_USART_Write_NineBitData((HAL_USART_Serial)10, 0x0000);
}

void panic_event_handler(const char* event, const char* data) {
    // Got into panic
    PANIC(UsageFault, "UsageFault");
}

void deadlock_event_handler(const char* event, const char* data) {
    // Lock an already locked mutex
    os_mutex_lock(s_mutex);
}

/* executes once at startup */
void setup() {
    os_mutex_create(&s_mutex);
    os_mutex_lock(s_mutex);

    pinMode(D7, OUTPUT);

    Particle.subscribe("hardfault", hardfault_event_handler, MY_DEVICES);
    Particle.subscribe("panic", panic_event_handler, MY_DEVICES);
    Particle.subscribe("deadlock", deadlock_event_handler, MY_DEVICES);
}

void somefunc();

/* executes continuously after setup() runs */
void loop() {
    delay(1000);
    CHECKPOINT();
    digitalWrite(D7, !digitalRead(D7));
    delay(1000);
    somefunc();
    digitalWrite(D7, !digitalRead(D7));
}








































































void somefunc() {
    CHECKPOINT();
}
