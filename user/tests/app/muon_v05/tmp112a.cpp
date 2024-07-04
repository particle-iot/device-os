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

#include "tmp112a.h"

using namespace particle;

Tmp112a::Tmp112a(TwoWire* wire)
        : initialized_(false),
          address_(TMP112A_SLAVE_ADDR),
          wire_(wire) {
}

Tmp112a::~Tmp112a() {

}

Tmp112a& Tmp112a::getInstance() {
    static Tmp112a tmp(&Wire);
    return tmp;
}

int Tmp112a::begin() {
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);

    wire_->begin();

    // FIXME: I/O glitch when there is a pull-up on a pin that is configured as OUTPUT

    Mcp23s17::getInstance().setPinMode(intPin_.first, intPin_.second, INPUT_PULLUP);

    wire_->beginTransmission(address_);
    wire_->write(CONFIG_REG);
    int ret = wire_->endTransmission(false);
    if (ret) {
        Log.info("[Tmp112a] endTransmission: %d", ret);
        return SYSTEM_ERROR_INTERNAL;
    }
    ret = wire_->requestFrom(address_, 2);
    if (ret != 2) {
        Log.info("[Tmp112a] requestFrom: %d", ret);
        return SYSTEM_ERROR_INTERNAL;
    }
    uint16_t val = (wire_->read() << 8) & 0x7FFF;
    val |= wire_->read();
    Log.info("[Tmp112a] Config: 0x%04x, expected: 0x%04x", val, DEFAULT_CONFIG);
    if (val != DEFAULT_CONFIG) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    initialized_ = true;
    return SYSTEM_ERROR_NONE;
}

int Tmp112a::end() {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int Tmp112a::getTemperature(float* temp) {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    wire_->beginTransmission(address_);
    wire_->write(TEMP_REG);
    wire_->endTransmission(false);
    wire_->requestFrom(address_, 2);
    uint16_t val = wire_->read() << 8;
    val |= wire_->read();
    val >>= 4;
    bool neg = false;
    if (val & 0x800) {
        val = (~val) + 1;
        neg = true;
    }
    if (temp) {
        *temp = val * 0.0625;
    }
    return SYSTEM_ERROR_NONE;
}
