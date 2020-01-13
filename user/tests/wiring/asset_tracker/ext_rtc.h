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

#ifndef EXT_RTC_H
#define EXT_RTC_H

#include "Particle.h"
#include "io_expander.h"


enum class Weekday {
    MONDAY = 0,
    TUESDAY = 1,
    WEDNESDAY = 2,
    THURSDAY = 3,
    FRIDAY = 4,
    SATURDAY = 5,
    SUNDAY = 6
};


class ExtRtcLock {
public:
    ExtRtcLock(RecursiveMutex& mutex)
            : locked_(false),
              mutex_(mutex) {
        lock();
    }

    ExtRtcLock(ExtRtcLock&& lock)
            : locked_(lock.locked_),
              mutex_(lock.mutex_) {
        lock.locked_ = false;
    }

    ExtRtcLock(const ExtRtcLock&) = delete;
    ExtRtcLock& operator=(const ExtRtcLock&) = delete;

    ~ExtRtcLock() {
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


class ExtRtcBase {
public:
    virtual int begin(uint8_t address) = 0;
    virtual int end() = 0;
    virtual int sleep() = 0;
    virtual int wakeup() = 0;

    virtual int getPartNumber(uint16_t* id) = 0;
};

#endif // EXT_RTC_H