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

#include "scope_guard.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
UNIT_TEST_APP();

SYSTEM_THREAD(ENABLED);

#define SERIAL      Serial1

namespace {

static const char* txString = 
"urjlU1tW177HwJsR6TylreMKge225qyLaIizW5IhXHkWgTGpH2fZtm2Od20Ne3Q81fxfUl7zoFaF\
Z6smPzkpTGNSxGg7TCEiE2f19951tKxjFCB4Se86R4CaWW2YZF0mogirgsu2qRMGe4mC9QlJkCgXP\
bgSVV1mc2xsZcu4bj0pbmPIhxkuyAHe4cVK3gLpWEGTadtAn2k66rOFNBdfPaE0cUY3wwXlVQ9yDl\
OxexepQcC2WTrbUe4z85OSae9s8A6BwUCRBYYfEH01cnGCzYGCEOEm5jl4nJ3HqWckHI5K2NeWS4x\
EhkgMqG3RwfOTM85SQ7q7NLIhgprCTsBTzv2YpGgbAB7oSX0joGQHxfyndxIyCVIHknvEj1hynXtw\
uebA6i7JBFiGkk4AnRzk7v3dNjHt6weuYWtf6yj3aVzhbMaWFCR6HOKFc3i8XzBsnLTc4Uzft61a0\
qV8ZssHdHO7sbiojOmA37RkrNUFxX1aODUXWNEntkTylwvhxKpsAb6Lsopzve4ea2G17WpW62Z12x\
mNgTZQHOo3fCZDy8L7WfVwCJiJunHPXu9jw6g11NJFcpo2AakkZQDgUGZoeZgDB6GfRheAiurAEB5\
Ym4EVIQB9AvVBf4zY84R8D4bnfjwwLDwiZSo9y2Z5JsVQ0yRdqPdxv0cV2Kp0AaevITeubJseCXOg\
LkFiaeDTBoR7kyMyoJvJl4vjLmiV03RNSAl9JpZkBfTHzalZw8oaRHMMiTVVGdieJOIbANoaXyRbe\
xSYU1t5dOe8wxybwfBBlPIswpVJ45kXd4Bu8NCLXPAbgJCOVSlTQsfvzVKZykp9V1DBQ3PwyeBXJB\
QsLDslIOHOKbfqB8njXotpE3Dz46Wi6QtpinLsSiviZmz62qLW5Pd9M7SDCarrxFk8SBHyJl2KdjH\
5Lx1LmkW8gMiYquOctT9xhFNs406BxWrPcTc5kwaSJ6RJQyohQEJk9ojchrbSo4ucfZGQzEMBEIJs";

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
        if (Network.ready()) {
            SERIAL.println("> Turn off the modem.");
            Network.off();
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
        constexpr float DISCHARGE_TERMINATION_VCELL = 3.85f;

        SERIAL.println("> Please \"unplug\" the USB");
        while (Serial.isConnected());

        SERIAL.println("> Wait the power management settled...");
        assertTrue(waitFor([]{
            return System.batteryState() == BATTERY_STATE_DISCHARGING;
        }, 10000));
        assertEqual(System.powerSource(), (int)POWER_SOURCE_BATTERY);

        SERIAL.print("> Device is connecting to network... ");
        Network.connect();
        assertTrue(waitFor(Network.ready, 120000)); // 2 minutes
        SERIAL.println("connected.");

        SERIAL.println("> Device is sending UDP packets to discharge the battery...");

        UDP udp;
        IPAddress remoteIP(192, 168, 1, 100);
        int port = 1337;
        udp.begin(8080);

        SERIAL.println("> Test is running...");
        uint8_t count = 0;
        FuelGauge fuel;
        float vcell = fuel.getVCell();
        while (count < 100) {
            assertEqual(System.batteryState(), (int)BATTERY_STATE_DISCHARGING);
            assertEqual(System.powerSource(), (int)POWER_SOURCE_BATTERY);

            const auto v = fuel.getVCell();
            if ((v + 0.01f) < vcell) {
                vcell = v;
                SERIAL.printlnf("> VBAT: %0.2fv", vcell);
            }
            if (vcell <= DISCHARGE_TERMINATION_VCELL) {
                count++;
            }

            udp.sendPacket(txString, strlen(txString), remoteIP, port);
            delay(10);
        }

        udp.stop();
    }
}

} // anonymous

test(power_00_setup) {
    SERIAL.begin(115200);
    SERIAL.println("\r\npower_00_setup");

    constexpr float DISCHARGE_VCELL = 4.0f;

    assertEqual(
        System.setPowerConfiguration(
            SystemPowerConfiguration().feature(SystemPowerFeature::PMIC_DETECTION)
                                      .powerSourceMaxCurrent(particle::power::DEFAULT_INPUT_CURRENT_LIMIT)
                                      .batteryChargeVoltage(particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT)
                                      .batteryChargeCurrent(particle::power::DEFAULT_CHARGE_CURRENT)
                                      .batteryChargeVoltage(particle::power::DEFAULT_TERMINATION_VOLTAGE)
                                      .socBitPrecision(particle::power::DEFAULT_SOC_18_BIT_PRECISION)),
        (int)SYSTEM_ERROR_NONE
    );
    delay(1s);

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
    assertEqual(cfg.socBitPrecision(), particle::power::DEFAULT_SOC_18_BIT_PRECISION);

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
        return (System.batteryState() != BATTERY_STATE_UNKNOWN) && (System.batteryState() != BATTERY_STATE_DISCONNECTED) && (System.powerSource() == POWER_SOURCE_USB_HOST);
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
    float soc = System.batteryCharge();
    while (System.batteryState() == BATTERY_STATE_CHARGING || soc < CHARGED_SOC_THRESHOLD) {
        assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
        delay(5s);
        FuelGauge fuel;
        auto newSoc = System.batteryCharge();
        if (newSoc != soc) {
            soc = newSoc;
            SERIAL.printlnf("SoC: %0.2f, Vol: %0.2f", soc, fuel.getVCell());
        }
    }
    SERIAL.printlnf("> Battery is charged using %lds.", (Time.now() - chargeStart));
    assertEqual(System.batteryState(), (int)BATTERY_STATE_CHARGED);

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
        assertEqual(System.batteryState(), (int)BATTERY_STATE_CHARGED);
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

    SERIAL.printlnf("> Device will sleep for %ld ms", SLEEP_DURATION);
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
    int currState = System.batteryState();

    SERIAL.printlnf("> Device will sleep for %ld ms", SLEEP_DURATION);
    SystemSleepResult r = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(SLEEP_DURATION));
    assertEqual(r.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)r.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

    SERIAL.println("> Device wakes up. Please reconnect the Serial to continue the test.");
    while (!Serial.isConnected());
    delay(10s);

    assertEqual(System.batteryState(), currState);
    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);

    SERIAL.println("> Done.");
}
