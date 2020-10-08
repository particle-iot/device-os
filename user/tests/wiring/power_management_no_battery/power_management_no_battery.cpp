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




// ========================================================
// USAGE:
//
// Run this test on an R410 based device, with no battery and USB only.
// Monitor logging on TX output
// The test runs itself, no need to reconnect to serial.
//
// uncomment AUTOMATIC_TEST_LOOP for repeated warm boot testing in a loop.
//
// ========================================================

#ifndef AUTOMATIC_TEST_LOOP
#define AUTOMATIC_TEST_LOOP 1
#endif

// TODO: Clean up all of the repeated code with some helper macro/functions.

#include "Particle.h"
#include "dct.h"
#include "logging.h"

#if !HAL_PLATFORM_POWER_MANAGEMENT
#error "Unsupported platform"
#endif // !HAL_PLATFORM_POWER_MANAGEMENT

bool passing = true;
#define CHECK_SUCCESS(x) { if (!(x)) { passing = false; break; } }

Serial1LogHandler log1Serial(115200, LOG_LEVEL_ALL);
// Serial1LogHandler log1Serial(115200, LOG_LEVEL_NONE, {
//     // { "net.ppp.client", LOG_LEVEL_ALL },
//     // { "ncp.at", LOG_LEVEL_ALL },
//     { "app", LOG_LEVEL_ALL }
// });

SYSTEM_MODE(SEMI_AUTOMATIC);

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

constexpr uint32_t STATE0_MAGICK = 0xc00f;
constexpr uint32_t STATE1_MAGICK = 0xbeef;
constexpr uint32_t STATE2_MAGICK = 0xdeed;
constexpr uint32_t STATE3_MAGICK = 0xc0fe;
constexpr uint32_t STATE4_MAGICK = 0x1337;
retained uint32_t g_state;

bool inState() {
    switch (g_state) {
        case STATE0_MAGICK:
        case STATE1_MAGICK:
        case STATE2_MAGICK:
        case STATE3_MAGICK:
        case STATE4_MAGICK: {
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
        Log.info("POWER_000_DefaultSettingsDuringSetup Passed, resetting...");
        delay(1000);
        System.reset();
    }
}

} // anonymous

bool test_POWER_00_PoweredByVinAndBatteryPresent() {
    if (g_state != STATE0_MAGICK) {
        Log.info("%s Skipped", __func__);
        return true;
    }

    delay(1000);
    if (System.powerSource() != (int)POWER_SOURCE_VIN &&
            System.powerSource() != (int)POWER_SOURCE_USB_HOST) {
        Log.error("%s 1 Failed", __func__);
        return false;
    }
    if (System.batteryState() == BATTERY_STATE_UNKNOWN ||
            System.batteryState() == BATTERY_STATE_FAULT) {
        Log.error("%s 2 Failed", __func__);
        return false;
    }

    PMIC power(true);
    bool input_current_failing = true;
    int count = 0;
    if (System.powerSource() == (int)POWER_SOURCE_VIN) {
        do {
            // power.dumpRegisters("POWER_00");
            input_current_failing = (power.getInputCurrentLimit() != particle::power::DEFAULT_INPUT_CURRENT_LIMIT) ? true : false;
            delay(200);
            count++;
        } while (input_current_failing && count < 150);
        if (power.getInputCurrentLimit() != particle::power::DEFAULT_INPUT_CURRENT_LIMIT) {
            Log.error("%s 3 Failed (%d)", __func__, power.getInputCurrentLimit());
            return false;
        }
    } else {
        do {
            // power.dumpRegisters("POWER_00");
            input_current_failing = (power.getInputCurrentLimit() != 500) ? true : false;
            delay(200);
            count++;
        } while (input_current_failing && count < 150);
        if (power.getInputCurrentLimit() != 500) {
            Log.error("%s 4 Failed (%d)", __func__, power.getInputCurrentLimit());
            return false;
        }
    }
    bool input_voltage_failing = true;
    count = 0;
    do {
        // power.dumpRegisters("POWER_00");
        input_voltage_failing = (power.getInputVoltageLimit() != particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT) ? true : false;
        delay(200);
        count++;
    } while (input_voltage_failing && count < 150);
    if (power.getInputVoltageLimit() != particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT) {
        Log.error("%s 5 Failed (%d)", __func__, power.getInputVoltageLimit());
        return false;
    }
    if (power.getChargeCurrentValue() != particle::power::DEFAULT_CHARGE_CURRENT) {
        Log.error("%s 6 Failed", __func__);
        return false;
    }
    if (power.getChargeVoltageValue() != particle::power::DEFAULT_TERMINATION_VOLTAGE) {
        Log.error("%s 7 Failed", __func__);
        return false;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_01_ApplyingDefaultPowerManagementConfigurationInRuntimeWorksAsIntended() {
    if (g_state != STATE0_MAGICK) {
        Log.info("%s Skipped", __func__);
        return true;
    }

    // Apply default configuration
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
        Log.error("%s 1 Failed", __func__);
        return false;
    }
    delay(3000);

    PMIC power(true);
    bool input_current_failing = true;
    int count = 0;
    if (System.powerSource() == (int)POWER_SOURCE_VIN) {
        do {
            // power.dumpRegisters("POWER_01");
            input_current_failing = (power.getInputCurrentLimit() != particle::power::DEFAULT_INPUT_CURRENT_LIMIT) ? true : false;
            delay(200);
            count++;
        } while (input_current_failing && count < 150);
        if (power.getInputCurrentLimit() != particle::power::DEFAULT_INPUT_CURRENT_LIMIT) {
            Log.error("%s 2 Failed (%d)", __func__, power.getInputCurrentLimit());
            return false;
        }
    } else {
        do {
            // power.dumpRegisters("POWER_01");
            input_current_failing = (power.getInputCurrentLimit() != 500) ? true : false;
            delay(200);
            count++;
        } while (input_current_failing && count < 150);
        if (power.getInputCurrentLimit() != 500) {
            Log.error("%s 3 Failed (%d)", __func__, power.getInputCurrentLimit());
            return false;
        }
    }
    bool input_voltage_failing = true;
    count = 0;
    do {
        // power.dumpRegisters("POWER_01");
        input_voltage_failing = (power.getInputVoltageLimit() != particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT) ? true : false;
        delay(200);
        count++;
    } while (input_voltage_failing && count < 150);
    if (power.getInputVoltageLimit() != particle::power::DEFAULT_INPUT_VOLTAGE_LIMIT) {
        Log.error("%s 4 Failed (%d)", __func__, power.getInputVoltageLimit());
        return false;
    }
    if (power.getChargeCurrentValue() != particle::power::DEFAULT_CHARGE_CURRENT) {
        Log.error("%s 5 Failed", __func__);
        return false;
    }
    if (power.getChargeVoltageValue() != particle::power::DEFAULT_TERMINATION_VOLTAGE) {
        Log.error("%s 6 Failed", __func__);
        return false;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_02_ApplyingCustomPowerManagementConfigurationInRuntimeWorksAsIntended() {
    if (g_state != STATE0_MAGICK) {
        Log.info("%s Skipped", __func__);
        return true;
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
    if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
        Log.error("%s 1 Failed", __func__);
        return false;
    }
    delay(1000);

    PMIC power(true);
    //
    // bool dpdm_running = true;
    // int count = 0;
    // do {
    //     power.dumpRegisters("POWER_02");
    //     dpdm_running = (power.readRegister(MISC_CONTROL_REGISTER) & 0x80) ? true : false;
    //     delay(200);
    //     count++;
    // } while (dpdm_running && count < 150);
    //
    // Alternatively below, we just re-read the setting if it's failing.
    //

    bool input_voltage_failing = true;
    int count = 0;
    if (System.powerSource() == (int)POWER_SOURCE_VIN) {
        do {
            // power.dumpRegisters("POWER_02");
            input_voltage_failing = (power.getInputVoltageLimit() != 4280) ? true : false;
            delay(200);
            count++;
        } while (input_voltage_failing && count < 150);
        if (power.getInputVoltageLimit() != 4280) {
            Log.error("%s 1 Failed (%d)", __func__, power.getInputVoltageLimit());
            return false;
        }
    } else {
        do {
            // power.dumpRegisters("POWER_02");
            input_voltage_failing = (power.getInputVoltageLimit() != 3880) ? true : false;
            delay(200);
            count++;
        } while (input_voltage_failing && count < 150);
        if (power.getInputVoltageLimit() != 3880) {
            Log.error("%s 2 Failed (%d)", __func__, power.getInputVoltageLimit());
            return false;
        }
    }
    bool input_current_failing = true;
    count = 0;
    do {
        // power.dumpRegisters("POWER_02");
        input_current_failing = (power.getInputCurrentLimit() != 500) ? true : false;
        delay(200);
        count++;
    } while (input_current_failing && count < 150);
    if (power.getInputCurrentLimit() != 500) {
        Log.error("%s 4 Failed (%d)", __func__, power.getInputCurrentLimit());
        return false;
    }
    if (power.getChargeCurrentValue() != 832) {
        Log.error("%s 5 Failed", __func__);
        return false;
    }
    if (power.getChargeVoltageValue() != 4208) {
        Log.error("%s 6 Failed", __func__);
        return false;
    }

    g_state = STATE1_MAGICK;
    Log.info("%s Passed, resetting...", __func__);
    delay(5000);
    System.reset();
    return true;
}

bool test_POWER_03_AppliedConfigurationIsPersisted() {
    if (g_state != STATE1_MAGICK) {
        Log.info("%s Skipped", __func__);
        return true;
    }

    PMIC power(true);
    bool input_voltage_failing = true;
    int count = 0;
    if (System.powerSource() == (int)POWER_SOURCE_VIN) {
        do {
            // power.dumpRegisters("POWER_03");
            input_voltage_failing = (power.getInputVoltageLimit() != 4280) ? true : false;
            delay(200);
            count++;
        } while (input_voltage_failing && count < 150);
        if (power.getInputVoltageLimit() != 4280) {
            Log.error("%s 1 Failed (%d)", __func__, power.getInputVoltageLimit());
            return false;
        }
    } else {
        do {
            // power.dumpRegisters("POWER_03");
            input_voltage_failing = (power.getInputVoltageLimit() != 3880) ? true : false;
            delay(200);
            count++;
        } while (input_voltage_failing && count < 150);
        if (power.getInputVoltageLimit() != 3880) {
            Log.error("%s 2 Failed (%d)", __func__, power.getInputVoltageLimit());
            return false;
        }
    }
    bool input_current_failing = true;
    count = 0;
    do {
        // power.dumpRegisters("POWER_03");
        input_current_failing = (power.getInputCurrentLimit() != 500) ? true : false;
        delay(200);
        count++;
    } while (input_current_failing && count < 150);
    if (power.getInputCurrentLimit() != 500) {
        Log.error("%s 3 Failed (%d)", __func__, power.getInputCurrentLimit());
        return false;
    }
    if (power.getChargeCurrentValue() != 832) {
        Log.error("%s 4 Failed", __func__);
        return false;
    }
    if (power.getChargeVoltageValue() != 4208) {
        Log.error("%s 5 Failed", __func__);
        return false;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_04_PmicDetectionFlagIsCompatibleWithOldDeviceOsVersions() {
    if (g_state != STATE1_MAGICK) {
        Log.info("%s Skipped", __func__);
        return true;
    }

    // Enable PMIC_DETECTION feature
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
        Log.error("%s 1 Failed", __func__);
        return false;
    }
    delay(1000);

    // Feature enabled
    uint8_t v;
    if (dct_read_app_data_copy(DCT_POWER_CONFIG_OFFSET, &v, sizeof(v)) != 0) {
        Log.error("%s 2 Failed", __func__);
        return false;
    }
    if ((v & 0x01) != 0x00) {
        Log.error("%s 3 Failed", __func__);
        return false;
    }

    // Apply default configuration
    conf = SystemPowerConfiguration();
    if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
        Log.error("%s 4 Failed", __func__);
        return false;
    }
    delay(1000);

    // Feature disabled
    if (dct_read_app_data_copy(DCT_POWER_CONFIG_OFFSET, &v, sizeof(v)) != 0) {
        Log.error("%s 5 Failed", __func__);
        return false;
    }
    if ((v & 0x01) != 0x01) {
        Log.error("%s 6 Failed", __func__);
        return false;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_05_DisableFeatureFlag() {
    if (g_state == STATE1_MAGICK) {
        // Set disabled feature flag
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::DISABLE);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        // NOTE: this setting will require a reboot to be applied
        if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
            Log.error("%s 1 Failed", __func__);
        }
        g_state = STATE2_MAGICK;
        Log.info("%s Passed, resetting...", __func__);
        delay(5000);
        System.reset();
    } else if (g_state == STATE2_MAGICK) {
        g_state = STATE3_MAGICK;
        if (System.batteryState() != (int)BATTERY_STATE_UNKNOWN) {
            Log.error("%s 2 Failed", __func__);
            return false;
        }
        if (System.powerSource() != (int)POWER_SOURCE_UNKNOWN) {
            Log.error("%s 3 Failed", __func__);
            return false;
        }
    } else {
        Log.info("%s Skipped", __func__);
        return true;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_06_ReEnableFeatureFlag() {
    if (g_state == STATE3_MAGICK) {
        // Set disabled feature flag
        SystemPowerConfiguration conf;
        conf.feature(SystemPowerFeature::PMIC_DETECTION);
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        conf.feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        // NOTE: this setting will require a reboot to be applied
        if (System.setPowerConfiguration(conf) != (int)SYSTEM_ERROR_NONE) {
            Log.error("%s 1 Failed", __func__);
        }
        g_state = STATE4_MAGICK;
        Log.info("%s Passed, resetting...", __func__);
        delay(5000);
        System.reset();
    } else {
        Log.info("%s Skipped", __func__);
        return true;
    }
    Log.info("%s Passed", __func__);
    return true;
}

bool test_POWER_07_ResetState() {
    g_state = 0;
    return true;
}

void setup() {
    // delay(100);
    LOG_PRINT_C(INFO, "app", "\r\n");
    if (!inState()) {
        PMIC power;
        power.begin();
        Log.trace("======================================");
        Log.trace("Current PMIC settings:");
        Log.trace("VIN Vmin: %u", power.getInputVoltageLimit());
        Log.trace("VIN Imax: %u", power.getInputCurrentLimit());
        Log.trace("Ichg: %u", power.getChargeCurrentValue());
        Log.trace("Vterm: %u", power.getChargeVoltageValue());
        int powerSource = System.powerSource();
        int batteryState = System.batteryState();
        float batterySoc = System.batteryCharge();
        constexpr char const* batteryStates[] = {
            "unknown", "not charging", "charging",
            "charged", "discharging", "fault", "disconnected"
        };
        constexpr char const* powerSources[] = {
            "unknown", "vin", "usb host", "usb adapter",
            "usb otg", "battery"
        };
        Log.trace("Power source: %s", powerSources[std::max(0, powerSource)]);
        Log.trace("Battery state: %s", batteryStates[std::max(0, batteryState)]);
        Log.trace("Battery charge: %f\r\n\r\n", batterySoc);

        bool input_voltage_failing = true;
        int count = 0;
        if (System.powerSource() == (int)POWER_SOURCE_VIN) {
            // do {
            //     // power.dumpRegisters("POWER_000");
            //     input_voltage_failing = (power.getInputVoltageLimit() != 4280) ? true : false;
            //     delay(200);
            //     count++;
            // } while (input_voltage_failing && count < 5);
            // if (power.getInputVoltageLimit() != 4280) {
            //     Log.error("%s 1 Failed (%d)", __func__, power.getInputVoltageLimit());
            //     passing = false;
            // }
        } else {
            do {
                // power.dumpRegisters("POWER_000");
                input_voltage_failing = (power.getInputVoltageLimit() != 3880) ? true : false;
                delay(200);
                count++;
            } while (input_voltage_failing && count < 5);
            if (power.getInputVoltageLimit() != 3880) {
                Log.error("%s 2 Failed (%d)", __func__, power.getInputVoltageLimit());
                passing = false;
            }
        }
        bool input_current_failing = true;
        count = 0;
        do {
            // power.dumpRegisters("SETUP");
            input_current_failing = (power.getInputCurrentLimit() < 500) ? true : false;
            delay(200);
            count++;
        } while (input_current_failing && count < 5);
        if (power.getInputCurrentLimit() < 500) {
            Log.error("%s 3 Failed (%d)", __func__, power.getInputCurrentLimit());
            passing = false;
        }
        if (power.getChargeCurrentValue() != 896) {
            Log.error("%s 4 Failed", __func__);
            passing = false;
        }
        if (power.getChargeVoltageValue() != 4112) {
            Log.error("%s 5 Failed", __func__);
            passing = false;
        }
    }

    if (passing) {
        startup();
    }

    do {
    CHECK_SUCCESS( test_POWER_00_PoweredByVinAndBatteryPresent() );
    CHECK_SUCCESS( test_POWER_01_ApplyingDefaultPowerManagementConfigurationInRuntimeWorksAsIntended() );
    CHECK_SUCCESS( test_POWER_02_ApplyingCustomPowerManagementConfigurationInRuntimeWorksAsIntended() );
    CHECK_SUCCESS( test_POWER_03_AppliedConfigurationIsPersisted() );
    CHECK_SUCCESS( test_POWER_04_PmicDetectionFlagIsCompatibleWithOldDeviceOsVersions() );
    CHECK_SUCCESS( test_POWER_05_DisableFeatureFlag() );
    CHECK_SUCCESS( test_POWER_06_ReEnableFeatureFlag() );
    CHECK_SUCCESS( test_POWER_07_ResetState() );
    } while (false);

    RGB.control(true);
    if (passing) {
        RGB.color(0,255,0); // GREEN
        Log.info("\r\n\r\nAll tests passed!\r\n");
    } else {
        RGB.color(255,0,0); // RED
        Log.info("\r\n\r\nTests failed!!!\r\n");
    }

#ifdef AUTOMATIC_TEST_LOOP
    delay(1000);
    System.reset();
#endif
}
