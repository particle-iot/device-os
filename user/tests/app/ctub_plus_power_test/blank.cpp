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
#include "bma400.h"
#include "exrtc_hal.h"

SYSTEM_MODE(MANUAL);
SerialLogHandler l(115200, LOG_LEVEL_ALL);

constexpr uint16_t DEFAULT_INPUT_CURRENT_LIMIT = 900; // 900mA
constexpr uint16_t DEFAULT_INPUT_VOLTAGE_LIMIT = 3880; // 3.88V
constexpr uint16_t DEFAULT_CHARGE_CURRENT = 896; // 896mA
constexpr uint16_t DEFAULT_TERMINATION_VOLTAGE = 4112; // 4.112V
constexpr uint8_t DEFAULT_SOC_18_BIT_PRECISION = 18; // 18 is default, but may be 18 or 19 when a custom model is loaded

STARTUP(
    SystemPowerConfiguration config = {};
    config.powerSourceMinVoltage(DEFAULT_INPUT_VOLTAGE_LIMIT)
        .powerSourceMaxCurrent(DEFAULT_INPUT_CURRENT_LIMIT)
        .batteryChargeCurrent(DEFAULT_CHARGE_CURRENT)
        .batteryChargeVoltage(DEFAULT_TERMINATION_VOLTAGE)
        .socBitPrecision(DEFAULT_SOC_18_BIT_PRECISION)
        .auxiliaryPowerControlPin(PIN_INVALID) // No aux power control pin
        .interruptPin(LOW_BAT_UC) // Use the default: LOW_BAT_UC
        .feature(SystemPowerFeature::PMIC_DETECTION);
    System.setPowerConfiguration(config); // Set the power configuration
);

BMA400_INTF_RET_TYPE bmaReadFn(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr) {
    uint8_t addr = *((uint8_t*)intf_ptr);
    hal_i2c_begin_transmission(HAL_I2C_INTERFACE1, addr, nullptr);
    hal_i2c_write(HAL_I2C_INTERFACE1, static_cast<uint8_t>(reg_addr), nullptr);
    hal_i2c_end_transmission(HAL_I2C_INTERFACE1, false, nullptr);
    if (hal_i2c_request(HAL_I2C_INTERFACE1, addr, length, true, nullptr) != length) {
        return BMA400_E_COM_FAIL;
    }
    for (uint32_t i = 0; i < length; ++i) {
        reg_data[i] = hal_i2c_read(HAL_I2C_INTERFACE1, nullptr);
    }
    return BMA400_OK;
}

BMA400_INTF_RET_TYPE bmaWriteFn(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr) {
    uint8_t addr = *((uint8_t*)intf_ptr);
    hal_i2c_begin_transmission(HAL_I2C_INTERFACE1, addr, nullptr);
    hal_i2c_write(HAL_I2C_INTERFACE1, static_cast<uint8_t>(reg_addr), nullptr);
    for (size_t i = 0; i < length; i++) {
        hal_i2c_write(HAL_I2C_INTERFACE1, reg_data[i], nullptr);
    }
    hal_i2c_end_transmission(HAL_I2C_INTERFACE1, true, nullptr);
    return BMA400_OK;
}

void bmaDelayUsFn(uint32_t period, void *intf_ptr) {
    delayMicroseconds(period); // Use the delay function to wait for the specified period
}

/* executes once at startup */
void setup() {
    delay(5000); // Wait for the system to stabilize

    // Power manager should have initialized the I2C interface
    // Wire.begin();

    uint8_t bmaAddr = 0x14;
    bma400_dev dev = {};
    dev.intf = BMA400_I2C_INTF; // Use I2C interface
    dev.intf_ptr = &bmaAddr;
    dev.read = bmaReadFn; // Set the read function
    dev.write = bmaWriteFn; // Set the write function
    dev.delay_us = bmaDelayUsFn; // Set the delay function
    if (bma400_init(&dev)) {
        Log.error("BMA400 initialization failed");
    } else {
        Log.info("BMA400 initialized successfully: %02X", dev.chip_id);
        int8_t ret = bma400_set_power_mode(BMA400_MODE_NORMAL, &dev); // Set the power mode to normal
        if (ret != BMA400_OK) {
            Log.error("Failed to set BMA400 power mode: %d", ret);
        } else {
            Log.info("BMA400 power mode set to normal");
            bma400_sensor_conf conf = {};
            conf.type = BMA400_GEN1_INT;
            conf.param.gen_int.gen_int_thres = 200; // 8mg per LSB. 0 - 255
            conf.param.gen_int.gen_int_dur = 1;
            conf.param.gen_int.axes_sel = 0x07; // Enable all axes
            conf.param.gen_int.data_src = BMA400_DATA_SRC_ACC_FILT1;
            conf.param.gen_int.criterion_sel = BMA400_ACTIVITY_INT; // Activity interrupt
            conf.param.gen_int.evaluate_axes = BMA400_ANY_AXES_INT;
            conf.param.gen_int.ref_update = BMA400_UPDATE_EVERY_TIME; // Update reference values every time
            conf.param.gen_int.hysteresis = BMA400_HYST_24_MG;
            conf.param.gen_int.int_thres_ref_x = 100; // No reference threshold for x
            conf.param.gen_int.int_thres_ref_y = 100; // No reference threshold for y
            conf.param.gen_int.int_thres_ref_z = 100; // No reference threshold for z
            conf.param.gen_int.int_chan = BMA400_INT_CHANNEL_1; // Map to interrupt channel 1
            ret = bma400_set_sensor_conf(&conf, 1, &dev);
            if (ret != BMA400_OK) {
                Log.error("Failed to set BMA400 sensor configuration: %d", ret);
            } else {
                Log.info("BMA400 sensor configuration set successfully");
                bma400_device_conf devConfig = {};
                devConfig.type = BMA400_INT_PIN_CONF;
                devConfig.param.int_conf.int_chan = BMA400_INT_CHANNEL_1; // Map to interrupt channel 1
                devConfig.param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_0;
                ret = bma400_set_device_conf(&devConfig, 1, &dev);
                if (ret != BMA400_OK) {
                    Log.error("Failed to configure BMA400 interrupt: %d", ret);
                } else {
                    Log.info("BMA400 interrupt configured successfully");
                    bma400_int_enable enable = {};
                    enable.type = BMA400_GEN1_INT_EN;
                    enable.conf = BMA400_ENABLE; // Enable the generic interrupt 1
                    ret = bma400_enable_interrupt(&enable, 1, &dev);
                    if (ret != BMA400_OK) {
                        Log.error("Failed to enable BMA400 interrupt: %d", ret);
                    } else {
                        Log.info("BMA400 interrupt enabled");
                    }
                }
            }
        }
    }

    hal_exrtc_config_t config = {
        .version = HAL_EXRTC_CONFIG_VERSION,
        .size = sizeof(hal_exrtc_config_t),
        .wdi_pin = D7,
        .int_pin = A7,
        .i2c_if = HAL_I2C_INTERFACE1,
        .i2c_addr = 0x69,
        .osc_cal_xt = -45,
    };
    hal_exrtc_init(&config, nullptr);
    
    struct timeval tv = {};
    hal_exrtc_get_time(&tv, nullptr);
    Log.info("Current time: %ld", tv.tv_sec);

    tv.tv_sec = 1753916506;
    Log.info("Set time: %ld", tv.tv_sec);
    hal_exrtc_set_time(&tv, nullptr);

    delay(1000);
    hal_exrtc_sleep_timer(30 * 1000, nullptr);

    // hal_exrtc_enable_watchdog(5000, nullptr); // Enable watchdog with 5 seconds timeout
}

/* executes continuously after setup() runs */
void loop() {
    // hal_exrtc_feed_watchdog(nullptr); // Feed the watchdog to prevent reset

    delay(1000); // Sleep for 1 second
    struct timeval tv = {};
    hal_exrtc_get_time(&tv, nullptr);
    Log.info("Current time: %ld", tv.tv_sec);
}
