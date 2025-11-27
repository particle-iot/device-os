#include "Particle.h"
#include "bma400.h"

SYSTEM_MODE(MANUAL);

Serial2LogHandler l(115200, LOG_LEVEL_ALL);

bma400_dev bmaDev = {};

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

void bmeInit() {
    uint8_t bmaAddr = 0x14;
    bmaDev.intf = BMA400_I2C_INTF; // Use I2C interface
    bmaDev.intf_ptr = &bmaAddr;
    bmaDev.read = bmaReadFn; // Set the read function
    bmaDev.write = bmaWriteFn; // Set the write function
    bmaDev.delay_us = bmaDelayUsFn; // Set the delay function
    if (bma400_init(&bmaDev)) {
        Serial2.println("ERROR: BMA400 not found");
    } else {
        Serial2.printlnf("BMA400 found: %02X", bmaDev.chip_id);

        // struct bma400_int_enable int_select;
        // int_select.type = BMA400_GEN1_INT_EN;
        // bma400_get_interrupts_enabled(&int_select, 1, &bmaDev);
        // if (int_select.conf == BMA400_ENABLE) {
        //     Serial2.println("BMA400 Generic interrupt 1 is already enabled");
        //     return;
        // }

        int8_t ret = bma400_set_power_mode(BMA400_MODE_NORMAL, &bmaDev); // Set the power mode to normal
        if (ret != BMA400_OK) {
            Serial2.printlnf("Failed to set BMA400 power mode: %d", ret);
        } else {
            Serial2.println("BMA400 power mode set to normal");
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
            ret = bma400_set_sensor_conf(&conf, 1, &bmaDev);
            if (ret != BMA400_OK) {
                Serial2.printlnf("Failed to set BMA400 sensor configuration: %d", ret);
            } else {
                Serial2.println("BMA400 sensor configuration set successfully");
                bma400_device_conf devConfig = {};
                devConfig.type = BMA400_INT_PIN_CONF;
                devConfig.param.int_conf.int_chan = BMA400_INT_CHANNEL_1; // Map to interrupt channel 1
                devConfig.param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_0;
                ret = bma400_set_device_conf(&devConfig, 1, &bmaDev);
                if (ret != BMA400_OK) {
                    Serial2.printlnf("Failed to configure BMA400 interrupt: %d", ret);
                } else {
                    Serial2.println("BMA400 interrupt configured successfully");
                    bma400_int_enable enable[2] = {};
                    enable[0].type = BMA400_GEN1_INT_EN;
                    enable[0].conf = BMA400_ENABLE; // Enable the generic interrupt 1
                    enable[1].type = BMA400_LATCH_INT_EN;
                    enable[1].conf = BMA400_ENABLE;
                    ret = bma400_enable_interrupt(enable, 2, &bmaDev);
                    if (ret != BMA400_OK) {
                        Serial2.printlnf("Failed to enable BMA400 interrupt: %d", ret);
                    } else {
                        Serial2.println("BMA400 interrupt enabled");
                    }
                }
            }
        }
    }
}

void PRE_STARTUP() {
    Serial2.begin(115200);
    Serial2.println("PRE_STARTUP called");

    SystemExternalRtcConfiguration config = {};
    config.defaultRtc(true)
          .watchdogInputPin(A5)
          .interruptPin(A7)
          .i2cInterface(HAL_I2C_INTERFACE1)
          .rcFallbackOnXtalFailure(true)
          .rcOnBatteryPowered(true)
          .oscSource(Am18x5Oscillator::EXTERNAL_CRYSTAL)
          .xtalCalibrationValue(-45);
    if (System.setExternalRtcConfiguration(config) == SYSTEM_ERROR_NONE) {
        Serial2.println("External RTC configured successfully");
    } else {
        Serial2.println("Failed to configure External RTC");
    }

    if (System.getExternalRtcConfiguration(config) != SYSTEM_ERROR_NONE) {
        System.enableFeature(FEATURE_EXRTC_DETECTION);
        Serial2.println("Enabled AM18x5 detection, resetting system...");
        delay(1s);
        System.reset();
    }

    Serial2.printlnf("Up time 1: %u ms", millis());
    delay(1s);
    Serial2.printlnf("Up time 2: %u ms", millis());

    Serial2.printlnf("Unix time: %u s, source: %d", Time.now(), Time.getTimeSource());

    SystemPowerConfiguration conf = {};
    conf = System.getPowerConfiguration();
    Serial2.printlnf("Min voltage: %u mV", conf.powerSourceMinVoltage());

    bmeInit();

    uint16_t intStatus = 0;
    bma400_get_interrupt_status(&intStatus, &bmaDev);

    if (intStatus & 0x0004) {
        Serial2.printlnf("BMA400 Activity interrupt triggered: %04x", intStatus);
        delay(100);
        SystemExternalRtcSleepConfiguration sleepConf = {};
        sleepConf.exti(Am18x5ExtiPolarity::FALLING);
        System.powerGatedByExternalRtc(sleepConf);
    } else {
        Serial2.printlnf("No BMA400 Activity interrupt: %04x", intStatus);
    }
}

void setup() {
    Serial2.println("setup called");

    delay(2s);

    SystemExternalRtcSleepConfiguration sleepConf = {};
    // sleepConf.duration(5s);
    sleepConf.exti(Am18x5ExtiPolarity::FALLING);
    System.powerGatedByExternalRtc(sleepConf);
}

void loop() {

}
