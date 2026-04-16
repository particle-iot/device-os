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

#ifdef ENABLE_MUON_DETECTION

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
    if (has(config, "BASE_BOARD_DETECT")) {
        detectBaseBoard();
        return true;
    } else if (has(config, "CONFIGURE_MODULE_BOARD")) {
        auto value = getValue(config, "CONFIGURE_MODULE_BOARD");
        configureBaseBoard(value);
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

void BoardConfig::detectBaseBoard() {
    constexpr std::pair<I2cDevice, uint8_t> addrs[] = {
        {STUSB4500, 0x28},
        {AM1805, 0x69},
        {TMP112A, 0x48},
        {BQ24195, 0x6B},
        {MAX17043, 0x36},
        {ATSHA204A, 0x64},
        {LORA, 0x61},  // Not used for now
    };
    constexpr uint16_t essentialI2cDevices = STUSB4500 | AM1805 | TMP112A | BQ24195 | MAX17043;
    constexpr uint16_t muonI2cDevices = essentialI2cDevices;
    constexpr uint16_t mhatI2cDevices = essentialI2cDevices | ATSHA204A;

    Wire.lock();
    Wire.begin();
    uint16_t i2cDeviceMask = 0;
    for (uint8_t i = 0; i < sizeof(addrs) / sizeof(addrs[0]); i++) {
        Wire.beginTransmission(addrs[i].second);
        if (Wire.endTransmission() == 0) {
            i2cDeviceMask |= addrs[i].first;
        }
    }
    Wire.unlock();

    replyWriter_.beginObject();
    if (i2cDeviceMask == muonI2cDevices) {
        replyWriter_.name("board").value("muon");
    } else if (i2cDeviceMask == mhatI2cDevices) {
        replyWriter_.name("board").value("m-hat");
    } else {
        replyWriter_.name("board").value("none");
    }
    replyWriter_.endObject();
}

void BoardConfig::configureBaseBoard(JSONValue value) {
    int ret = SYSTEM_ERROR_INVALID_ARGUMENT;
    ret = configure(static_cast<String>(value.toString()));
    replyWriter_.beginObject();
    replyWriter_.name("status").value(ret);
    replyWriter_.endObject();
}

int BoardConfig::configure(String baseBoard) {
    Log.info("Set system power configuration");
    SystemPowerConfiguration powerConfig = System.getPowerConfiguration();
    if (baseBoard == "muon" || baseBoard == "m-hat") {
        powerConfig.auxiliaryPowerControlPin(D7, HIGH).interruptPin(A7).feature(SystemPowerFeature::PMIC_DETECTION);
    } else {
        powerConfig.auxiliaryPowerControlPin(PIN_INVALID).interruptPin(LOW_BAT_UC);
    }
    CHECK(System.setPowerConfiguration(powerConfig));

    Log.info("Set Ethernet configuration");
    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;
    if (baseBoard == "muon") {
        System.enableFeature(FEATURE_ETHERNET_DETECTION);
        remap.cs_pin = A3;
        remap.reset_pin = PIN_INVALID;
        remap.int_pin = A4;
    } else {
        System.disableFeature(FEATURE_ETHERNET_DETECTION);
        remap.cs_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT;
        remap.reset_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT;
        remap.int_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT;
    }
    CHECK(if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr));

    Log.info("Device need reset to apply new configurations");
    return SYSTEM_ERROR_NONE;
}

} // particle

#endif // ENABLE_MUON_DETECTION
