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

constexpr uint32_t MAGICK = 0xbeef;
retained uint32_t g_state;

void startup() {
    if (g_state != MAGICK) {
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        System.setPowerConfiguration(conf);
    }
}

STARTUP(startup());

} // anonymous

test(POWER_00_PoweredByVinAndBatteryPresent) {
    delay(1000);
    assertEqual(System.powerSource(), (int)POWER_SOURCE_VIN);
    assertTrue(System.batteryState() != BATTERY_STATE_UNKNOWN &&
            System.batteryState() != BATTERY_STATE_FAULT &&
            System.batteryState() != BATTERY_STATE_DISCONNECTED);
    assertTrue(System.batteryCharge() >= 0.0f);
}

test(POWER_01_ApplyingDefaultPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state == MAGICK) {
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

    PMIC power;
    assertEqual(power.getInputCurrentLimit(), particle::power::DEFAULT_INPUT_CURRENT_LIMIT);
    assertEqual(power.getInputVoltageLimit(), particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT);
    assertEqual(power.getChargeCurrentValue(), particle::power::DEFAULT_CHARGE_CURRENT);
    assertEqual(power.getChargeVoltageValue(), particle::power::DEFAULT_TERMINATION_VOLTAGE);
}

test(POWER_02_ApplyingCustomPowerManagementConfigurationInRuntimeWorksAsIntended) {
    if (g_state == MAGICK) {
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

    PMIC power;
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);
}

test(POWER_03_AppliedConfigurationIsPersisted) {
    if (g_state != MAGICK) {
        g_state = MAGICK;
        delay(5000);
        System.reset();
        return;
    }

    PMIC power;
    assertEqual(power.getInputCurrentLimit(), 500);
    assertEqual(power.getInputVoltageLimit(), 4280);
    assertEqual(power.getChargeCurrentValue(), 832);
    assertEqual(power.getChargeVoltageValue(), 4208);
}

test(POWER_04_ResetState) {
    g_state = 0x0000;
}

test(POWER_05_PmicDetectionFlagIsCompatibleWithOldDeviceOsVersions) {
    // Enable PMIC_DETECTION feature
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
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
