/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, 
{
    //Setup the Tinker application here
    { "app", LOG_LEVEL_ALL }, 
    //{ "sys.power", LOG_LEVEL_TRACE },
    { "comm.protocol", LOG_LEVEL_TRACE },
    { "system", LOG_LEVEL_TRACE },
    { "network", LOG_LEVEL_TRACE },
    { "ncp.at", LOG_LEVEL_TRACE },
    { "system.nm", LOG_LEVEL_TRACE },
    { "system.cm", LOG_LEVEL_TRACE },
    { "system.nd", LOG_LEVEL_TRACE },
    { "ncp.client", LOG_LEVEL_TRACE },
    { "net.rltkncp", LOG_LEVEL_TRACE },
    { "net.ifapi", LOG_LEVEL_TRACE },
    { "net.ppp.client", LOG_LEVEL_WARN },
    { "net.en", LOG_LEVEL_TRACE },
    { "service.ntp", LOG_LEVEL_TRACE },
    { "net.en", LOG_LEVEL_TRACE },
    { "mux", LOG_LEVEL_ERROR },
    { "net.ppp.ipcp", LOG_LEVEL_ERROR },
    { "comm.cloud.posix", LOG_LEVEL_TRACE },
}   
);
SYSTEM_MODE(SEMI_AUTOMATIC);

/* executes once at startup */
void setup() {
    // waitUntil(Serial.isConnected);
    // Enable Cellular
    Cellular.on();
    Cellular.connect();
    // Bind Tether interface to USB Serial with default settings (8n1 + RTS/CTS flow control)
    Tether.bind(TetherSerialConfig().baudrate(921600).usbserial());
    // Turn on Tether interface and bring it up
    Tether.on();
    Tether.connect();
    Particle.connect();
}

void loop() {

}
