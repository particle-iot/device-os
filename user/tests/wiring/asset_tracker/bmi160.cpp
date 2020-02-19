/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "bmi160.h"

#if PLATFORM_ID == PLATFORM_B5SOM
#define WIREIF                           Wire
#elif PLATFORM_ID == PLATFORM_TRACKER
#define WIREIF                           Wire1
#endif

using namespace spark;
using namespace particle;

namespace {

} // anonymous namespace

Bmi160::Bmi160()
        : address_(INVALID_I2C_ADDRESS),
          initialized_(false),
          pwrEnPin_(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN6) {

}

Bmi160::~Bmi160() {

}

Bmi160& Bmi160::getInstance() {
    static Bmi160 newSensor;
    return newSensor;
}

int Bmi160::begin(uint8_t address) {
    Bmi160Lock lock(mutex_);
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
    address_ = address;
    CHECK(pwrEnPin_.mode(IoExpanderPinMode::OUTPUT));
    CHECK(pwrEnPin_.write(IoExpanderPinValue::HIGH));
    initialized_ = true;
    return SYSTEM_ERROR_NONE;
}

int Bmi160::end() {
    Bmi160Lock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    return SYSTEM_ERROR_NONE;
}

int Bmi160::sleep() {
    Bmi160Lock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int Bmi160::wakeup() {
    Bmi160Lock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int Bmi160::getChipId(uint8_t* val) {
    Bmi160Lock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return readRegister(Bmi160Register::CHIP_ID, val);
}

int Bmi160::writeRegister(const Bmi160Register reg, const uint8_t val) {
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>(reg);
    buf[1] = val;
    WIREIF.beginTransmission(address_);
    WIREIF.write(buf, sizeof(buf));
    return WIREIF.endTransmission();
}

int Bmi160::readRegister(const Bmi160Register reg, uint8_t* const val) {
    WIREIF.beginTransmission(address_);
    WIREIF.write(reinterpret_cast<const uint8_t*>(&reg), 1);
    CHECK_TRUE(WIREIF.endTransmission(false) == 0, SYSTEM_ERROR_INTERNAL);
    WIREIF.requestFrom(address_, (uint8_t)1);
    if (WIREIF.available()) {
        *val = WIREIF.read();
    }
    return SYSTEM_ERROR_NONE;
}

RecursiveMutex Bmi160::mutex_;
