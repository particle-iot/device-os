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

#if Wiring_Cellular

test(api_cellular_rssi)
{
    CellularSignal sig;
    API_COMPILE(Cellular.RSSI());
    API_COMPILE(sig = Cellular.RSSI());
    API_COMPILE(Serial.println(sig));
}

test(api_cellular_ip)
{
    IPAddress address;
    API_COMPILE(address = Cellular.localIP());
}

test(api_cellular_set_credentials)
{
    API_COMPILE(Cellular.setCredentials("apn"));
    API_COMPILE(Cellular.setCredentials("username","password"));
}

test (api_cellular_connect_disconnect)
{
    API_COMPILE(Cellular.connect());
    API_COMPILE(Cellular.disconnect());
}

test (api_cellular_on_off)
{
    API_COMPILE(Cellular.on());
    API_COMPILE(Cellular.off());
}

test (api_cellular_listen)
{
    bool result;
    uint16_t val;
    API_COMPILE(Cellular.listen());
    API_COMPILE(Cellular.listen(false));
    API_COMPILE(result = Cellular.listening());
    API_COMPILE(Cellular.setListenTimeout(10));
    API_COMPILE(val = Cellular.getListenTimeout());
    (void)result; // avoid unused warning
    (void)val;    //   |
}

test (api_cellular_ready)
{
    bool result;
    API_COMPILE(result = Cellular.ready());
    (void)result; // avoid unused variable warning
}

test(api_cellular_data_usage)
{
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

test(api_cellular_band_select) {
    CellularBand band1;
    CellularBand band2;
    band2.ok = true;
    bool result;

    API_COMPILE(result = Cellular.getBandSelect(band1));

    API_COMPILE(result = Cellular.setBandSelect("1900"));

    API_COMPILE(result = Cellular.getBandAvailable(band1));

    API_COMPILE(!Cellular.getBandSelect(band1));

    API_COMPILE(!!Cellular.getBandSelect(band1));

    API_COMPILE(result = band1);

    API_COMPILE(result = band2);

    API_COMPILE(Serial.println(band1));

    // These should not compile, test these manually by changing to API_COMPILE()
    API_NO_COMPILE(band1 == band2);
    API_NO_COMPILE(band1 != band2);

    (void)result; // avoid unused warning
}

test(api_cellular_resolve) {
    IPAddress ip;
    API_COMPILE(ip = Cellular.resolve("www.google.com"));
    (void)ip;
}

test(api_cellular_lock_unlock) {
    API_COMPILE(Cellular.lock());
    API_COMPILE(Cellular.unlock());
}

#endif
