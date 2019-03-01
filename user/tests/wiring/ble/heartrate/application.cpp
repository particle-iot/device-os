/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "application.h"

BLEAttribute heartrate(READABLE | NOTIFIABLE_WO_RSP, "heartrate");
BLEConnectionInstance myConn;

void setup() {
    BLE.on();

    BLE.advertise();

    BLEDevice local;
    myConn = BLE.connect();
}

void loop() {
    uint32_t newHr = 1234;
    if (BLE.connected(myConn)) {
        heartrate.update((const uint8_t*)&newHr, sizeof(uint32_t));
    }
}
