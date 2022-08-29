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
    API_COMPILE(System.dfu(RESET_NO_WAIT | RESET_PERSIST_DFU));

    API_COMPILE(System.factoryReset());
    API_COMPILE(System.factoryReset(RESET_NO_WAIT));

    API_COMPILE(System.enterSafeMode());
    API_COMPILE(System.enterSafeMode(RESET_NO_WAIT));

    API_COMPILE(System.reset());
    API_COMPILE(System.reset(RESET_NO_WAIT));
    API_COMPILE(System.reset(0)); // User data
    API_COMPILE(System.reset(0, RESET_NO_WAIT));
    API_COMPILE(System.resetReason());
    API_COMPILE(System.resetReasonData());

    API_COMPILE(System.sleep(60));
    API_COMPILE(System.sleep(60s));

    API_COMPILE(System.sleep(SystemSleepConfiguration()));
}

test(system_sleep)
{
    const hal_pin_t pins_array[] = {D0, D1};
    const InterruptMode mode_array[] = {RISING, FALLING};

    // All sleep methods should return System.sleep()
    API_COMPILE({ SleepResult r = System.sleep(60); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(60s); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_WLAN, 60); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_WLAN, 60s); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60s); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(A0, CHANGE); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, RISING); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, 20); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, 20s); (void)r; });

    // with network flags
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_WLAN, 60, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_WLAN, 60s, SLEEP_NETWORK_STANDBY); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60s, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY, 60); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY, 60s); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_NETWORK_STANDBY | SLEEP_NO_WAIT); (void)r; });

    API_COMPILE({ SleepResult r = System.sleep(A0, CHANGE, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, RISING, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, 20, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, 20s, SLEEP_NETWORK_STANDBY); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, SLEEP_NETWORK_STANDBY, 20); (void)r; });
    API_COMPILE({ SleepResult r = System.sleep(A0, FALLING, SLEEP_NETWORK_STANDBY, 20s); (void)r; });

    // Multi-pin variants
    {
        /*
         * wakeup pins: std::initializer_list<hal_pin_t>
         * trigger mode: single InterruptMode
         */
        API_COMPILE({ SleepResult r = System.sleep({D0, D1}, RISING); (void)r; });

        /*
         * wakeup pins: std::initializer_list<hal_pin_t>
         * trigger mode: std::initializer_list<InterruptMode>
         */
        API_COMPILE({ SleepResult r = System.sleep({D0, D1}, {RISING, FALLING}); (void)r; });

        /*
         * wakeup pins: hal_pin_t* + size_t
         * trigger mode: single InterruptMode
         */
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, 20); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, 20s); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, 20, SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, 20s, SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, SLEEP_NETWORK_STANDBY, 20); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING, SLEEP_NETWORK_STANDBY, 20s); (void)r; });

        /*
         * wakeup pins: hal_pin_t* + size_t
         * trigger mode: InterruptMode* + size_t
         */
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array)); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), 20); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), 20s); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), 20, SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), 20s, SLEEP_NETWORK_STANDBY); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), SLEEP_NETWORK_STANDBY, 20); (void)r; });
        API_COMPILE({ SleepResult r = System.sleep(pins_array, sizeof(pins_array)/sizeof(*pins_array), mode_array, sizeof(mode_array)/sizeof(*mode_array), SLEEP_NETWORK_STANDBY, 20s); (void)r; });
		// SLEEP_DISABLE_WKP_PIN
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN, 60); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN, 60s); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60, SLEEP_DISABLE_WKP_PIN); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60s, SLEEP_DISABLE_WKP_PIN); (void)r;});

		// Flags OR-ing
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN | SLEEP_NETWORK_STANDBY); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN | SLEEP_NETWORK_STANDBY, 60); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN | SLEEP_NETWORK_STANDBY, 60s); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60, SLEEP_DISABLE_WKP_PIN | SLEEP_NETWORK_STANDBY); (void)r;});
		API_COMPILE({ SleepResult r = System.sleep(SLEEP_MODE_DEEP, 60s, SLEEP_DISABLE_WKP_PIN | SLEEP_NETWORK_STANDBY); (void)r;});
    }
    API_COMPILE({
        SleepResult r;
        (void)r.reason();
        (void)r.wokenUpByRtc();
        (void)r.wokenUpByPin();

        (void)r.pin();
        (void)r.rtc();
        (void)r.error();
    });

    API_COMPILE(System.wakeUpReason());
    API_COMPILE(System.wokenUpByPin());
    API_COMPILE(System.wokenUpByRtc());
    API_COMPILE(System.wakeUpPin());
    API_COMPILE(System.sleepResult());
    API_COMPILE(System.sleepError());

    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::ULTRA_LOW_POWER)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::HIBERNATE)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpio(WKP, RISING)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpio(WKP, FALLING)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpio(WKP, CHANGE)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpios(Vector<std::pair<hal_pin_t, InterruptMode>>())); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpios(Vector<hal_pin_t>(), RISING)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().gpios(pins_array, sizeof(pins_array)/sizeof(*pins_array), RISING)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().duration(1000)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().duration(1000ms)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().duration(1s)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().duration(1min)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().duration(1h)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().flag(SystemSleepFlag::WAIT_CLOUD)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().network(NETWORK_INTERFACE_ETHERNET)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().network(NETWORK_INTERFACE_CELLULAR)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().network(NETWORK_INTERFACE_WIFI_STA)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().analog(A0, 1800, AnalogInterruptMode::CROSS)); (void)r; });
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().usart(Serial1)); (void)r; });

#if HAL_PLATFORM_BLE
    API_COMPILE({ SystemSleepResult r = System.sleep(SystemSleepConfiguration().ble()); (void)r; });
#endif

    API_COMPILE({
        SystemSleepResult r;
        (void)r.setError(SYSTEM_ERROR_NONE);
        (void)r.setError(SYSTEM_ERROR_NONE, true);
        (void)r.wakeupReason();
        (void)r.wakeupPin();
        (void)r.error();
        r = SystemSleepResult(SleepResult());
        SleepResult test = r;
        (void)test;
    });
}

test(system_mode) {
    API_COMPILE(System.mode());
    API_COMPILE(SystemClass dummy(AUTOMATIC));
    API_COMPILE(SystemClass dummy(SEMI_AUTOMATIC));
    API_COMPILE(SystemClass dummy(MANUAL));

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

    API_COMPILE(Serial.println(PP_STR(SYSTEM_VERSION_STRING)));
    API_COMPILE(Serial.println(SYSTEM_VERSION));
}

test(hardware_info) {
    SystemHardwareInfo info;
    API_COMPILE({ info = System.hardwareInfo(); });
    API_COMPILE(info.isValid());
    API_COMPILE(info.model());
    API_COMPILE(info.variant());
    API_COMPILE(info.revision());
    API_COMPILE((bool)info);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
    // These cases are designed to all fall through
    switch (event) {
    case network_status: {
        switch (data) {
        case network_status_powering_off:
        case network_status_off:
        case network_status_powering_on:
        case network_status_on:
        case network_status_connecting:
        case network_status_connected:
        case network_status_disconnecting:
        case network_status_disconnected:
            break;
        }
        break;
    }
    case cloud_status: {
        switch (data) {
        case cloud_status_disconnected:
        case cloud_status_connecting:
        case cloud_status_connected:
        case cloud_status_disconnecting:
            break;
        }
        break;
    }
    default:
        break;
    }
#pragma GCC diagnostic pop
}

class EventsHandler {
public:
    void handler3(system_event_t events, int data, void* pointer) {}
    void handler2(system_event_t events, int data) {}
    void handler1(system_event_t events) {}
    void handler0() {}
};

EventsHandler handlerInstance;

test(system_events)
{
    using namespace std::placeholders;
    int clicks = system_button_clicks(123);
    system_event_t my_events = wifi_listen_begin + wifi_listen_end + wifi_listen_update +
                               setup_begin + setup_end + setup_update + network_credentials +
                               network_status + cloud_status + button_status + button_click + button_final_click +
                               reset + reset_pending + firmware_update + firmware_update_pending +
                               firmware_update_status + all_events;

    API_COMPILE(System.on(my_events, handler));
    API_COMPILE(System.on(my_events, handler_event));
    API_COMPILE(System.on(my_events, handler_event_data));
    API_COMPILE(System.on(my_events, handler_event_data_param));
    API_COMPILE(System.on(my_events, &EventsHandler::handler3, &handlerInstance));
    API_COMPILE(System.on(my_events, &EventsHandler::handler2, &handlerInstance));
    API_COMPILE(System.on(my_events, &EventsHandler::handler1, &handlerInstance));
    API_COMPILE(System.on(my_events, &EventsHandler::handler0, &handlerInstance));
    API_COMPILE(System.on(my_events, std::bind(&EventsHandler::handler3, &handlerInstance, _1, _2, _3)));
    API_COMPILE(System.on(my_events, std::function<void(system_event_t, int)>(std::bind(&EventsHandler::handler2, &handlerInstance, _1, _2))));
    API_COMPILE(System.on(my_events, std::function<void(system_event_t)>(std::bind(&EventsHandler::handler1, &handlerInstance, _1))));
    API_COMPILE(System.on(my_events, std::function<void()>(std::bind(&EventsHandler::handler0, &handlerInstance))));
    API_COMPILE(System.on(my_events, [](system_event_t events, int data, void* pointer) {}));
    API_COMPILE(System.on(my_events, [](system_event_t events, int data) {}));
    API_COMPILE(System.on(my_events, [](system_event_t events) {}));
    API_COMPILE(System.on(my_events, []() {}));
    API_COMPILE(System.on(my_events, [&](system_event_t events, int data, void* pointer) {}));
    API_COMPILE(System.on(my_events, [&](system_event_t events, int data) {}));
    API_COMPILE(System.on(my_events, [&](system_event_t events) {}));
    API_COMPILE(System.on(my_events, [&]() {}));

    API_COMPILE(System.off(my_events));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    API_COMPILE(System.off(handler_event_data_param));
    API_COMPILE(System.off(my_events, handler_event_data_param));
#pragma GCC diagnostic pop
    SystemEventSubscription sub;
    API_COMPILE(sub = System.on(my_events, [&](system_event_t events, int data, void* pointer) {}));
    API_COMPILE(System.off(sub));

    API_COMPILE({ auto r = System.on(my_events, handler) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, handler_event) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, handler_event_data) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, handler_event_data_param) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, &EventsHandler::handler3, &handlerInstance) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, &EventsHandler::handler2, &handlerInstance) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, &EventsHandler::handler1, &handlerInstance) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, &EventsHandler::handler0, &handlerInstance) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, std::bind(&EventsHandler::handler3, &handlerInstance, _1, _2, _3)) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, std::function<void(system_event_t, int)>(std::bind(&EventsHandler::handler2, &handlerInstance, _1, _2))) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, std::function<void(system_event_t)>(std::bind(&EventsHandler::handler1, &handlerInstance, _1))) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, std::function<void()>(std::bind(&EventsHandler::handler0, &handlerInstance))) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [](system_event_t events, int data, void* pointer) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [](system_event_t events, int data) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [](system_event_t events) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, []() {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [&](system_event_t events, int data, void* pointer) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [&](system_event_t events, int data) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [&](system_event_t events) {}) == true; (void)r; });
    API_COMPILE({ auto r = System.on(my_events, [&]() {}) == true; (void)r; });

    (void)clicks; // avoid unused variable warning
}

test(system_flags)
{
    // SYSTEM_FLAG_OTA_UPDATE_ENABLED
    API_COMPILE(System.enableUpdates());
    API_COMPILE(System.disableUpdates());
    API_COMPILE(System.updatesEnabled());
    // SYSTEM_FLAG_OTA_UPDATE_PENDING
    API_COMPILE(System.updatesPending());
    // SYSTEM_FLAG_RESET_ENABLED
    API_COMPILE(System.enableReset());
    API_COMPILE(System.disableReset());
    API_COMPILE(System.resetEnabled());
    // SYSTEM_FLAG_RESET_PENDING
    API_COMPILE(System.resetPending());
    // Generic API
    API_COMPILE(System.enable(SYSTEM_FLAG_MAX));
    API_COMPILE(System.disable(SYSTEM_FLAG_MAX));
    API_COMPILE(System.enabled(SYSTEM_FLAG_MAX));
}

test(system_update)
{
    int r = 0;
    API_COMPILE(r = System.updateStatus());
    API_COMPILE(r == UpdateStatus::UNKNOWN || r == UpdateStatus::NOT_AVAILABLE || r == UpdateStatus::PENDING ||
            r == UpdateStatus::IN_PROGRESS);
}

#if defined(USER_BACKUP_RAM)
static retained uint8_t app_backup[USER_BACKUP_RAM];

test(backup_ram)
{
    // Not designed to be run!
    // only here to prevent compiler from optimizing out the app_backup array.
	int total = 0;
	for (unsigned i=0; i<sizeof(app_backup); i++) {
		total += app_backup[i];		// 8 bytes for the
	}
	Serial.println(total);
}

#endif // defined(USER_BACKUP_RAM)

test(system_mode_button)
{
    API_COMPILE(System.buttonMirror(D1, RISING));
    API_COMPILE(System.buttonMirror(D1, FALLING));
    API_COMPILE(System.buttonMirror(D1, RISING, true));
    API_COMPILE(System.buttonMirror(D1, FALLING, true));

    API_COMPILE(System.disableButtonMirror());
    API_COMPILE(System.disableButtonMirror(false));
}

test(system_uptime_millis) {
    uint64_t u64 = 0;
    unsigned u = 0;

    API_COMPILE(u64 = System.millis());
    API_COMPILE(u = System.uptime());

    (void)u64;
    (void)u;
}

#if HAL_PLATFORM_POWER_MANAGEMENT
test(system_power_management) {
    SystemPowerConfiguration conf;
    API_COMPILE(conf.powerSourceMinVoltage(1234));
    API_COMPILE(conf.powerSourceMaxCurrent(1234));
    API_COMPILE(conf.batteryChargeVoltage(1234));
    API_COMPILE(conf.batteryChargeCurrent(1234));
    API_COMPILE(conf.socBitPrecision(19));
    API_COMPILE(conf.feature(SystemPowerFeature::PMIC_DETECTION));
    API_COMPILE(conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST));
    API_COMPILE(conf.feature(SystemPowerFeature::DISABLE));
    API_COMPILE(conf.feature(SystemPowerFeature::DISABLE_CHARGING));

    API_COMPILE(System.setPowerConfiguration(conf));

    API_COMPILE({ auto source = System.powerSource() == POWER_SOURCE_VIN; (void)source; });
    API_COMPILE({ auto state = System.batteryState() == BATTERY_STATE_CHARGING; (void)state; });
    API_COMPILE({ auto charge = System.batteryCharge(); (void)charge; });

    SystemPowerConfiguration getConf = {};
    API_COMPILE({ getConf = System.getPowerConfiguration(); });
    API_COMPILE({ auto featurePmic = getConf.isFeatureSet(SystemPowerFeature::PMIC_DETECTION); (void)featurePmic; });
    API_COMPILE({ auto featureVin = getConf.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST); (void)featureVin; });
    API_COMPILE({ auto featureDisable = getConf.isFeatureSet(SystemPowerFeature::DISABLE); (void)featureDisable; });
    API_COMPILE({ auto featureCharging = getConf.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING); (void)featureCharging; });
    API_COMPILE({ auto getSourceA = getConf.powerSourceMaxCurrent(); (void)getSourceA; });
    API_COMPILE({ auto getSourceV = getConf.powerSourceMinVoltage(); (void)getSourceV; });
    API_COMPILE({ auto getBatteryA = getConf.batteryChargeCurrent(); (void)getBatteryA; });
    API_COMPILE({ auto getBatteryV = getConf.batteryChargeVoltage(); (void)getBatteryV; });
    API_COMPILE({ auto getSocBitPrecision = getConf.socBitPrecision(); (void)getSocBitPrecision; });
}
#endif // HAL_PLATFORM_POWER_MANAGEMENT
