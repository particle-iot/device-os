/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#include "gnss_hal.h"
#include "rtc_hal.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

Serial1LogHandler l(115200, LOG_LEVEL_ALL);

uint32_t counter = 0;

void gnssThread() {
    while (true) {
        hal_gnss_pos(nullptr);
        delay(500);
    }
}

/* executes once at startup */
void setup() {
    Log.info("Application started");

    Cellular.on();
    waitUntil(Cellular.isOn);

    // Cellular.command(1000, "AT+QGPSCFG=\"priority\",1,0\r\n");
    // delay(3s);
    // Cellular.command(1000, "AT+QGPSCFG=\"priority\"\r\n");
    // delay(3s);

    hal_gnss_init(nullptr);

    Cellular.off();
    delay(10s);

    Cellular.on();
    waitUntil(Cellular.isOn);

    delay(10s);

    Particle.connect();
    waitUntil(Particle.connected);

    static Thread *thread = new Thread("gnss", gnssThread, OS_THREAD_PRIORITY_DEFAULT);
}

/* executes continuously after setup() runs */
void loop() {
    if (Particle.connected()) {
        counter++;
        char temp[32];
        sprintf(temp, "%ld", counter);
        Particle.publish("GH-GNSS", temp);
        Log.info("counter: %ld", counter);
        delay(1000);
    }
}
