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

#include "pcal6416a.h"

using namespace spark;
using namespace particle;

namespace {
void ioExpanderInterruptHandler(void) {
    Pcal6416a::getInstance().sync();
}
} // anonymous namespace


Pcal6416a::Pcal6416a() {

}

Pcal6416a::~Pcal6416a() {

}

int Pcal6416a::begin(uint8_t address, pin_t resetPin, pin_t interruptPin) {
    IoExpanderLock lock(mutex_);
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);

    address_ = address;
    resetPin_ = resetPin;
    intPin_ = interruptPin;

    if (os_queue_create(&ioExpanderWorkerQueue_, 1, 1, nullptr)) {
        ioExpanderWorkerQueue_ = nullptr;
        LOG(ERROR, "os_queue_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&ioExpanderWorkerThread_, "IO Expander Thread", OS_THREAD_PRIORITY_CRITICAL, ioInterruptHandleThread, this, 512)) {
        os_queue_destroy(ioExpanderWorkerQueue_, nullptr);
        ioExpanderWorkerQueue_ = nullptr;
        LOG(ERROR, "os_thread_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    Wire.setSpeed(CLOCK_SPEED_400KHZ);
    Wire.begin();
    CHECK_TRUE(attachInterrupt(intPin_, ioExpanderInterruptHandler, FALLING), SYSTEM_ERROR_INTERNAL);

    initialized_ = true;
    CHECK(reset());
    resetRegValue();

    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::end() {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);

    ioExpanderWorkerThreadExit_ = true;
    os_thread_join(ioExpanderWorkerThread_);
    os_thread_cleanup(ioExpanderWorkerThread_);
    os_queue_destroy(ioExpanderWorkerQueue_, nullptr);
    ioExpanderWorkerThreadExit_ = false;
    ioExpanderWorkerThread_ = nullptr;
    ioExpanderWorkerQueue_ = nullptr;

    CHECK(reset());
    Wire.end();

    address_ = INVALID_I2C_ADDRESS;
    resetPin_ = PIN_INVALID;
    intPin_ = PIN_INVALID;
    intConfigs_.clear();

    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::reset() {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(resetPin_ != PIN_INVALID, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Assert reset pin
    pinMode(resetPin_, OUTPUT);
    digitalWrite(resetPin_, LOW);
    delayMicroseconds(1);
    digitalWrite(resetPin_, HIGH);
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::sleep() {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int Pcal6416a::wakeup() {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int Pcal6416a::setPinMode(uint8_t port, uint8_t pin, IoExpanderPinMode mode) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(configurePullAbility(port, pin, mode));
    return configureDirection(port, pin, mode);
}

int Pcal6416a::setPinOutputDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    return configureOutputDrive(port, pin, drive);
}

int Pcal6416a::setPinInputInverted(uint8_t port, uint8_t pin, bool enable) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    return configureInputInverted(port, pin, enable);
}

int Pcal6416a::setPinInputLatch(uint8_t port, uint8_t pin, bool enable) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    return configureInputLatch(port, pin, enable);
}

int Pcal6416a::writePinValue(uint8_t port, uint8_t pin, IoExpanderPinValue value) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    return writePin(port, pin, value);
}

int Pcal6416a::readPinValue(uint8_t port, uint8_t pin, IoExpanderPinValue& value) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    return readPin(port, pin, value);
}

int Pcal6416a::attachPinInterrupt(uint8_t port, uint8_t pin, IoExpanderIntTrigger trig, IoExpanderOnInterruptCallback callback) {
    IoExpanderLock lock(mutex_);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (callback != nullptr) {
        IoPinInterruptConfig config = {};
        config.port = port;
        config.pin = pin;
        config.trig = trig;
        config.cb = callback;
        CHECK_TRUE(intConfigs_.append(config), SYSTEM_ERROR_NO_MEMORY);
    }
    return configureIntMask(port, pin, true);
}

Pcal6416a& Pcal6416a::getInstance() {
    static Pcal6416a newIoExpander;
    return newIoExpander;
}

void Pcal6416a::resetRegValue() {
    memset(outputRegValue_, 0xff, sizeof(outputRegValue_));
    memset(inputInvertedRegValue_, 0x00, sizeof(inputInvertedRegValue_));
    memset(dirRegValue_, 0xff, sizeof(dirRegValue_));
    memset(outputDriveRegValue_, 0xff, sizeof(outputDriveRegValue_));
    memset(inputLatchRegValue_, 0x00, sizeof(inputLatchRegValue_));
    memset(pullEnableRegValue_, 0x00, sizeof(pullEnableRegValue_));
    memset(pullSelectRegValue_, 0xff, sizeof(pullSelectRegValue_));
    memset(intMaskRegValue_, 0xff, sizeof(intMaskRegValue_));
    portPullRegValue_ = 0x00;
}

int Pcal6416a::writeRegister(uint8_t reg, uint8_t val) {
    LOG(TRACE, "writeRegister(0x%02x, 0x%02x)", reg, val);
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    Wire.beginTransmission(address_);
    Wire.write(buf, sizeof(buf));
    return Wire.endTransmission();
}

int Pcal6416a::readRegister(uint8_t reg, uint8_t* val) {
    Wire.beginTransmission(address_);
    Wire.write(&reg, 1);
    CHECK_TRUE(Wire.endTransmission(false) == 0, SYSTEM_ERROR_INTERNAL);
    Wire.requestFrom(address_, (uint8_t)1);
    if (Wire.available()) {
        *val = Wire.read();
    }
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configurePullAbility(uint8_t port, uint8_t pin, IoExpanderPinMode mode) {
    uint8_t newPullEn = pullEnableRegValue_[port];
    uint8_t newPullSel = pullSelectRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (mode == IoExpanderPinMode::INPUT || mode == IoExpanderPinMode::OUTPUT) {
        CHECK_TRUE((newPullEn & bitMask), SYSTEM_ERROR_NONE);
        newPullEn &= ~bitMask;
        CHECK(writeRegister(PULL_ENABLE_REG[port], newPullEn));
        pullEnableRegValue_[port] = newPullEn;
    } else {
        if ((mode == IoExpanderPinMode::INPUT_PULLUP || mode == IoExpanderPinMode::OUTPUT_PULLUP) && !(newPullSel & bitMask)) {
            newPullSel |= bitMask;
            CHECK(writeRegister(PULL_SELECT_REG[port], newPullSel));
            pullSelectRegValue_[port] = newPullSel;
        }
        if ((mode == IoExpanderPinMode::INPUT_PULLDOWN || mode == IoExpanderPinMode::OUTPUT_PULLDOWN) && (newPullSel & bitMask)) {
            newPullSel &= ~bitMask;
            CHECK(writeRegister(PULL_SELECT_REG[port], newPullSel));
            pullSelectRegValue_[port] = newPullSel;
        }
        if (!(newPullEn & bitMask)) {
            newPullEn |= bitMask;
            CHECK(writeRegister(PULL_ENABLE_REG[port], newPullEn));
            pullEnableRegValue_[port] = newPullEn;
        }
    }
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configureDirection(uint8_t port, uint8_t pin, IoExpanderPinMode mode) {
    uint8_t newVal = dirRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (mode == IoExpanderPinMode::OUTPUT || mode == IoExpanderPinMode::OUTPUT_PULLUP || mode == IoExpanderPinMode::OUTPUT_PULLDOWN) {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    } else {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    }
    CHECK(writeRegister(DIR_REG[port], newVal));
    dirRegValue_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configureOutputDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive) {
    uint8_t regIdx = (port * 2) + (pin / 4);
    uint8_t newVal = outputDriveRegValue_[regIdx];
    uint8_t bitMask = 0x03 << ((pin % 4) * 2);
    auto bitsVal = static_cast<uint8_t>(drive);
    CHECK_TRUE((newVal & bitMask) != bitsVal, SYSTEM_ERROR_NONE);
    newVal &= ~bitMask;
    newVal |= bitsVal << ((pin % 4) * 2);
    CHECK(writeRegister(OUTPUT_DRIVE_REG[regIdx], newVal));
    outputDriveRegValue_[regIdx] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configureInputInverted(uint8_t port, uint8_t pin, bool enable) {
    uint8_t newVal = inputInvertedRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(writeRegister(POLARITY_REG[port], newVal));
    inputInvertedRegValue_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configureInputLatch(uint8_t port, uint8_t pin, bool enable) {
    uint8_t newVal = inputLatchRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(writeRegister(INPUT_LATCH_REG[port], newVal));
    inputLatchRegValue_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::configureIntMask(uint8_t port, uint8_t pin, bool enable) {
    uint8_t newVal = intMaskRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    } else {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    }
    CHECK(writeRegister(INT_MASK_REG[port], newVal));
    intMaskRegValue_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::writePin(uint8_t port, uint8_t pin, IoExpanderPinValue val) {
    uint8_t newVal = outputRegValue_[port];
    uint8_t bitMask = 0x01 << pin;
    if (val == IoExpanderPinValue::HIGH) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(writeRegister(OUTPUT_PORT_REG[port], newVal));
    outputRegValue_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::readPin(uint8_t port, uint8_t pin, IoExpanderPinValue& val) {
    uint8_t newVal = 0x00;
    uint8_t bitMask = 0x01 << pin;
    CHECK(readRegister(INPUT_PORT_REG[port], &newVal));
    if (newVal & bitMask) {
        val = IoExpanderPinValue::HIGH;
    } else {
        val = IoExpanderPinValue::LOW;
    }
    return SYSTEM_ERROR_NONE;
}

int Pcal6416a::sync() {
    if (ioExpanderWorkerQueue_) {
        bool flag = true;
        os_queue_put(ioExpanderWorkerQueue_, &flag, 0, nullptr);
    }
    return SYSTEM_ERROR_NONE;
}

os_thread_return_t Pcal6416a::ioInterruptHandleThread(void* param) {
    auto instance = static_cast<Pcal6416a*>(param);
    while(!instance->ioExpanderWorkerThreadExit_) {
        bool flag;
        os_queue_take(instance->ioExpanderWorkerQueue_, &flag, CONCURRENT_WAIT_FOREVER, nullptr);
        {
            IoExpanderLock lock(instance->mutex_);
            uint8_t input[2] = {0x00, 0x00};
            uint8_t intStatus[2] = {0x00, 0x00};
            if (instance->readRegister(instance->INT_STATUS_REG[0], &intStatus[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (instance->readRegister(instance->INT_STATUS_REG[1], &intStatus[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (instance->readRegister(instance->INPUT_PORT_REG[0], &input[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (instance->readRegister(instance->INPUT_PORT_REG[1], &input[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            for (const auto& config : instance->intConfigs_) {
                uint8_t bitMask = 0x01 << config.pin;
                if ((intStatus[config.port] & bitMask) && (config.cb != nullptr)) {
                    if ( ((config.trig == IoExpanderIntTrigger::RISING) && (input[config.port] & bitMask)) ||
                            ((config.trig == IoExpanderIntTrigger::FALLING) && !(input[config.port] & bitMask)) ||
                            (config.trig == IoExpanderIntTrigger::CHANGE) ) {
                        config.cb();
                    }
                }
            }
        }
    }
    os_thread_exit(instance->ioExpanderWorkerThread_);
}

RecursiveMutex Pcal6416a::mutex_;
