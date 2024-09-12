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
    bool isMuon = detectI2cSlaves();
    replyWriter_.beginObject();
    if (isMuon) {
        replyWriter_.name("board").value("muon");
    } else {
        replyWriter_.name("board").value("none");
    }
    replyWriter_.endObject();
}

void BoardConfig::configureBaseBoard(JSONValue value) {
    int ret = SYSTEM_ERROR_INVALID_ARGUMENT;
    if (value.toString() == "muon") {
        ret = configure(true);
    } else if (value.toString() == "none") {
        ret = configure(false);
    }
    replyWriter_.beginObject();
    replyWriter_.name("status").value(ret);
    replyWriter_.endObject();
}

bool BoardConfig::detectI2cSlaves() {
    constexpr uint8_t addrs[] = {
        0x28,   // STUSB4500 USB PD chip
        0x69,   // AM18x5 RTC
        0x48,   // TMP112A temperature sensor
        0x6B,   // BQ24195 PMIC
        0x36    // MAX17043 fuel gauge
    };
    Wire.begin();
    for (uint8_t i = 0; i < sizeof(addrs); i++) {
        Wire.beginTransmission(addrs[i]);
        if (Wire.endTransmission() != 0) {
            return false;
        }
    }
    return true;
}

int BoardConfig::configure(bool muon) {
    Log.info("Set system power configuration");
    SystemPowerConfiguration powerConfig = System.getPowerConfiguration();
    if (muon) {
        powerConfig.auxiliaryPowerControlPin(D7).interruptPin(A7);
    } else {
        powerConfig.auxiliaryPowerControlPin(PIN_INVALID).interruptPin(LOW_BAT_UC);
    }
    CHECK(System.setPowerConfiguration(powerConfig));

    Log.info("Set Ethernet configuration");
    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;
    if (muon) {
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
