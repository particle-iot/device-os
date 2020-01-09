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

#include "io_expander.h"
#include "io_expander_impl.h"

using namespace spark;

namespace {

const uint8_t INVALID_I2C_ADDRESS = 0x7F;

// Resister address
const uint8_t inputPortReg[2]       = { 0x00, 0x01 };
const uint8_t outputPortReg[2]      = { 0x02, 0x03 };
const uint8_t polarityReg[2]        = { 0x04, 0x05 };
const uint8_t dirReg[2]             = { 0x06, 0x07 };
const uint8_t outputDriveReg[4]     = { 0x40, 0x41, 0x42, 0x43 };
const uint8_t inputLatchReg[2]      = { 0x44, 0x45 };
const uint8_t pullEnableReg[2]      = { 0x46, 0x47 };
const uint8_t pullSelectReg[2]      = { 0x48, 0x49 };
const uint8_t intMaskReg[2]         = { 0x4A, 0x4B };
const uint8_t intStatusReg[2]       = { 0x4C, 0x4D };
const uint8_t outputPortConfigReg   = 0x4F;

// This only caches the writable registers for pin specific operation.
struct RegisterValues {
    RegisterValues() {
        reset();
    }
    ~RegisterValues() {}

    void reset() {
        memset(output, 0xff, sizeof(output));
        memset(inputInverted, 0x00, sizeof(inputInverted));
        memset(dir, 0xff, sizeof(dir));
        memset(outputDrive, 0xff, sizeof(outputDrive));
        memset(inputLatch, 0x00, sizeof(inputLatch));
        memset(pullEnable, 0x00, sizeof(pullEnable));
        memset(pullSelect, 0xff, sizeof(pullSelect));
        memset(intMask, 0xff, sizeof(intMask));
        portPull = 0x00;
    }

    uint8_t output[2];
    uint8_t inputInverted[2];
    uint8_t dir[2];
    uint8_t outputDrive[4];
    uint8_t inputLatch[2];
    uint8_t pullEnable[2];
    uint8_t pullSelect[2];
    uint8_t intMask[2];
    uint8_t portPull;
};

struct IntterruptCallback {
    IntterruptCallback()
            : port(IoExpanderPort::INVALID),
              pin(IoExpanderPin::INVALID),
              cb(nullptr) {
    }
    ~IntterruptCallback() {}

    IoExpanderPort port;
    IoExpanderPin pin;
    IoExpanderIntTrigger trig;
    IoExpanderOnInterruptCallback cb;
};

struct IoExpanderControlBlock {
    IoExpanderControlBlock() {
        reset();
    }
    ~IoExpanderControlBlock() {}

    void reset() {
        exit = false;
        handleInterruptThread = nullptr;
        queue = nullptr;
        address = INVALID_I2C_ADDRESS;
        resetPin = PIN_INVALID;
        intPin = PIN_INVALID;
        callbacks.clear();
        regValues.reset();
    }

    uint8_t address;
    pin_t resetPin;
    pin_t intPin;
    RegisterValues regValues;
    Vector<IntterruptCallback> callbacks;
    os_thread_t handleInterruptThread;
    os_queue_t queue;
    bool exit = false;
};

IoExpanderControlBlock IoCtlBlk;


int ioExpanderWriteRegister(uint8_t reg, uint8_t val) {
    IoExpanderLock lock;
    LOG(TRACE, "ioExpanderWriteRegister(0x%02x, 0x%02x)", reg, val);
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    Wire.beginTransmission(IoCtlBlk.address);
    Wire.write(buf, sizeof(buf));
    return Wire.endTransmission();
}

int ioExpanderReadRegister(uint8_t reg, uint8_t* val) {
    IoExpanderLock lock;
    // FIXME: Why does it run into hardfault if looging here?
    // LOG(TRACE, "ioExpanderReadRegister(0x%02x)", reg);
    Wire.beginTransmission(IoCtlBlk.address);
    Wire.write(&reg, 1);
    CHECK_TRUE(Wire.endTransmission(false) == 0, SYSTEM_ERROR_INTERNAL);
    Wire.requestFrom(IoCtlBlk.address, (uint8_t)1);
    if (Wire.available()) {
        *val = Wire.read();
    }
    return SYSTEM_ERROR_NONE;
}

int ioExpanderWritePin(uint8_t port, uint8_t pin, IoExpanderPinValue val) {
    IoExpanderLock lock;
    uint8_t newVal = IoCtlBlk.regValues.output[port];
    uint8_t bitMask = 0x01 << pin;
    if (val == IoExpanderPinValue::HIGH) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(ioExpanderWriteRegister(outputPortReg[port], newVal));
    IoCtlBlk.regValues.output[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int ioExpanderReadPin(uint8_t port, uint8_t pin, IoExpanderPinValue& val) {
    IoExpanderLock lock;
    uint8_t newVal = 0x00;
    uint8_t bitMask = 0x01 << pin;
    CHECK(ioExpanderReadRegister(inputPortReg[port], &newVal));
    if (newVal & bitMask) {
        val = IoExpanderPinValue::HIGH;
    } else {
        val = IoExpanderPinValue::LOW;
    }
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigureDirection(uint8_t port, uint8_t pin, IoExpanderPinDir dir) {
    IoExpanderLock lock;
    uint8_t newVal = IoCtlBlk.regValues.dir[port];
    uint8_t bitMask = 0x01 << pin;
    if (dir == IoExpanderPinDir::OUTPUT) {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    } else {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    }
    CHECK(ioExpanderWriteRegister(dirReg[port], newVal));
    IoCtlBlk.regValues.dir[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigurePull(uint8_t port, uint8_t pin, IoExpanderPinPull pull) {
    IoExpanderLock lock;
    uint8_t newPullEn = IoCtlBlk.regValues.pullEnable[port];
    uint8_t newPullSel = IoCtlBlk.regValues.pullSelect[port];
    uint8_t bitMask = 0x01 << pin;
    if (pull == IoExpanderPinPull::NO_PULL) {
        CHECK_TRUE((newPullEn & bitMask), SYSTEM_ERROR_NONE);
        newPullEn &= ~bitMask;
        CHECK(ioExpanderWriteRegister(pullEnableReg[port], newPullEn));
        IoCtlBlk.regValues.pullEnable[port] = newPullEn;
    } else {
        if (pull == IoExpanderPinPull::PULL_UP && !(newPullSel & bitMask)) {
            newPullSel |= bitMask;
            CHECK(ioExpanderWriteRegister(pullSelectReg[port], newPullSel));
            IoCtlBlk.regValues.pullSelect[port] = newPullSel;
        }
        if (pull == IoExpanderPinPull::PULL_DOWN && (newPullSel & bitMask)) {
            newPullSel &= ~bitMask;
            CHECK(ioExpanderWriteRegister(pullSelectReg[port], newPullSel));
            IoCtlBlk.regValues.pullSelect[port] = newPullSel;
        }
        if (!(newPullEn & bitMask)) {
            newPullEn |= bitMask;
            CHECK(ioExpanderWriteRegister(pullEnableReg[port], newPullEn));
            IoCtlBlk.regValues.pullEnable[port] = newPullEn;
        }
    }
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigureDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive) {
    IoExpanderLock lock;
    uint8_t regIdx = (port * 2) + (pin / 4);
    uint8_t newVal = IoCtlBlk.regValues.outputDrive[regIdx];
    uint8_t bitMask = 0x03 << ((pin % 4) * 2);
    auto bitsVal = static_cast<uint8_t>(drive);
    CHECK_TRUE((newVal & bitMask) != bitsVal, SYSTEM_ERROR_NONE);
    newVal &= ~bitMask;
    newVal |= bitsVal << ((pin % 4) * 2);
    CHECK(ioExpanderWriteRegister(outputDriveReg[regIdx], newVal));
    IoCtlBlk.regValues.outputDrive[regIdx] = newVal;
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigureLatch(uint8_t port, uint8_t pin, bool enable) {
    IoExpanderLock lock;
    uint8_t newVal = IoCtlBlk.regValues.inputLatch[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(ioExpanderWriteRegister(inputLatchReg[port], newVal));
    IoCtlBlk.regValues.inputLatch[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigureInverted(uint8_t port, uint8_t pin, bool enable) {
    IoExpanderLock lock;
    uint8_t newVal = IoCtlBlk.regValues.inputInverted[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(ioExpanderWriteRegister(polarityReg[port], newVal));
    IoCtlBlk.regValues.inputInverted[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int ioExpanderConfigureInterrupt(uint8_t port, uint8_t pin, bool enable) {
    IoExpanderLock lock;
    uint8_t newVal = IoCtlBlk.regValues.intMask[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    } else {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    }
    CHECK(ioExpanderWriteRegister(intMaskReg[port], newVal));
    IoCtlBlk.regValues.intMask[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

void ioExpanderIsr(void) {
    if (IoCtlBlk.queue) {
        bool flag = true;
        os_queue_put(IoCtlBlk.queue, &flag, 0, nullptr);
    }
}

int ioExpanderEnableInterrupt() {
    static bool configured = false;
    CHECK_FALSE(configured, SYSTEM_ERROR_NONE);
    CHECK_TRUE(attachInterrupt(IoCtlBlk.intPin, ioExpanderIsr, FALLING), SYSTEM_ERROR_INTERNAL);
    configured = true;
    return SYSTEM_ERROR_NONE;
}

os_thread_return_t ioInterruptHandleThread(void* param) {
    while(!IoCtlBlk.exit) {
        bool flag;
        os_queue_take(IoCtlBlk.queue, &flag, CONCURRENT_WAIT_FOREVER, nullptr);
        {
            IoExpanderLock lock;
            uint8_t input[2] = {0x00, 0x00};
            uint8_t intStatus[2] = {0x00, 0x00};
            if (ioExpanderReadRegister(intStatusReg[0], &intStatus[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (ioExpanderReadRegister(intStatusReg[1], &intStatus[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (ioExpanderReadRegister(inputPortReg[0], &input[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (ioExpanderReadRegister(inputPortReg[1], &input[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            for (const auto& callback : IoCtlBlk.callbacks) {
                auto port = static_cast<uint8_t>(callback.port);
                auto pin = static_cast<uint8_t>(callback.pin);
                uint8_t bitMask = 0x01 << pin;
                if ((intStatus[port] & bitMask) && (callback.cb != nullptr)) {
                    if ( ((callback.trig == IoExpanderIntTrigger::RISING) && (input[port] & bitMask)) ||
                         ((callback.trig == IoExpanderIntTrigger::FALLING) && !(input[port] & bitMask)) ||
                         (callback.trig == IoExpanderIntTrigger::CHANGE) ) {
                        callback.cb();
                    }
                }
            }
        }
    }
    os_thread_exit(IoCtlBlk.handleInterruptThread);
}

} // anonymous namespace


int io_expander_init(uint8_t addr, pin_t reset, pin_t intPin) {
    IoExpanderLock lock;
    IoCtlBlk.address = addr;
    IoCtlBlk.resetPin = reset;
    IoCtlBlk.intPin = intPin;
    if (os_queue_create(&IoCtlBlk.queue, 1, 1, nullptr)) {
        IoCtlBlk.queue = nullptr;
        LOG(ERROR, "os_queue_create() failed");
    }
    if (os_thread_create(&IoCtlBlk.handleInterruptThread, "IO Expander Thread", OS_THREAD_PRIORITY_CRITICAL, ioInterruptHandleThread, nullptr, 512)) {
        os_queue_destroy(IoCtlBlk.queue, nullptr);
        IoCtlBlk.queue = nullptr;
        LOG(ERROR, "os_thread_create() failed");
    }
    Wire.setSpeed(CLOCK_SPEED_400KHZ);
    Wire.begin();
    CHECK(io_expander_hard_reset());
    return SYSTEM_ERROR_NONE;
}

int io_expander_deinit(void) {
    IoExpanderLock lock;
    IoCtlBlk.exit = true;
    os_thread_join(IoCtlBlk.handleInterruptThread);
    os_thread_cleanup(IoCtlBlk.handleInterruptThread);
    os_queue_destroy(IoCtlBlk.queue, nullptr);
    CHECK(io_expander_hard_reset());
    IoCtlBlk.reset();
    Wire.end();
    return SYSTEM_ERROR_NONE;
}

int io_expander_hard_reset(void) {
    IoExpanderLock lock;
    CHECK_TRUE(IoCtlBlk.resetPin != PIN_INVALID, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Assert reset pin
    pinMode(IoCtlBlk.resetPin, OUTPUT);
    digitalWrite(IoCtlBlk.resetPin, LOW);
    delayMicroseconds(1);
    digitalWrite(IoCtlBlk.resetPin, HIGH);
    return SYSTEM_ERROR_NONE;
}

int io_expander_configure_pin(const IoExpanderPinConfig& config) {
    IoExpanderLock lock;
    auto port = static_cast<uint8_t>(config.port);
    auto pin = static_cast<uint8_t>(config.pin);
    CHECK_TRUE(port < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(ioExpanderConfigurePull(port, pin, config.pull));
    if (config.dir == IoExpanderPinDir::INPUT) {
        CHECK(ioExpanderConfigureLatch(port, pin, config.inputLatch));
        CHECK(ioExpanderConfigureInverted(port, pin, config.inputInverted));
        if (config.intEn && config.callback != nullptr) {
            IntterruptCallback cb;
            cb.port = config.port;
            cb.pin = config.pin;
            cb.trig = config.trig;
            cb.cb = config.callback;
            CHECK_TRUE(IoCtlBlk.callbacks.append(cb), SYSTEM_ERROR_NO_MEMORY);
        }
        CHECK(ioExpanderConfigureInterrupt(port, pin, config.intEn));
        if (IoCtlBlk.callbacks.size() > 0) {
            CHECK(ioExpanderEnableInterrupt());
        }
    } else {
        CHECK(ioExpanderConfigureDrive(port, pin, config.drive));
    }
    CHECK(ioExpanderConfigureDirection(port, pin, config.dir));
    return SYSTEM_ERROR_NONE;
}

int io_expander_write_pin(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue val) {
    IoExpanderLock lock;
    auto sPort = static_cast<uint8_t>(port);
    auto sPin = static_cast<uint8_t>(pin);
    uint8_t bitMask = 0x01 << sPin;
    CHECK_TRUE(sPort < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(sPin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(!(IoCtlBlk.regValues.dir[sPort] & bitMask), SYSTEM_ERROR_INVALID_STATE);
    CHECK(ioExpanderWritePin(sPort, sPin, val));
    return SYSTEM_ERROR_NONE;
}

int io_expander_read_pin(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue& val) {
    IoExpanderLock lock;
    auto sPort = static_cast<uint8_t>(port);
    auto sPin = static_cast<uint8_t>(pin);
    CHECK_TRUE(sPort < IO_EXPANDER_PORT_COUNT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(sPin < IO_EXPANDER_PIN_COUNT_PER_PORT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(ioExpanderReadPin(sPort, sPin, val));
    return SYSTEM_ERROR_NONE;
}
