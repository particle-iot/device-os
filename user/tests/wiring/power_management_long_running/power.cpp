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

#define SERIAL      Serial1

namespace {

constexpr uint32_t DISCHARGE_STATE1 = 0xc00f;
constexpr uint32_t DISCHARGE_STATE2 = 0xd00f;
constexpr uint32_t CHARGING_STATE_DISABLED_CHARGING = 0xbeef;
constexpr uint32_t CHARGING_STATE_ENABLED_CHARGING = 0xdeed;
constexpr uint32_t CHARGED_STATE = 0xc0fe;
constexpr uint32_t DISCONNECTED_STATE = 0xd00d;
constexpr uint32_t DISCONNECTED_STATE_SLEEP = 0xbead;
constexpr uint32_t CONNECTED_STATE_SLEEP = 0xb007;
uint32_t g_state;
bool dischargeTested = false;

void startup() {
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    System.setPowerConfiguration(conf);
}

STARTUP(startup());

void testDischarge() {
    SCOPE_GUARD ({
        dischargeTested = true;
        if (Particle.connected()) {
            SERIAL.println("> Disconnect from the cloud...");
            Particle.disconnect();
            SERIAL.println("> Turn off the Cellular modem.");
            Cellular.off();
        }
        if (!Serial.isConnected()) {
            SERIAL.println("\r\n> Please \"plug\" the USB and reconnect the Serial to continue the test.");
            while (!Serial.isConnected());
            delay(1s);
            while (SERIAL.available() > 0) {
                (void)SERIAL.read();
            }
        }
    });
    if (!dischargeTested) {
        constexpr float DISCHARGE_TERMINATION_VCELL = 3.90f;

        SERIAL.println("> Please \"unplug\" the USB");
        while (Serial.isConnected());

        SERIAL.println("> Wait the power management settled...");
        assertTrue(waitFor([]{
            return System.batteryState() == BATTERY_STATE_DISCHARGING;
        }, 10000));
        assertEqual(System.powerSource(), (int)POWER_SOURCE_BATTERY);

        SERIAL.print("> Device is connecting to the cloud... ");
        Particle.connect();
        assertTrue(waitFor(Particle.connected, 120000)); // 2 minutes
        SERIAL.println("connected.");

        SERIAL.println("> Device is publishing message in every second to discharge the battery...");
        FuelGauge fuel;
        float vcell = fuel.getVCell();
        SERIAL.printlnf("> VBAT: %0.2fv", vcell);

        SERIAL.println("> Test is running...");
        uint8_t count = 0;
        time32_t now = Time.now();
        while (vcell > DISCHARGE_TERMINATION_VCELL || count < 10) {
            if (Time.now() - now > 1) {
                now = Time.now();
                Particle.publish("Hello world!");
                assertEqual(System.batteryState(), (int)BATTERY_STATE_DISCHARGING);
                assertEqual(System.powerSource(), (int)POWER_SOURCE_BATTERY);

                vcell = fuel.getVCell();
                if (vcell <= DISCHARGE_TERMINATION_VCELL) {
                    count++;
                }
                // Threading is not enabled in this test application, need a delay to give the system thread a chance to run.
                delay(1s);
            }
        }
    }
}

} // anonymous

test(power_00_setup) {
    SERIAL.begin(115200);
    SERIAL.println("\r\npower_00_setup");

    constexpr float DISCHARGE_VCELL = 4.1f;

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);

    while (SERIAL.available() > 0) {
        (void)SERIAL.read();
    }

    SERIAL.println("> Please \"plug\" both of the battery and USB. Press any key to continue when the hardware is set up.");
    while (!Serial.isConnected());
    while (SERIAL.available() <= 0);
    while (SERIAL.available() > 0) {
        (void)SERIAL.read();
    }

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return (System.batteryState()!= BATTERY_STATE_UNKNOWN) && (System.powerSource() == POWER_SOURCE_USB_HOST);
    }, 10000));
    PMIC power;
    assertTrue(power.isChargingEnabled());

    FuelGauge fuel;
    float vcell = fuel.getVCell();
    if (System.batteryState() == BATTERY_STATE_CHARGED) {
        assertMoreOrEqual(vcell, 4.0f);
    }
    SERIAL.printlnf("> VBAT: %0.2fv", vcell);
    if (vcell > DISCHARGE_VCELL || System.batteryState() == BATTERY_STATE_CHARGED) {
        // Let's discharge the battery first.
        g_state = DISCHARGE_STATE1;
    } else {
        g_state = CHARGING_STATE_DISABLED_CHARGING;
    }
}

test(power_01_battery_discharging) {
    SERIAL.println("\r\npower_01_battery_discharging");
    if (g_state != DISCHARGE_STATE1) {
        skip();
        return;
    }
    testDischarge();
    g_state = CHARGING_STATE_DISABLED_CHARGING;
}

test(power_02_battery_charging_when_charging_disabled) {
    SERIAL.println("\r\npower_02_battery_charging_when_charging_disabled");
    if (g_state != CHARGING_STATE_DISABLED_CHARGING) {
        skip();
        return;
    }

    constexpr time32_t TEST_DURATION = 10 * 60; // 10 minutes

    SERIAL.println("> Disable charging.");
    int ret = System.setPowerConfiguration(System.getPowerConfiguration().feature(SystemPowerFeature::DISABLE_CHARGING));
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), true);

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return System.batteryState() == BATTERY_STATE_NOT_CHARGING;
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
    PMIC power;
    assertFalse(power.isChargingEnabled());

    SERIAL.println("> Test is running...");
    time32_t start = Time.now();
    while ((Time.now() - start) < TEST_DURATION) {
        assertEqual(System.batteryState(), (int)BATTERY_STATE_NOT_CHARGING);
        assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
        delay(1s);
    }

    g_state = CHARGING_STATE_ENABLED_CHARGING;
}

test(power_03_battery_charging_when_charging_enabled) {
    SERIAL.println("\r\npower_03_battery_charging_when_charging_enabled");
    if (g_state != CHARGING_STATE_ENABLED_CHARGING) {
        skip();
        return;
    }

    constexpr float CHARGED_SOC_THRESHOLD = 95.0f;
    const time32_t chargeStart = Time.now();

    SERIAL.println("> Enable charging.");
    int ret = System.setPowerConfiguration(System.getPowerConfiguration().clearFeature(SystemPowerFeature::DISABLE_CHARGING));
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return System.batteryState() == BATTERY_STATE_CHARGING;
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
    PMIC power;
    assertTrue(power.isChargingEnabled());

    SERIAL.println("> Test is running...");
    while (System.batteryState() == BATTERY_STATE_CHARGING) {
        assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
        delay(5s);
        FuelGauge fuel;
        float batterySoc = System.batteryCharge();
        SERIAL.printlnf("SoC: %0.2f, Vol: %0.2f", batterySoc, fuel.getVCell());
    }
    SERIAL.printlnf("> Battery is charged using %lds.", (Time.now() - chargeStart));
    assertEqual(System.batteryState(), (int)BATTERY_STATE_CHARGED);
    
    float soc = System.batteryCharge();
    assertMoreOrEqual(soc, CHARGED_SOC_THRESHOLD);

    SERIAL.println("> Battery is charged.");

    g_state = CHARGED_STATE;
}

test(power_04_battery_charged) {
    SERIAL.println("\r\npower_04_battery_charged");
    if (g_state != CHARGED_STATE) {
        skip();
        return;
    }

    constexpr time32_t TEST_DURATION = 10 * 60; // 10 minutes

    assertEqual(System.batteryState(), (int)BATTERY_STATE_CHARGED);
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
    PMIC power;
    assertTrue(power.isChargingEnabled());

    // When the battery is charged and the USB is connected, the battery should
    // reside in the charged state. While for problematic battery, the voltage
    // drops too much once stop charging due to charged. If the voltage drops more
    // than 100mV, the PMIC will start charging again. From the human view, the charging
    // status LED will blink.
    SERIAL.println("> Test is running...");
    time32_t start = Time.now();
    while ((Time.now() - start) < TEST_DURATION) {
        if (System.batteryState() == BATTERY_STATE_CHARGING) {
            SERIAL.println("> WARN: (problematic) battery is charging!");
        } else {
            assertEqual(System.batteryState(), (int)BATTERY_STATE_CHARGED);
        }
        assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
    }

    g_state = DISCHARGE_STATE2;
}

test(power_05_battery_discharging) {
    SERIAL.println("\r\npower_05_battery_discharging");
    if (g_state != DISCHARGE_STATE2) {
        skip();
        return;
    }
    testDischarge();
    g_state = DISCONNECTED_STATE;
}


test(power_06_battery_disconnected) {
    SERIAL.println("\r\npower_06_battery_disconnected");
    if (g_state != DISCONNECTED_STATE) {
        skip();
        return;
    }

    constexpr time32_t TEST_DURATION = 10 * 60; // 10 minutes

    SERIAL.println("> Please \"unplug\" the battery. Press any key to continue when the hardware is set up.");
    while (SERIAL.available() <= 0);
    while (SERIAL.available() > 0) {
        (void)SERIAL.read();
    }

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return System.batteryState() == BATTERY_STATE_DISCONNECTED;
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);

    SERIAL.println("> Test is running...");
    time32_t start = Time.now();
    while ((Time.now() - start) < TEST_DURATION) {
        assertEqual(System.batteryState(), (int)BATTERY_STATE_DISCONNECTED);
        assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
        delay(1s);
    }

    g_state = DISCONNECTED_STATE_SLEEP;
}

test(power_07_battery_disconnected_and_sleep) {
    SERIAL.println("\r\npower_07_battery_disconnected_and_sleep");
    if (g_state != DISCONNECTED_STATE_SLEEP) {
        skip();
        return;
    }

    constexpr system_tick_t SLEEP_DURATION = 10 * 1000; // 10 seconds

    SERIAL.printlnf("> Device will sleep for %ld seconds", SLEEP_DURATION);
    SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(SLEEP_DURATION));
    assertEqual(r.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)r.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

    SERIAL.println("> Device wakes up. Please reconnect the Serial to continue the test.");
    while (!Serial.isConnected());
    delay(1s);

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return System.batteryState() == BATTERY_STATE_DISCONNECTED;
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);

    g_state = CONNECTED_STATE_SLEEP;
}

test(power_08_battery_connected_and_sleep) {
    SERIAL.println("\r\npower_08_battery_connected_and_sleep");
    if (g_state != CONNECTED_STATE_SLEEP) {
        skip();
        return;
    }

    constexpr system_tick_t SLEEP_DURATION = 10 * 1000; // 10 seconds

    SERIAL.println("> Please \"plug\" the battery. Press any key to continue when the hardware is set up.");
    while (SERIAL.available() <= 0);
    while (SERIAL.available() > 0) {
        (void)SERIAL.read();
    }

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([]{
        return System.batteryState() != BATTERY_STATE_UNKNOWN;
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);

    delay(10s); // When battery is pluged, the PMIC may take some time to finally settle the battery state.
    static int currState = System.batteryState();

    SERIAL.printlnf("> Device will sleep for %ld seconds", SLEEP_DURATION);
    SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(SLEEP_DURATION));
    assertEqual(r.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)r.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

    SERIAL.println("> Device wakes up. Please reconnect the Serial to continue the test.");
    while (!Serial.isConnected());
    delay(1s);

    SERIAL.println("> Wait the power management settled...");
    assertTrue(waitFor([&]{
        if (currState == BATTERY_STATE_CHARGING || currState == BATTERY_STATE_CHARGED) {
            // FIXME: when the battery is charged, the PMIC may be still jumping between fast charging and charged state
            // due to 100mV recharge threshold.
            int state = System.batteryState();
            return (state == BATTERY_STATE_CHARGING || state == BATTERY_STATE_CHARGED);
        } else {
            return System.batteryState() == currState;
        }
    }, 10000));
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
}
