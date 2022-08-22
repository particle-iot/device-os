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

#include "hal_platform.h"

#if HAL_PLATFORM_DEMUX

#include "static_recursive_mutex.h"
#include "pinmap_defines.h"
#include "pinmap_hal.h"
#if HAL_PLATFORM_NRF52840
#include "nrf_gpio.h"
#endif


#define DEMUX_MAX_PIN_COUNT     8
#if HAL_PLATFORM_NRF52840
#define DEMUX_NRF_PORT          (NRF_P1)
#define DEMUX_PIN_0_MASK        0x00000400
#define DEMUX_PIN_1_MASK        0x00000800
#define DEMUX_PIN_2_MASK        0x00001000
#define DEMUX_PINS_SHIFT        (10)
#endif


namespace particle {

class Demux {
public:
    int write(uint8_t pin, uint8_t value);
    uint8_t read(uint8_t pin) const;
    int lock();
    int unlock();
    static Demux& getInstance();

private:
    Demux();
    ~Demux();

    void init();
    uint8_t getPinValue(uint8_t pin) const;
    void setPinValue(uint8_t pin, uint8_t value);

    static constexpr uint8_t DEFAULT_PINS_VALUE = 0x7F;

    bool initialized_;
    uint8_t pinValue_; // Bitmask
    StaticRecursiveMutex mutex_;
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
        Demux::getInstance().lock();
        locked_ = true;
    }

    void unlock() {
        Demux::getInstance().unlock();
        locked_ = false;
    }

    DemuxLock(const DemuxLock&) = delete;
    DemuxLock& operator=(const DemuxLock&) = delete;

private:
    bool locked_;
};

} // namespace particle

#endif // HAL_PLATFORM_DEMUX

#endif // DEMUX_H
