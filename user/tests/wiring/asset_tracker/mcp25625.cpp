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

#include "mcp25625.h"

#if PLATFORM_ID == PLATFORM_B5SOM
#define SPIIF                           SPI
#elif PLATFORM_ID == PLATFORM_TRACKER
#define SPIIF                           SPI1
#endif

using namespace spark;
using namespace particle;

namespace {

} // anonymous namespace

Mcp25625::Mcp25625()
        : initialized_(false),
          csPin_(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN1),
          resetPin_(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN0),
          pwrEnPin_(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN7) {

}

Mcp25625::~Mcp25625() {

}

Mcp25625& Mcp25625::getInstance() {
    static Mcp25625 newCanTransceiver;
    return newCanTransceiver;
}

int Mcp25625::begin() {
    CanTransceiverLock lock(mutex_);
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
    CHECK(csPin_.mode(IoExpanderPinMode::OUTPUT));
    CHECK(csPin_.write(IoExpanderPinValue::HIGH));
    CHECK(pwrEnPin_.mode(IoExpanderPinMode::OUTPUT));
    CHECK(pwrEnPin_.write(IoExpanderPinValue::HIGH));
    CHECK(resetPin_.mode(IoExpanderPinMode::OUTPUT));
    CHECK(resetPin_.write(IoExpanderPinValue::LOW));
    delay(100);
    CHECK(resetPin_.write(IoExpanderPinValue::HIGH));
    SPIIF.setDataMode(SPI_MODE0);
    SPIIF.setClockSpeed(5 * 1000 * 1000);
    SPIIF.begin();
    initialized_ = true;
    CHECK(reset());
    return SYSTEM_ERROR_NONE;
}

int Mcp25625::end() {
    CanTransceiverLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    return SYSTEM_ERROR_NONE;
}

int Mcp25625::sleep() {
    CanTransceiverLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int Mcp25625::wakeup() {
    CanTransceiverLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int Mcp25625::reset() {
    CanTransceiverLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(csPin_.write(IoExpanderPinValue::LOW));
    SPIIF.transfer(static_cast<uint8_t>(Mcp25625Instruction::RESET_REG));
    return csPin_.write(IoExpanderPinValue::HIGH);
}

int Mcp25625::getCanCtrl(uint8_t* const value) {
    return readRegister(0x7f, value);
}

int Mcp25625::writeRegister(const uint8_t reg, const uint8_t val) {
    return SYSTEM_ERROR_NONE;
}

int Mcp25625::readRegister(const uint8_t reg, uint8_t* const val) {
    CanTransceiverLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(csPin_.write(IoExpanderPinValue::LOW));
    SPIIF.transfer(static_cast<uint8_t>(Mcp25625Instruction::READ)); // Instruction
    SPIIF.transfer(reg); // Address
    *val = SPIIF.transfer(reg); // Value
    return csPin_.write(IoExpanderPinValue::HIGH);
}

RecursiveMutex Mcp25625::mutex_;
