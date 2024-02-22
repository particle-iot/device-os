/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include <bits/gthr.h>
#include <mutex>
#include "scope_guard.h"
#include "gthr-default.h"
#include "delay_hal.h"

extern "C" void __once_proxy(void) {
    std::__once_functor();
}

namespace std {

extern std::unique_lock<std::mutex>*& __get_once_functor_lock_ptr();

} // std

// XXX/FIXME: std::call_once platform-specific implementations are not exported to the application
// so we should probably just get rid of std::call_once altogether and replace with a separate implememntation.
extern "C" int __gthread_once (__gthread_once_t* once, void (*func) (void))
{
    if (once->load(std::memory_order_acquire) == GTHREAD_ONCE_STATE_INITIALIZED) {
        // Short path, already initialized
        return 0;
    }

    // Make a local copy, this is a workaround for a non-entrant implementation in libstdc++
    auto callable = std::__once_functor;
    auto lock = std::__get_once_functor_lock_ptr();
    if (lock) {
        // For consistency
        std::__set_once_functor_lock_ptr(0);
        // This won't be relocked in <mutex> std::call_once implementation
        lock->unlock();
    }
    // std::__once_functor() is not usable from this point on, local callable should be used

    auto state = GTHREAD_ONCE_STATE_NOT_INITIALIZED;
    if (once->compare_exchange_strong(state, GTHREAD_ONCE_STATE_RUNNING,
            std::memory_order_acq_rel)) {
        // Switched to initializing/running state
        callable();
        once->store(GTHREAD_ONCE_STATE_INITIALIZED, std::memory_order_release);
    } else {
        // Wait for some other thread to complete initialization
        while (once->load(std::memory_order_acquire) == GTHREAD_ONCE_STATE_RUNNING) {
            HAL_Delay_Milliseconds(0);
        }
    }

    return 0;
}