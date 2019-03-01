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

BLEAttributePtr heartrate;
BLEAdvertiser advertisers[10];
BLEConnectionInstance myConn;

void setup1() {
    BLE.on();
}

void loop1() {
    if (BLE.connected(myConn)) {
        uint8_t newHr[4];
        uint16_t len = 4;
        heartrate->value(newHr, &len);
    }
    else {
        uint8_t count = BLE.scan(advertisers, 10);

        if (count > 0) {
            for (uint8_t i = 0; i < count; i++) {
                bool found = advertisers[i].data().find(BLE_SIG_AD_TYPE_FLAGS);
                if (found) {
                    BLEPeerAttrListPtr attrList;

                    myConn = BLE.connect(advertisers[i].device(), &attrList);
                    if (myConn.valid()) {
                        heartrate = attrList->find("heartrate");
                    }
                }
            }
        }
    }
}
