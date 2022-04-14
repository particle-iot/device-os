/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#include "dct.h"

#if !HAL_PLATFORM_POWER_MANAGEMENT
#error "Unsupported platform"
#endif // !HAL_PLATFORM_POWER_MANAGEMENT

SYSTEM_MODE(MANUAL);
UNIT_TEST_APP();

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

constexpr uint32_t STATE0_MAGICK = 0xc00f;
constexpr uint32_t STATE1_MAGICK = 0xbeef;
constexpr uint32_t STATE2_MAGICK = 0xdeed;
constexpr uint32_t STATE3_MAGICK = 0xc0fe;
constexpr uint32_t STATE4_MAGICK = 0xd00d;
constexpr uint32_t STATE5_MAGICK = 0xbead;
constexpr uint32_t STATE6_MAGICK = 0xb007;
constexpr uint32_t STATE7_MAGICK = 0x1234;
constexpr uint32_t STATE8_MAGICK = 0x8008;
constexpr uint32_t STATE9_MAGICK = 0xb00b;
constexpr uint32_t STATE10_MAGICK = 0xb077;
constexpr uint32_t STATE11_MAGICK = 0xfeed;
constexpr uint32_t STATE12_MAGICK = 0xfaad;
constexpr uint32_t STATE13_MAGICK = 0xface;
constexpr uint32_t STATE14_MAGICK = 0xfade;
constexpr uint32_t STATE15_MAGICK = 0xecc0;
retained uint32_t g_state;

int s_powerSource = -1;
int s_batteryState = -1;

const diag_source* s_batterStateDiagSource = nullptr;

bool inState() {
    switch (g_state) {
        case STATE0_MAGICK:
        case STATE1_MAGICK:
        case STATE2_MAGICK:
        case STATE3_MAGICK:
        case STATE4_MAGICK:
        case STATE5_MAGICK:
        case STATE6_MAGICK:
        case STATE7_MAGICK:
        case STATE8_MAGICK:
        case STATE9_MAGICK:
        case STATE10_MAGICK:
        case STATE11_MAGICK:
        case STATE12_MAGICK:
        case STATE13_MAGICK:
        case STATE14_MAGICK:
        case STATE15_MAGICK: {
            return true;
        }
    }

    return false;
}

bool powerManagementEventTrackingIsRequired() {
    switch (g_state) {
        case STATE1_MAGICK:
        case STATE3_MAGICK:
        case STATE6_MAGICK:
        case STATE8_MAGICK: {
            return true;
        }
    }

    return false;
}

spark::Vector<uint16_t> s_powerInputCurrentLimitTracking;

void captureInputCurrentLimit() {
    if (powerManagementEventTrackingIsRequired()) {
        // XXX: this is a hack to make sure that we get this event on every PMIC event (interrupt)
        // so that we can track how some parameters change
        if (!s_batterStateDiagSource) {
            diag_get_source(DIAG_ID_SYSTEM_BATTERY_STATE, &s_batterStateDiagSource, nullptr);
        }

        if (s_batterStateDiagSource) {
            auto diag = static_cast<particle::SimpleEnumDiagnosticData<particle::power::battery_state_t>*>(s_batterStateDiagSource->data);
            // Acquire power management lock to ensure thread safety
            if (diag) {
                PMIC power(true);
                auto val = power.getInputCurrentLimit();
                if (s_powerInputCurrentLimitTracking.size() == 0 || s_powerInputCurrentLimitTracking.last() != val) {
                    if (s_powerInputCurrentLimitTracking.size() < 1024) {
                        s_powerInputCurrentLimitTracking.append(val);
                    }
                }
                diag->operator=((battery_state_t)-1);
            }
        }
    }
}

void startup() {
    {
        PMIC power;
        power.begin();
        captureInputCurrentLimit();
    }

    System.on(battery_state, [](system_event_t event, int data) -> void {
        s_batteryState = data;
        captureInputCurrentLimit();
    });

    System.on(power_source, [](system_event_t event, int data) -> void {
        s_powerSource = data;
    });
}

bool batteryStateUpdated() {
    return s_batteryState != -1 && s_batteryState != BATTERY_STATE_UNKNOWN;
}

bool powerSourceUpdated() {
    return s_powerSource != -1 && s_powerSource != POWER_SOURCE_UNKNOWN;
}

bool powerManagementSettled() {
    return batteryStateUpdated() && powerSourceUpdated();
}

bool isChargingDisabled() {
    PMIC power(true);
    return ((power.readPowerONRegister() & 0b00110000) == 0b00000000);
}

void notifyAndReset(bool reset = true) {
#ifndef PARTICLE_TEST_RUNNER
    while (Serial.available() > 0) {
        Serial.read();
    }
    Serial.printlnf("Press any key to reset, restart the tests once the device reboots");
    while (Serial.available() <= 0);
#else
#error "This test needs to be modified to notify test runner that we are going to reboot"
#endif // PARTICLE_TEST_RUNNER
    if (reset) {
        System.reset();
    }
}

STARTUP(startup());

} // anonymous

test(POWER_00_Prepare) {
    if (!inState()) {
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::PMIC_DETECTION);
        System.setPowerConfiguration(conf);
        g_state = STATE0_MAGICK;
        notifyAndReset(false);
        // Reset PMIC to start with a clean slate and disconnect USB so that it doesn't interfere
        Serial.end();
        PMIC power(true);
        power.reset();
        // Finally reset ourselves
        System.reset();
    } else {
        skip();
    }
}

test(POWER_01_PoweredByUsbHostAndBatteryStateIsValid) {
    if (g_state != STATE0_MAGICK) {
        skip();
        return;
    }

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

    // Allow some time for the power management subsystem to settle
    waitFor(powerManagementSettled, 10000);

    assertEqual(System.powerSource(), (int)POWER_SOURCE_USB_HOST);
    assertTrue(System.batteryState() != BATTERY_STATE_UNKNOWN &&
            System.batteryState() != BATTERY_STATE_FAULT);

    if (System.batteryState() != BATTERY_STATE_DISCONNECTED) {
        assertTrue(System.batteryCharge() >= 0.0f);
    } else {
        assertEqual(System.batteryState(), (int)BATTERY_STATE_DISCONNECTED);
    }
}

test(POWER_02_PoweredByUsbHostPrepareWarmBootup) {
    if (g_state != STATE0_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle (just in case)
    waitFor(powerManagementSettled, 10000);
    PMIC power(true);
    assertMore(power.getInputCurrentLimit(), (uint16_t)100);
    g_state = STATE1_MAGICK;
    notifyAndReset();
}

test(POWER_03_PoweredByUsbHostNoCurrentGlitchWarmBootup) {
    if (g_state != STATE1_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle
    // Mandatory delay to allow to catch input current limit data
    delay(std::max(5000 - (int)millis(), 0));
    waitFor(powerManagementSettled, 10000);

    // Switch state immediately
    g_state = STATE2_MAGICK;

    assertNotEqual(s_powerInputCurrentLimitTracking.size(), 0);
    for (int i = 0; i < s_powerInputCurrentLimitTracking.size(); i++) {
        auto cur = s_powerInputCurrentLimitTracking[i];
        Serial.printlnf("%u mA", cur);
        // Check that there was no input current limit drop to 100mA
        assertMore(cur, (uint16_t)100);
    }
}

test(POWER_03_PoweredByUsbHostPrepareColdBootup) {
    if (g_state != STATE2_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle (just in case)
    PMIC power(true);
    assertMore(power.getInputCurrentLimit(), (uint16_t)100);

    // We are emulating cold boot by blocking system power manager,
    // enabling PMIC watchdog (which will reset PMIC state),
    // waiting for watchdog fault and resetting immediately
    power.resetWatchdog();
    power.setWatchdog(0b01); // 40s - minimum
    auto start = millis();
    Serial.println("Device will restart in about 40s, restart the tests once it boots up");
    while (millis() - start < 50000) {
        if (power.isWatchdogFault()) {
            // Disable Serial so it doesn't interferer with initial DPDM detection on boot
            Serial.end();
            g_state = STATE3_MAGICK;
            System.reset();
        }
    }
    // Watchdog fault never occured
    assertTrue(false);
}

test(POWER_04_PoweredByUsbHostNoCurrentGlitchColdBootup) {
    if (g_state != STATE3_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle
    // Mandatory delay to allow to catch input current limit data
    delay(std::max(5000 - (int)millis(), 0));
    waitFor(powerManagementSettled, 10000);

    // Switch state immediately
    g_state = STATE4_MAGICK;

    assertNotEqual(s_powerInputCurrentLimitTracking.size(), 0);
    uint16_t prev = 0;
    for (int i = 0; i < s_powerInputCurrentLimitTracking.size(); i++) {
        auto cur = s_powerInputCurrentLimitTracking[i];
        Serial.printlnf("%u mA", cur);
        // Check that there was no input current limit drop
        if (prev > 100) {
            assertMore(cur, (uint16_t)100);
        }
        prev = cur;
    }

    // Check that we've settled on > 100mA input current limit
    assertMore(PMIC().getInputCurrentLimit(), (uint16_t)100);
}

test(POWER_05_PrepareVin) {
    if (g_state != STATE4_MAGICK) {
        skip();
        return;
    }

    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
    System.setPowerConfiguration(conf);
    g_state = STATE5_MAGICK;
    notifyAndReset();
}

test(POWER_06_PoweredByVinAndBatteryStateIsValid) {
    if (g_state != STATE5_MAGICK) {
        skip();
        return;
    }

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
    assertEqual(cfg.socBitPrecision(), particle::power::DEFAULT_SOC_18_BIT_PRECISION);

    // Allow some time for the power management subsystem to settle
    waitFor(powerManagementSettled, 10000);
    assertEqual(System.powerSource(), (int)POWER_SOURCE_VIN);
    assertTrue(System.batteryState() != BATTERY_STATE_UNKNOWN &&
            System.batteryState() != BATTERY_STATE_FAULT);

    if (System.batteryState() != BATTERY_STATE_DISCONNECTED) {
        assertMoreOrEqual(System.batteryCharge(), 0.0f);
    } else {
        assertEqual(System.batteryState(), (int)BATTERY_STATE_DISCONNECTED);
    }
}

test(POWER_07_PoweredByVinPrepareWarmBootup) {
    if (g_state != STATE5_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle (just in case)
    waitFor(powerManagementSettled, 10000);
    PMIC power(true);
    assertMore(power.getInputCurrentLimit(), (uint16_t)100);
    g_state = STATE6_MAGICK;
    notifyAndReset();
}

test(POWER_08_PoweredByVinNoCurrentGlitchWarmBootup) {
    if (g_state != STATE6_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle
    // Mandatory delay to allow to catch input current limit data
    delay(std::max(5000 - (int)millis(), 0));
    waitFor(powerManagementSettled, 10000);

    // Switch state immediately
    g_state = STATE7_MAGICK;

    assertNotEqual(s_powerInputCurrentLimitTracking.size(), 0);
    for (int i = 0; i < s_powerInputCurrentLimitTracking.size(); i++) {
        auto cur = s_powerInputCurrentLimitTracking[i];
        Serial.printlnf("%u mA", cur);
        // Check that there was no input current limit drop to 100mA
        assertMore(cur, (uint16_t)100);
    }
}

test(POWER_09_PoweredByVinPrepareColdBootup) {
    if (g_state != STATE7_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle (just in case)
    PMIC power(true);
    assertMore(power.getInputCurrentLimit(), (uint16_t)100);

    // We are emulating cold boot by blocking system power manager,
    // enabling PMIC watchdog (which will reset PMIC state),
    // waiting for watchdog fault and resetting immediately
    power.resetWatchdog();
    power.setWatchdog(0b01); // 40s - minimum
    auto start = millis();
    Serial.println("Device will restart in about 40s, restart the tests once it boots up");
    while (millis() - start < 50000) {
        if (power.isWatchdogFault()) {
            // Disable Serial so it doesn't interferer with initial DPDM detection on boot
            Serial.end();
            g_state = STATE8_MAGICK;
            System.reset();
        }
    }
    // Watchdog fault never occured
    assertTrue(false);
}

test(POWER_10_PoweredByVinNoCurrentGlitchColdBootup) {
    if (g_state != STATE8_MAGICK) {
        skip();
        return;
    }

    // Allow some time for the power management subsystem to settle
    // Mandatory delay to allow to catch input current limit data
    delay(std::max(5000 - (int)millis(), 0));
    waitFor(powerManagementSettled, 10000);

    // Switch state immediately
    g_state = STATE9_MAGICK;

    assertNotEqual(s_powerInputCurrentLimitTracking.size(), 0);
    uint16_t prev = 0;
    for (int i = 0; i < s_powerInputCurrentLimitTracking.size(); i++) {
        auto cur = s_powerInputCurrentLimitTracking[i];
        Serial.printlnf("%u mA", cur);
        // Check that there was no input current limit drop
        if (prev > 100) {
            assertMore(cur, (uint16_t)100);
        }
        prev = cur;
    }

    // Check that we've settled on > 100mA input current limit
    assertEqual(PMIC().getInputCurrentLimit(), (uint16_t)particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
}

test(POWER_11_ApplyingDefaultPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state != STATE9_MAGICK) {
        skip();
        return;
    }

    // Apply default configuration
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
    assertEqual(cfg.socBitPrecision(), particle::power::DEFAULT_SOC_18_BIT_PRECISION);

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(power.getInputVoltageLimit(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(power.getChargeCurrentValue(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(power.getChargeVoltageValue(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
}

test(POWER_12_ApplyingCustomPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state != STATE9_MAGICK) {
        skip();
        return;
    }

    // Apply custom configuration
    SystemPowerConfiguration conf;
    // We are specifically using the values that don't exactly match what
    // the PMIC can use as settings and validating that they are being
    // mapped correctly later on.
    conf.powerSourceMaxCurrent(550);
    conf.powerSourceMinVoltage(4300);
    conf.batteryChargeCurrent(850);
    conf.batteryChargeVoltage(4210);
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), 550);
    assertEqual(cfg.powerSourceMinVoltage(), 4300);
    assertEqual(cfg.batteryChargeCurrent(), 850);
    assertEqual(cfg.batteryChargeVoltage(), 4210);

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);

    g_state = STATE10_MAGICK;
    delay(5000);
    notifyAndReset();
}

test(POWER_13_AppliedConfigurationIsPersisted) {
    if (g_state != STATE10_MAGICK) {
        skip();
        return;
    }

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), 550);
    assertEqual(cfg.powerSourceMinVoltage(), 4300);
    assertEqual(cfg.batteryChargeCurrent(), 850);
    assertEqual(cfg.batteryChargeVoltage(), 4210);

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);
}

test(POWER_14_PmicDetectionFlagIsCompatibleWithOldDeviceOsVersions) {
    if (g_state != STATE10_MAGICK) {
        skip();
        return;
    }

    // Enable PMIC_DETECTION feature
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    auto cfg = System.getPowerConfiguration();
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), true);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
#else
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), false);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), false);
    assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
    assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
    assertEqual(cfg.socBitPrecision(), particle::power::DEFAULT_SOC_18_BIT_PRECISION);

    // Feature enabled
    uint8_t v;
    assertEqual(dct_read_app_data_copy(DCT_POWER_CONFIG_OFFSET, &v, sizeof(v)), 0);
    assertTrue((v & 0x01) == 0x00);

    // Apply default configuration
    conf = SystemPowerConfiguration();
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    // Feature disabled
    assertEqual(dct_read_app_data_copy(DCT_POWER_CONFIG_OFFSET, &v, sizeof(v)), 0);
    assertTrue((v & 0x01) == 0x01);
}

test(POWER_15_DisableFeatureFlag) {
    if (g_state == STATE10_MAGICK) {
        // Set disabled feature flag
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::DISABLE);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        // NOTE: this setting will require a reboot to be applied
        assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
        g_state = STATE11_MAGICK;
        delay(5000);
        notifyAndReset();
    } else if (g_state == STATE11_MAGICK) {
        auto cfg = System.getPowerConfiguration();
        assertEqual(cfg.isFeatureSet(SystemPowerFeature::PMIC_DETECTION), false);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), true);
#else
        assertEqual(cfg.isFeatureSet(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST), false);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE), true);
        assertEqual(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING), false);
        assertEqual(cfg.powerSourceMaxCurrent(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
        assertEqual(cfg.powerSourceMinVoltage(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
        assertEqual(cfg.batteryChargeCurrent(), particle::power::DEFAULT_CHARGE_CURRENT);
        assertEqual(cfg.batteryChargeVoltage(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
        assertEqual(cfg.socBitPrecision(), particle::power::DEFAULT_SOC_18_BIT_PRECISION);

        assertEqual(System.batteryState(), (int)BATTERY_STATE_UNKNOWN);
        assertEqual(System.powerSource(), (int)POWER_SOURCE_UNKNOWN);
        // Restore default power management configuration
        // NOTE: this setting will require a reboot to be applied
        assertEqual(System.setPowerConfiguration(SystemPowerConfiguration()), (int)SYSTEM_ERROR_NONE);

        g_state = STATE12_MAGICK;
    } else {
        skip();
        return;
    }
}

test(POWER_16_PrepareDisableChargingFeatureFlag) {
    if (g_state != STATE12_MAGICK) {
        skip();
        return;
    }

    // Place device into known state and reset before advancing to the next test phase
    // The charge disable feature will not be set
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    System.setPowerConfiguration(conf);
    g_state = STATE13_MAGICK;
    notifyAndReset(false);
    // Reset PMIC to start with a clean slate and disconnect USB so that it doesn't interfere
    Serial.end();
    PMIC power(true);
    power.reset();
    // Finally reset ourselves
    System.reset();
}

test(POWER_17_DisableChargingFeatureFlag) {
    if (g_state == STATE13_MAGICK) {
        // Check that charging is not disabled after the last reset
        auto cfg = System.getPowerConfiguration();
        assertFalse(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
        // PMIC charging may be enabled or disabled based on power manager state
        assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);

        // Set charge disable feature flag
        {
            SystemPowerConfiguration conf;
            conf.feature(SystemPowerFeature::PMIC_DETECTION)
                .feature(SystemPowerFeature::DISABLE_CHARGING);
            assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
            delay(5000);
            // Check that charging is disabled after setting feature en vivo
            cfg = System.getPowerConfiguration();
            assertTrue(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
            // The charger configuration settings in PMIC better be disabled for charging
            assertTrue(isChargingDisabled());
            assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);
        }

        // Clear charge disable feature flag
        {
            SystemPowerConfiguration conf;
            conf.feature(SystemPowerFeature::PMIC_DETECTION);
            assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
            delay(5000);
            // Check that charging is enabled after setting feature en vivo
            cfg = System.getPowerConfiguration();
            assertFalse(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
            // PMIC charging may be enabled or disabled based on power manager state
            assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);
        }

        // Set charge disable feature flag
        {
            SystemPowerConfiguration conf;
            conf.feature(SystemPowerFeature::PMIC_DETECTION)
                .feature(SystemPowerFeature::DISABLE_CHARGING);
            assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
            delay(5000);
            // Check that charging is disabled after setting feature en vivo
            cfg = System.getPowerConfiguration();
            assertTrue(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
            // The charger configuration settings in PMIC better be disabled for charging
            assertTrue(isChargingDisabled());
            assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);
        }

        g_state = STATE14_MAGICK;
        notifyAndReset(false);
        // Reset PMIC to start with a clean slate and disconnect USB so that it doesn't interfere
        Serial.end();
        PMIC power(true);
        power.reset();
        // Finally reset ourselves
        System.reset();
    } else if (g_state == STATE14_MAGICK) {
        // Check that charging is disabled after the last reset
        auto cfg = System.getPowerConfiguration();
        assertTrue(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
        // The charger configuration settings in PMIC better be disabled for charging
        assertTrue(isChargingDisabled());
        assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);
    } else {
        skip();
        return;
    }
}

test(POWER_18_DisableChargingFeatureFlagWithWatchdog) {
    if (g_state != STATE14_MAGICK) {
        skip();
        return;
    }

    // Charging should be disabled when entering this test and is ensured in the previous test step
    // Reset the PMIC watchdog and make sure that the power manager recovers
    {
        PMIC power(true);
        power.resetWatchdog();
        power.setWatchdog(0b01); // 40s - minimum
    }
    auto start = millis();
    bool wdtResetted = false;
    Serial.println("Device will pause about 40s to check PMIC watchdog reset");
    while (millis() - start < 50000) {
        delay(500);
        PMIC power(true);
        // Determine reset status by monitoring that the timer is disabled
        if ((power.readChargeTermRegister() & 0b00110000) == 0) {
            wdtResetted = true;
            break;
        }
    }

    // Watchdog better have happened
    assertTrue(wdtResetted);
    // Check that charging is disabled after the watchdog reset
    auto cfg = System.getPowerConfiguration();
    assertTrue(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
    // The charger configuration settings in PMIC better be disabled for charging
    assertTrue(isChargingDisabled());
    assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);
}

test(POWER_19_NotDisableChargingFeatureFlagWithWatchdog) {
    if (g_state != STATE14_MAGICK) {
        skip();
        return;
    }

    // Charging disable should be now be cleared
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(5000);
    // Check that charging is enabled after setting feature en vivo
    auto cfg = System.getPowerConfiguration();
    assertFalse(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
    // PMIC charging may be enabled or disabled based on power manager state
    assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);

    // Reset the PMIC watchdog and make sure that the power manager recovers
    {
        PMIC power(true);
        power.resetWatchdog();
        power.setWatchdog(0b01); // 40s - minimum
    }
    auto start = millis();
    bool wdtResetted = false;
    Serial.println("Device will pause about 40s to check PMIC watchdog reset");
    while (millis() - start < 50000) {
        delay(500);
        PMIC power(true);
        // Determine reset status by monitoring that the timer is disabled
        if ((power.readChargeTermRegister() & 0b00110000) == 0) {
            wdtResetted = true;
            break;
        }
    }

    // Watchdog better have happened
    assertTrue(wdtResetted);
    // Check that charging is enabled after the watchdog reset
    cfg = System.getPowerConfiguration();
    assertFalse(cfg.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
    // PMIC charging may be enabled or disabled based on power manager state
    assertNotEqual((battery_state_t)System.batteryState(), BATTERY_STATE_FAULT);

    g_state = STATE15_MAGICK;
}

test(POWER_99_ResetState) {
    g_state = 0;
}
