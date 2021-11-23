/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "hal_irq_flag.h"
#include "service_debug.h"

namespace particle {

class StaticRecursiveCriticalSectionLock {
public:
    StaticRecursiveCriticalSectionLock()
            : locked_{0},
              prevState_(0) {
    }

    void lock() {
        auto state = HAL_disable_irq();
        if (locked_++ == 0) {
            prevState_ = state;
        }
    }

    int32_t counter() {
        return locked_;
    }

    void unlock() {
        SPARK_ASSERT(locked_ > 0);
        if (--locked_ == 0) {
            HAL_enable_irq(prevState_);
        }
    }

private:
    volatile int32_t locked_;
    int32_t prevState_;
};

} // particle
