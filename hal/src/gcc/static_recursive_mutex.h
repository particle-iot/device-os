/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include <mutex>
#include <chrono>

class StaticRecursiveMutex {
public:
    StaticRecursiveMutex() = default;

    bool lock(unsigned timeout = 0) {
        if (timeout > 0) {
            return mutex_.try_lock_for(std::chrono::milliseconds(timeout));
        }
        mutex_.lock();
        return true;
    }

    bool unlock() {
        mutex_.unlock();
        return true;
    }

private:
    std::recursive_timed_mutex mutex_;
};
