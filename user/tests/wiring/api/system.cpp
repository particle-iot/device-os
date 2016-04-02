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
    API_COMPILE(System.resetReason());

    API_COMPILE(System.sleep(60));

}

test(system_sleep)
{
    API_COMPILE(System.sleep(60));

    API_COMPILE(System.sleep(SLEEP_MODE_WLAN, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP));

    API_COMPILE(System.sleep(A0, CHANGE));
    API_COMPILE(System.sleep(A0, RISING));
    API_COMPILE(System.sleep(A0, FALLING));
    API_COMPILE(System.sleep(A0, FALLING, 20));

    // with network flags
    API_COMPILE(System.sleep(SLEEP_MODE_WLAN, 60, SLEEP_NETWORK_STANDBY));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, 60, SLEEP_NETWORK_STANDBY));
    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY));

    API_COMPILE(System.sleep(A0, CHANGE, SLEEP_NETWORK_STANDBY));
    API_COMPILE(System.sleep(A0, RISING, SLEEP_NETWORK_STANDBY));
    API_COMPILE(System.sleep(A0, FALLING, SLEEP_NETWORK_STANDBY));
    API_COMPILE(System.sleep(A0, FALLING, 20, SLEEP_NETWORK_STANDBY));
    API_COMPILE(System.sleep(A0, FALLING, SLEEP_NETWORK_STANDBY, 20));


}

test(system_mode) {
    API_COMPILE(System.mode());
    API_COMPILE(SystemClass(AUTOMATIC));
    API_COMPILE(SystemClass(SEMI_AUTOMATIC));
    API_COMPILE(SystemClass(MANUAL));

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
#if Wiring_WiFi == 1
    API_COMPILE(System.waitCondition([]{return WiFi.ready();}));
    API_COMPILE(waitFor(WiFi.ready, 10000));
    API_COMPILE(waitUntil(WiFi.ready));
#endif
}


test(system_config_set) {

    API_COMPILE(System.set(SYSTEM_CONFIG_DEVICE_KEY, NULL, 123));
#if PLATFORM == photon
    API_COMPILE(System.set(SYSTEM_CONFIG_SOFTAP_PREFIX, "hello"));
    API_COMPILE(System.set(SYSTEM_CONFIG_SOFTAP_SUFFIX, "hello"));
#endif
}

/*
test(system_config_get) {

    uint8_t buf[123];
    API_COMPILE(System.get(CONFIG_DEVICE_KEY, buf, 123));
    API_COMPILE(System.get(CONFIG_SSID_PREFIX, buf, 123));
}
*/

void handler()
{
}

void handler_event(system_event_t event)
{
}

void handler_event_data(system_event_t event, int data)
{
}

void handler_event_data_param(system_event_t event, int data, void* param)
{
}

test(system_events)
{
    int clicks = system_button_clicks(123);
    system_event_t my_events = wifi_listen_begin + wifi_listen_end + wifi_listen_update +
                               setup_begin + setup_end + setup_update + network_credentials +
                               network_status + button_status + button_click + button_final_click +
                               reset + reset_pending + firmware_update + firmware_update_pending +
                               all_events;

    API_COMPILE(System.on(my_events, handler));
    API_COMPILE(System.on(my_events, handler_event));
    API_COMPILE(System.on(my_events, handler_event_data));
    API_COMPILE(System.on(my_events, handler_event_data_param));
    (void)clicks; // avoid unused variable warning
}
