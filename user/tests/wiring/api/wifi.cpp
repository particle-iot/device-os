/**
 ******************************************************************************
 * @file    cloud.cpp
 * @authors Matthew McGowan
 * @date    13 January 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "testapi.h"

test(api_wifi_resolve) {

    API_COMPILE(WiFi.resolve(String("abc.def.com")));
    API_COMPILE(WiFi.resolve("abc.def.com"));
}

test (api_wifi_connect) {

    bool UNUSED(result);
    API_COMPILE(WiFi.connect());
    API_COMPILE(WiFi.connect(WIFI_CONNECT_SKIP_LISTEN));
    API_COMPILE(WiFi.connecting());
}

test (wifi_api_listen) {
    bool UNUSED(result);
    API_COMPILE(WiFi.listen());
    API_COMPILE(WiFi.listen(false));
    API_COMPILE(WiFi.listening());
}

#if PLATFORM_ID>=4
test(api_wifi_selectantenna) {

    API_COMPILE(WiFi.selectAntenna(ANT_AUTO));
    API_COMPILE(WiFi.selectAntenna(ANT_INTERNAL));
    API_COMPILE(WiFi.selectAntenna(ANT_EXTERNAL));

}
#endif


test(api_wifi_set_credentials) {

    API_COMPILE(WiFi.setCredentials("ssid)",4,"password", 8, WPA2));

    API_COMPILE(WiFi.setCredentials("ssid)",4,"password", 8, WPA2, WLAN_CIPHER_AES));

}