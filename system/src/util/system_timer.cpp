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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include <cassert>

#include "system_timer.h"

#include "system_error.h"
#include "logging.h"
#include "debug.h"

namespace particle::system {

SystemTimer::~SystemTimer() {
    stop();
    if (timer_) {
        os_timer_destroy(timer_, nullptr);
    }
}

int SystemTimer::start(unsigned timeout) {
    stop();
    // Period of an RTOS timer must be greater than 0
    if (timeout > 0) {
        if (!timer_) {
            int r = os_timer_create(&timer_, timeout, timerCallback, this /* timer_id */, true /* one_shot */, nullptr /* reserved */);
            if (r != 0) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            r = os_timer_change(timer_, OS_TIMER_CHANGE_START, false /* fromISR */, 0 /* period */, 0xffffffffu /* block */, nullptr /* reserved */);
            if (r != 0) {
                return SYSTEM_ERROR_INTERNAL; // Should not happen
            }
        } else {
            // Changing the period also starts the timer
            int r = os_timer_change(timer_, OS_TIMER_CHANGE_PERIOD, false /* fromISR */, timeout, 0xffffffffu /* block */, nullptr /* reserved */);
            if (r != 0) {
                return SYSTEM_ERROR_INTERNAL; // Should not happen
            }
        }
    } else {
        // Schedule a call in the system thread
        SystemISRTaskQueue.enqueue(this);
    }
    return 0;
}

void SystemTimer::stop() {
    if (timer_) {
        int r = os_timer_change(timer_, OS_TIMER_CHANGE_STOP, false /* fromISR */, 0 /* period */, 0xffffffffu /* block */, nullptr /* reserved */);
        if (r != 0) {
            LOG_DEBUG(ERROR, "Failed to stop timer"); // Should not happen
        }
    }
    SystemISRTaskQueue.remove(this);
}

void SystemTimer::taskCallback(ISRTaskQueue::Task* task) {
    auto self = static_cast<SystemTimer*>(task);
    assert(self && self->callback_);
    self->callback_(self->arg_);
}

void SystemTimer::timerCallback(os_timer_t timer) {
    void* id = nullptr;
    int r = os_timer_get_id(timer, &id);
    if (r != 0) {
        LOG_DEBUG(ERROR, "Failed to get timer ID"); // Should not happen
        return;
    }
    auto self = static_cast<SystemTimer*>(id);
    assert(self);
    SystemISRTaskQueue.enqueue(self);
}

} // namespace particle::system
