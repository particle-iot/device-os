/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "board_config.h"
#include "SparkFun_STUSB4500.h"
#include "stusb4500_register_map.h"

namespace {
    JSONValue getValue(const JSONValue& obj, const char* name) {
        JSONObjectIterator it(obj);
        while (it.next()) {
            if (it.name() == name) {
                return it.value();
            }
        }
        return JSONValue();
    }

    bool has(const JSONValue& obj, const char* name) {
        return getValue(obj, name).isValid();
    }
}

namespace particle {

BoardConfig::BoardConfig()
        : replyWriter_(nullptr, 0) {
}

BoardConfig::~BoardConfig() {
}

BoardConfig* BoardConfig::instance() {
    static BoardConfig config;
    return &config;
}

bool BoardConfig::process(JSONValue config) {
    memset(replyBuffer_, 0x00, sizeof(replyBuffer_));
    replyWriter_ = JSONBufferWriter((char*)replyBuffer_, sizeof(replyBuffer_));
    if (has(config, "IDENTIFY_BASE_BOARD")) {
        BoardConfig::instance()->configForBaseBoard();
        return true;
    }
    return false;
}

char* BoardConfig::reply() {
    return replyWriter_.buffer();
}

size_t BoardConfig::replySize() {
    return replyWriter_.dataSize();
}

void BoardConfig::configForBaseBoard() {
    Log.info("Identifying the base board...");

    bool isMuon = true;
    SCOPE_GUARD({
        replyWriter_.beginObject();
        if (isMuon) {
            replyWriter_.name("board").value("Particle Muon");
        } else {
            replyWriter_.name("board").value("Generic Module");
        }
        replyWriter_.endObject();
    });

    constexpr uint8_t addrs[] = {
        0x28,   // STUSB4500 USB PD chip
        0x61,   // KG200Z Lora module
        0x69,   // AM18x5 RTC
        0x48,   // TMP112A temperature sensor
        0x6B,   // BQ24195 PMIC
        0x36    // MAX17043 fuel gauge
    };
    Wire.begin();
    for (uint8_t i = 0; i < sizeof(addrs); i++) {
        if (!detectI2cSlave(addrs[i])) {
            isMuon = false;
            Log.info("Generic module detected");
            return;
        }
    }
    Log.info("Muon board detected");

    // System power manager
    Log.info("Set system power configuration");
    SystemPowerConfiguration powerConfig = System.getPowerConfiguration();
    powerConfig.auxPowerControlPin(auxPwrCtrlPin_)
               .intPin(pmicIntPin_);
    System.setPowerConfiguration(powerConfig);

    // Etehrnet
    Log.info("Set Ethernet configuration");
    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;
    remap.cs_pin = ethernetCsPin_;
    remap.reset_pin = ethernetResetPin_;
    remap.int_pin = ethernetIntPin_;
    if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);
    System.enableFeature(FEATURE_ETHERNET_DETECTION);

    // USB PD
    Log.info("Set USB PD configuration");
    STUSB4500 usb;
    if (usb.begin()) {
        usb.setPdoNumber(3);
        /* PDO1:
            - Voltage fixed at 5V
            - Current value for PDO1 0-5A, if 0 used, FLEX_I value is used
            - Under Voltage Lock Out (setUnderVoltageLimit) fixed at 3.3V
            - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
        */
        usb.setCurrent(1, 3); // 5V3A
        usb.setUpperVoltageLimit(1, 20);
        /* PDO2:
            - Voltage 5-20V
            - Current value for PDO2 0-5A, if 0 used, FLEX_I value is used
            - Under Voltage Lock Out (setUnderVoltageLimit) 5-20%
            - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
        */
        usb.setVoltage(2, 9.0);
        usb.setCurrent(2, 1.5); // 9V1.5A
        usb.setLowerVoltageLimit(2, 20);
        usb.setUpperVoltageLimit(2, 20);
        /* PDO3:
            - Voltage 5-20V
            - Current value for PDO3 0-5A, if 0 used, FLEX_I value is used
            - Under Voltage Lock Out (setUnderVoltageLimit) 5-20%
            - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
        */
        usb.setVoltage(3, 12.0);
        usb.setCurrent(3, 1); // 12V1A
        usb.setLowerVoltageLimit(3, 20);
        usb.setUpperVoltageLimit(3, 20);
        // USB 2.0 or 3.x data communication capability by sink system
        usb.setUsbCommCapable(true);
        /* Selects GPIO pin configuration
            0 - SW_CTRL_GPIO
            1 - ERROR_RECOVERY
            2 - DEBUG
            3 - SINK_POWER
        */
        usb.setGpioCtrl(3);
        // Selects VBUS_EN_SNK pin configuration
        usb.setPowerAbove5vOnly(false);
        // Write the new settings to the NVM
        usb.write();
    }

    Log.info("Device needs reset to apply the settings");
}

bool BoardConfig::detectI2cSlave(uint8_t addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
        return true;
    }
    return false;
}

} // particle
