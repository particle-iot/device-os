/**
 ******************************************************************************
 * @file    system.cpp
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

test(system_api) {

    API_COMPILE(System.dfu());
    API_COMPILE(System.dfu(true));

    API_COMPILE(System.factoryReset());

    API_COMPILE(System.enterSafeMode());

    API_COMPILE(System.reset());

    API_COMPILE(System.sleep(60));

    API_COMPILE(System.sleep(SLEEP_MODE_WLAN, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP));

    API_COMPILE(System.sleep(A0, CHANGE));
    API_COMPILE(System.sleep(A0, RISING));
    API_COMPILE(System.sleep(A0, FALLING));
    API_COMPILE(System.sleep(A0, FALLING, 20));

    API_COMPILE(System.mode());
    API_COMPILE(SystemClass(AUTOMATIC));
    API_COMPILE(SystemClass(SEMI_AUTOMATIC));
    API_COMPILE(SystemClass(MANUAL));
}

test(system_mode) {
    // braces are required since the macro evaluates to a declaration
    API_COMPILE({ SYSTEM_MODE(AUTOMATIC) });
    API_COMPILE({ SYSTEM_MODE(SEMI_AUTOMATIC) });
    API_COMPILE({ SYSTEM_MODE(MANUAL) });
}

test(system_thread_setting) {
#if PLATFORM_THREADING
    API_COMPILE({SYSTEM_THREAD(ENABLED)});
#endif
    API_COMPILE({SYSTEM_THREAD(DISABLED)});
}

test(system_version) {

    API_COMPILE(Serial.println(stringify(SYSTEM_VERSION_STRING)));
    API_COMPILE(Serial.println(SYSTEM_VERSION));
}


test(system_freememory) {
    uint32_t f;
    API_COMPILE(f=System.freeMemory());
    (void)f;
}

test(system_waitfor) {
    API_COMPILE(System.waitCondition([]{return WiFi.ready();}));

    API_COMPILE(waitFor(WiFi.ready, 10000));
    API_COMPILE(waitUntil(WiFi.ready));
}


test(system_config_set) {

    API_COMPILE(System.set(SYSTEM_CONFIG_DEVICE_KEY, NULL, 123));
    API_COMPILE(System.set(SYSTEM_CONFIG_SOFTAP_PREFIX, "hello"));
    API_COMPILE(System.set(SYSTEM_CONFIG_SOFTAP_SUFFIX, "hello"));
}

/*
test(system_config_get) {

    uint8_t buf[123];
    API_COMPILE(System.get(CONFIG_DEVICE_KEY, buf, 123));
    API_COMPILE(System.get(CONFIG_SSID_PREFIX, buf, 123));
}
*/