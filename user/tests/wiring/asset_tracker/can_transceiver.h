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

#ifndef CAN_TRANSCEIVER_H
#define CAN_TRANSCEIVER_H

#include "Particle.h"


class CanTransceiverLock {
public:
    CanTransceiverLock(RecursiveMutex& mutex)
            : locked_(false),
              mutex_(mutex) {
        lock();
    }

    CanTransceiverLock(CanTransceiverLock&& lock)
            : locked_(lock.locked_),
              mutex_(lock.mutex_) {
        lock.locked_ = false;
    }

    CanTransceiverLock(const CanTransceiverLock&) = delete;
    CanTransceiverLock& operator=(const CanTransceiverLock&) = delete;

    ~CanTransceiverLock() {
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


class CanTransceiverBase {
public:
    virtual int begin() = 0;
    virtual int end() = 0;
    virtual int sleep() = 0;
    virtual int wakeup() = 0;

    virtual int reset() = 0;
};

#endif // CAN_TRANSCEIVER_H