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

#include "Particle.h"

SerialLogHandler dbg(LOG_LEVEL_ALL);

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void network_change(system_event_t event, int param)
{
  switch (param) {
    case network_status_powering_on:
      Log.info("Powering on");
      break;

    case network_status_on:
      Log.info("Powered on");
      break;

    case network_status_powering_off:
      Log.info("Powering off");
      break;

    case network_status_off:
      Log.info("Powered off");
      break;

    case network_status_connecting:
      Log.info("Connecting");
      break;

    case network_status_connected:
      Log.info("Connected");
      break;

    case network_status_disconnecting:
      Log.info("Disconnecting");
      break;

    case network_status_disconnected:
      Log.info("Disconnected");
      break;
  }

  Serial.flush();
}

/* executes once at startup */
void setup() {
    // waitUntil(Serial.isConnected);
    //Log.info("Free mem: %u", System.freeMemory());
    network_on(NETWORK_INTERFACE_PPP_SERVER, 0, 0, nullptr);
    network_connect(NETWORK_INTERFACE_PPP_SERVER, 0, 0, nullptr);
    System.on(network_status, network_change);
    Particle.connect();
}

/* executes continuously after setup() runs */
unsigned int rssiTick = 0;

void loop() {
    if ((System.uptime() - rssiTick) >= 10 /*seconds*/) {
        rssiTick = System.uptime();

        if (Cellular.ready()) {
            auto rssi = Cellular.RSSI();

            Log.info("Cellular RSSI = %0.1f dBm, %0.1f %%", rssi.getStrengthValue(), rssi.getStrength());
        }
    }
}
