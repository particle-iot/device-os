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

SerialLogHandler dbg(LOG_LEVEL_ALL);
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

/* executes once at startup */
void setup() {
    waitUntil(Serial.isConnected);
    Log.info("Free mem: %u", System.freeMemory());
    network_on(NETWORK_INTERFACE_PPP_SERVER, 0, 0, nullptr);
    network_connect(NETWORK_INTERFACE_PPP_SERVER, 0, 0, nullptr);
    Particle.connect();
    //Particle.connect();
}

/* executes continuously after setup() runs */
void loop() {

}
