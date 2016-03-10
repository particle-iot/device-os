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

test(api_cellular_ip) {
    IPAddress address;
    API_COMPILE(address=Cellular.localIP());
}

test(api_cellular_set_credentials) {
    API_COMPILE(Cellular.setCredentials("apn"));
    API_COMPILE(Cellular.setCredentials("username","password"));
}

test (api_cellular_connect_disconnect) {
    API_COMPILE(Cellular.connect());
    API_COMPILE(Cellular.disconnect());
}

test (api_cellular_on_off) {
    API_COMPILE(Cellular.on());
    API_COMPILE(Cellular.off());
}

test (api_cellular_listen) {
    bool result;
    API_COMPILE(Cellular.listen());
    API_COMPILE(Cellular.listen(false));
    API_COMPILE(result=Cellular.listening());
    (void)result; // avoid unused variable warning
}

test (api_cellular_ready) {
    bool result;
    API_COMPILE(result=Cellular.ready());
    (void)result; // avoid unused variable warning
}

#endif