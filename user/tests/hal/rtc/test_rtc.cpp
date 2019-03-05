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
#include "rtc_hal.h"
#include "spark_wiring_time.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
Serial1LogHandler logHandler(115200);

/* executes once at startup */
void setup() {
    Log.info("default time: %s", (const char*)Time.timeStr());
    Time.setTime(1532086230);
    Log.info("new time: %s", (const char*)Time.timeStr());
}

/* executes continuously after setup() runs */
void loop() {
    delay(1000);
    Log.info("curr time: %s", (const char*)Time.timeStr());
}
