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

class IoExpanderLock {
public:
    IoExpanderLock()
            : locked_(false) {
        lock();
    }

    IoExpanderLock(IoExpanderLock&& lock)
            : locked_(lock.locked_) {
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
    static RecursiveMutex mutex_;
};

RecursiveMutex IoExpanderLock::mutex_;

using namespace particle;


IoExpander::IoExpander()
        : initialized_(false) {
}

IoExpander::~IoExpander() {
    deinit();
}

IoExpander& IoExpander::getInstance() {
    static IoExpander ioExpander;
    return ioExpander;
}

int IoExpander::init(uint8_t addr, pin_t resetPin, pin_t intPin) {
    IoExpanderLock lock;
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
    CHECK(io_expander_init(addr, resetPin, intPin));
    initialized_ = true;
    return SYSTEM_ERROR_NONE;
}

int IoExpander::deinit() {
    IoExpanderLock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    CHECK(io_expander_deinit());
    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int IoExpander::reset() const {
    IoExpanderLock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return io_expander_hard_reset();
}

int IoExpander::sleep() const {
    // TODO: implementation
    return SYSTEM_ERROR_NONE;
}

int IoExpander::wakeup() const {
    // TODO: implementation
    return SYSTEM_ERROR_NONE;
}

int IoExpander::configure(const IoExpanderPinConfig& config) {
    IoExpanderLock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(io_expander_configure_pin(config));
    return SYSTEM_ERROR_NONE;
}

int IoExpander::write(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue val) {
    IoExpanderLock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(io_expander_write_pin(port, pin, val));
    return SYSTEM_ERROR_NONE;
}

int IoExpander::read(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue& val) {
    IoExpanderLock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(io_expander_read_pin(port, pin, val));
    return SYSTEM_ERROR_NONE;
}


