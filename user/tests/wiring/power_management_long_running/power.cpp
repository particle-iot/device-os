/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"

#if !HAL_PLATFORM_POWER_MANAGEMENT
#error "Unsupported platform"
#endif // !HAL_PLATFORM_POWER_MANAGEMENT

SYSTEM_MODE(SEMI_AUTOMATIC);
UNIT_TEST_APP();

#define SERIAL    Serial1

namespace {

constexpr char const* batteryStates[] = {
    "unknown", "not charging", "charging", "charged", "discharging", "fault", "disconnected"
};
constexpr char const* powerSources[] = {
    "unknown", "vin", "usb host", "usb adapter", "usb otg", "battery"
};
const char* commandsList =
"\r\nCommands:\r\n\
q: Exit the long running test\r\n\
c: Connect to the cloud and publish message every second\r\n\
d: Disconnect from the cloud and turn off the modem\r\n\
p: Print the current system power management configuration\r\n\
s: Enter sleep mode\r\n\
0: Disable charging\r\n\
1: Enable charging\r\n\r\n\
To discharge the battery, unplug the USB and make the device connect to the cloud.\r\n\r\n\
Randomly unplug the battery or USB host (Do not unplug both at the same time to power off the device) \
to observe the power status.\r\n";

int powerSource = -1;
int batteryState = -1;
bool updated = false;
system_tick_t logTimeStamp;
system_tick_t pubTimeStamp;

bool batteryStateUpdated() {
    return batteryState != -1 && batteryState != BATTERY_STATE_UNKNOWN;
}

bool powerSourceUpdated() {
    return powerSource != -1 && powerSource != POWER_SOURCE_UNKNOWN;
}

bool powerManagementSettled() {
    return batteryStateUpdated() && powerSourceUpdated();
}

void startup() {
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    System.setPowerConfiguration(conf);

    System.on(battery_state, [](system_event_t event, int data) -> void {
        batteryState = System.batteryState();
        updated = true;
    });

    System.on(power_source, [](system_event_t event, int data) -> void {
        powerSource = System.powerSource();
        updated = true;
    });
}

STARTUP(startup());

} // anonymous

test(power_00_setup) {
    SERIAL.begin(115200);
    SERIAL.println("Test started.");

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);

    SERIAL.println("Wait the power management settled...");
    waitFor(powerManagementSettled, 10000);

    while (SERIAL.available() > 0) {
        (void)SERIAL.read();
    }

    logTimeStamp = pubTimeStamp = Time.now();
}

test(power_01_state_detection_long_running) {
    constexpr system_tick_t LOG_INTERVAL = 5;
    constexpr system_tick_t PUBLISH_INTERVAL = 1;

    SERIAL.printlnf("%s", commandsList);

    bool exit = false;
    while (!exit) {
        if (SERIAL.available()) {
            char cmd = SERIAL.read();
            SERIAL.printlnf("\r\nCommand: %c", cmd);
            switch (cmd) {
                case 'q': {
                    SERIAL.println("Exit.\r\n");
                    exit = true;
                    break;
                }
                case 'c': {
                    SERIAL.println("Connecting to the cloud...");
                    Particle.connect();
                    waitUntil(Particle.connected);
                    SERIAL.println("Connected. It will keep publishing message in every second.\r\n");
                    break;
                }
                case 'd': {
                    SERIAL.println("Disconnect from the cloud.\r\n");
                    Particle.disconnect();
                    break;
                }
                case 'p': {
                    auto cfg = System.getPowerConfiguration();
                    SERIAL.printlnf("PMIC_DETECTION: %d\r\nUSE_VIN_SETTINGS_WITH_USB_HOST: %d\r\nDISABLE: %d\r\nDISABLE_CHARGING: %d\r\npowerSourceMinVoltage: %umV\r\npowerSourceMaxCurrent: %umA\r\nbatteryChargeVoltage: %umV\r\nbatteryChargeCurrent: %umA\r\n",
                        cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION) ? 1 : 0,
                        cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST) ? 1 : 0,
                        cfg.isFeatureSet(SystemPowerFeature::DISABLE) ? 1 : 0,
                        cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING) ? 1 : 0,
                        cfg.powerSourceMinVoltage(),
                        cfg.powerSourceMaxCurrent(),
                        cfg.batteryChargeVoltage(),
                        cfg.batteryChargeCurrent());
                    break;
                }
                case 's': {
                    SERIAL.println("Sleep for 10 seconds. The charging state should be consistent.\r\n");
                    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(10s));
                    break;
                }
                case '0': {
                    SERIAL.println("Disable charging.\r\n");
                    System.setPowerConfiguration(System.getPowerConfiguration().feature(SystemPowerFeature::DISABLE_CHARGING));
                    break;
                }
                case '1': {
                    SERIAL.println("Enable charging.\r\n");
                    System.setPowerConfiguration(System.getPowerConfiguration().clearFeature(SystemPowerFeature::DISABLE_CHARGING));
                    break;
                }
                default: {
                    SERIAL.println("Undefined command.");
                    SERIAL.printlnf("%s", commandsList);
                    break;
                }
            }
        }

        if (updated || (Time.now() - logTimeStamp) > LOG_INTERVAL) {
            updated = false;
            logTimeStamp = Time.now();
            FuelGauge fuel;
            SERIAL.printlnf("%s, %s, %0.2f%%, %0.2fv",
                powerSources[std::max(0, powerSource)], batteryStates[std::max(0, batteryState)], System.batteryCharge(), fuel.getVCell());
        }

        if (Particle.connected() && (Time.now() - pubTimeStamp) > PUBLISH_INTERVAL) {
            pubTimeStamp = Time.now();
            Particle.publish("Hello world!");
        }
    }
}
