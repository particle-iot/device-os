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
retained uint32_t g_state;

bool inState() {
    switch (g_state) {
        case STATE0_MAGICK:
        case STATE1_MAGICK:
        case STATE2_MAGICK:
        case STATE3_MAGICK: {
            return true;
        }
    }

    return false;
}

void startup() {
    if (!inState()) {
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        System.setPowerConfiguration(conf);
        g_state = STATE0_MAGICK;
        System.reset();
    }
}

STARTUP(startup());

} // anonymous

test(POWER_00_PoweredByVinAndBatteryPresent) {
    if (g_state != STATE0_MAGICK) {
        skip();
        return;
    }

    delay(1000);
    assertEqual(System.powerSource(), (int)POWER_SOURCE_VIN);
    assertTrue(System.batteryState() != BATTERY_STATE_UNKNOWN &&
            System.batteryState() != BATTERY_STATE_FAULT &&
            System.batteryState() != BATTERY_STATE_DISCONNECTED);
    assertTrue(System.batteryCharge() >= 0.0f);
}

test(POWER_01_ApplyingDefaultPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state != STATE0_MAGICK) {
        skip();
        return;
    }

    // Apply default configuration
    SystemPowerConfiguration conf;
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(power.getInputVoltageLimit(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(power.getChargeCurrentValue(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(power.getChargeVoltageValue(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
}

test(POWER_02_ApplyingCustomPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state != STATE0_MAGICK) {
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
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
    delay(1000);

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);

    g_state = STATE1_MAGICK;
    delay(5000);
    System.reset();
}

test(POWER_03_AppliedConfigurationIsPersisted) {
    if (g_state != STATE1_MAGICK) {
        skip();
        return;
    }

    PMIC power(true);
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);
}

test(POWER_04_PmicDetectionFlagIsCompatibleWithOldDeviceOsVersions) {
    if (g_state != STATE1_MAGICK) {
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

test(POWER_05_DisableFeatureFlag) {
    if (g_state == STATE1_MAGICK) {
        // Set disabled feature flag
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::DISABLE);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        // NOTE: this setting will require a reboot to be applied
        assertEqual(System.setPowerConfiguration(conf), (int)SYSTEM_ERROR_NONE);
        g_state = STATE2_MAGICK;
        delay(5000);
        System.reset();
    } else if (g_state == STATE2_MAGICK) {
        g_state = STATE3_MAGICK;
        assertEqual(System.batteryState(), (int)BATTERY_STATE_UNKNOWN);
        assertEqual(System.powerSource(), (int)POWER_SOURCE_UNKNOWN);
    } else {
        skip();
        return;
    }
}

test(POWER_06_ResetState) {
    g_state = 0;
}
