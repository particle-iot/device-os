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

#ifndef IO_EXPANDER_H
#define IO_EXPANDER_H

#include "Particle.h"

namespace particle {

typedef void (*IoExpanderOnInterruptCallback)(void* context);

enum class IoExpanderPinValue : uint8_t {
    HIGH,
    LOW
};

enum class IoExpanderPinMode : uint8_t {
    INPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN,
    OUTPUT,
    OUTPUT_PULLUP,
    OUTPUT_PULLDOWN
};

enum class IoExpanderIntTrigger : uint8_t {
    CHANGE,
    RISING,
    FALLING
};

enum class IoExpanderPort : uint8_t {
    // It must start from 0, as it is also the port index.
    PORT0 = 0,
    PORT1 = 1,
    INVALID = 0x7F
};

enum class IoExpanderPin : uint8_t {
    // It must start from 0, as it is also the bit index in registers.
    PIN0 = 0,
    PIN1 = 1,
    PIN2 = 2,
    PIN3 = 3,
    PIN4 = 4,
    PIN5 = 5,
    PIN6 = 6,
    PIN7 = 7,
    INVALID = 0x7F
};

enum class IoExpanderPinDrive : uint8_t {
    PERCENT25 = 0x00,
    PERCENT50 = 0x01,
    PERCENT75 = 0x02,
    PERCENT100 = 0x03
};


class IoExpanderLock {
public:
    IoExpanderLock(RecursiveMutex& mutex)
            : locked_(false),
              mutex_(mutex) {
        lock();
    }

    IoExpanderLock(IoExpanderLock&& lock)
            : locked_(lock.locked_),
              mutex_(lock.mutex_) {
        lock.locked_ = false;
    }

    IoExpanderLock(const IoExpanderLock&) = delete;
    IoExpanderLock& operator=(const IoExpanderLock&) = delete;

    ~IoExpanderLock() {
        if (locked_) {
            unlock();
        }
    }

    void lock() {
        mutex_.lock();
        locked_ = true;
    }

    void unlock() {
        mutex_.unlock();
        locked_ = false;
    }

private:
    bool locked_;
    RecursiveMutex& mutex_;
};


class IoExpanderBase {
public:
    virtual int begin(uint8_t address, pin_t resetPin, pin_t interruptPin, TwoWire* wire) = 0;
    virtual int end() = 0;
    virtual int reset() = 0;
    virtual int sleep() = 0;
    virtual int wakeup() = 0;

    virtual int setPinMode(uint8_t port, uint8_t pin, IoExpanderPinMode mode) = 0;
    virtual int setPinOutputDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive) = 0;
    virtual int setPinInputInverted(uint8_t port, uint8_t pin, bool enable) = 0;
    virtual int setPinInputLatch(uint8_t port, uint8_t pin, bool enable) = 0;
    virtual int writePinValue(uint8_t port, uint8_t pin, IoExpanderPinValue value) = 0;
    virtual int readPinValue(uint8_t port, uint8_t pin, IoExpanderPinValue& value) = 0;
    virtual int attachPinInterrupt(uint8_t port, uint8_t pin, IoExpanderIntTrigger trig, IoExpanderOnInterruptCallback callback, void* context) = 0;
};


class IoExpanderPinObj {
public:
    IoExpanderPinObj();
    IoExpanderPinObj(IoExpanderBase& instance, IoExpanderPort port, IoExpanderPin pin);
    ~IoExpanderPinObj();

    int mode(IoExpanderPinMode mode) const;
    int outputDrive(IoExpanderPinDrive drive) const;
    int inputInverted(bool enable) const;
    int inputLatch(bool enable) const;
    int write(IoExpanderPinValue value) const;
    int read(IoExpanderPinValue& value) const;
    int attachInterrupt(IoExpanderIntTrigger trig, IoExpanderOnInterruptCallback callback, void* context) const;

private:
    IoExpanderBase* instance_;
    uint8_t port_;
    uint8_t pin_;
    bool configured_;
};

} // namespace particle

#endif // IO_EXPANDER_H
