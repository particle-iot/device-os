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

#ifndef DEMUX_H
#define DEMUX_H

#include "static_recursive_mutex.h"
#include "pinmap_defines.h"
#include "pinmap_hal.h"


#define DEMUX_MAX_PIN_COUNT     8

namespace particle {

class Demux {
public:
    int write(uint8_t pin, uint8_t value);
    static Demux& getInstance();
    static int lock();
    static int unlock();

private:
    Demux();
    ~Demux();

    void init();
    uint8_t getPinValue(uint8_t pin) const;
    void setPinValue(uint8_t pin, uint8_t value);

    const uint8_t DEFAULT_PINS_VALUE = 0x7F;

    bool initialized_;
    uint8_t pinValue_; // Bitmask
    static StaticRecursiveMutex mutex_;
}; // class Demux

class DemuxLock {
public:
    DemuxLock()
            : locked_(false) {
        lock();
    }

    ~DemuxLock() {
        if (locked_) {
            unlock();
        }
    }

    DemuxLock(DemuxLock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        Demux::lock();
        locked_ = true;
    }

    void unlock() {
        Demux::unlock();
        locked_ = false;
    }

    DemuxLock(const DemuxLock&) = delete;
    DemuxLock& operator=(const DemuxLock&) = delete;

private:
    bool locked_;
};

} // namespace particle

#define DEMUX particle::Demux::getInstance()


#endif // DEMUX_H
