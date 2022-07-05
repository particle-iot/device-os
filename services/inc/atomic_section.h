/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "interrupts_hal.h"

namespace particle {

class AtomicSection {
    int prev;
public:
    AtomicSection() {
        prev = HAL_disable_irq();
    }

    ~AtomicSection() {
        HAL_enable_irq(prev);
    }
};

#define ATOMIC_BLOCK() 	for (bool __todo=true; __todo;) for (::particle::AtomicSection __as; __todo; __todo=false)

// Class implementing a concurrency policy based on critical sections
class AtomicConcurrency {
public:
    int lock() const {
        return HAL_disable_irq();
    }

    void unlock(int state) const {
        HAL_enable_irq(state);
    }
};

} // particle
