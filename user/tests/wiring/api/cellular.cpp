/**
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

#include "testapi.h"

#if Wiring_Cellular == 1

test(api_cellular_rssi) {
    CellularSignal sig;

    API_COMPILE(Cellular.RSSI());

    API_COMPILE(sig = Cellular.RSSI());

    API_COMPILE(Serial.println(sig));
}

test(api_cellular_data_usage) {
    CellularData data1;
    CellularData data2;
    data2.ok = true;
    bool result;

    API_COMPILE(result = Cellular.getDataUsage(data1));

    API_COMPILE(result = Cellular.setDataUsage(data1));

    API_COMPILE(result = Cellular.resetDataUsage());

    API_COMPILE(!Cellular.getDataUsage(data1));

    API_COMPILE(!!Cellular.getDataUsage(data1));

    API_COMPILE(result = data1);

    API_COMPILE(result = data2);

    API_COMPILE(Serial.println(data1));

    // These should not compile, test these manually by changing to API_COMPILE()
    API_NO_COMPILE(data1 == data2);
    API_NO_COMPILE(data1 != data2);

    (void)result; // avoid unused warning
}

#endif