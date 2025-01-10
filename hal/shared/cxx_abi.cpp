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

#include <cxxabi.h>
#include <stdint.h>
#include "service_debug.h"
#include "static_recursive_mutex.h"
#include "interrupts_hal.h"
#include "static_event_group.h"
#include "concurrent_hal.h"
#include <mutex>

/* __guard type is already defined in cxxabi_tweaks.h for each architecture */
using __cxxabiv1::__guard;

namespace {

/* Using a global recursive mutex, should be enough for our use-cases */
StaticRecursiveMutex s_mutex;
StaticEventGroup s_event_group;

struct __attribute__((packed)) guard_t {
    /* Normally this is:
    *  Guard Object Layout:
    * ---------------------------------------------------------------------------
    * | a+0: guard byte | a+1: init byte | a+2: unused ... | a+4: thread-id ... |
    * ---------------------------------------------------------------------------
    * On ARM this is just 4 bytes
    * */
    uint8_t done;
    uint8_t init;
    uint8_t wait_count;

    enum GuardFlags : uint8_t {
        NONE = 0,
        COMPLETE = 0x01,
        PENDING = 0x02,
        WAITING = 0x04
    };
};

static_assert(sizeof(guard_t) <= sizeof(__guard), "guard is too large");

} /* anonymous */

/* http://refspecs.linuxbase.org/cxxabi-1.86.html#once-ctor */

extern "C" {

int __cxa_guard_acquire(__guard* g) {
    guard_t* guard = reinterpret_cast<guard_t*>(g);

    SPARK_ASSERT(!hal_interrupt_is_isr());

    /* Acquire mutex */
    std::unique_lock<StaticRecursiveMutex> lk(s_mutex);

    while (true) {
        // Nothing to do here, already initialized
        if (guard->done) {
            return 0;
        }

        if (guard->init & guard_t::PENDING) {
            // Pending initialization
            // We need to wait for initialization to complete
            // Scheduler MUST be running
            auto scheduler = os_scheduler_get_state(nullptr);
            SPARK_ASSERT(scheduler == OS_SCHEDULER_STATE_RUNNING);

            guard->init |= guard_t::WAITING;
            guard->wait_count++;
            lk.unlock();
            s_event_group.wait(guard_t::COMPLETE, StaticEventGroup::Flag::CLEAR_ON_EXIT);
            lk.lock();
            guard->wait_count--;
            if (guard->wait_count == 0) {
                s_event_group.set(guard_t::WAITING);
            }
        } else {
            // Set pending flag continue with initialization
            guard->init |= guard_t::PENDING;
            return 1;
        }
    }
    return 0;
}

void __cxa_guard_release(__guard* g) {
    guard_t* guard = reinterpret_cast<guard_t*>(g);
    SPARK_ASSERT(!hal_interrupt_is_isr());

    std::unique_lock<StaticRecursiveMutex> lk(s_mutex);
    guard->init &= ~(guard_t::PENDING);
    guard->done = guard_t::COMPLETE;
    guard->init |= guard_t::COMPLETE;
    if ((guard->init & guard_t::WAITING) && guard->wait_count > 0) {
        // This will wake all threads waiting on initialization completion
        s_event_group.set(guard_t::COMPLETE);
    } else {
        return;
    }

    while (guard->wait_count > 0) {
        lk.unlock();
        // FIXME: not ideal, but we'll spin with 10ms delay
        auto ev = s_event_group.sync(guard_t::COMPLETE, guard_t::WAITING, 10);
        lk.lock();
        if (ev & guard_t::WAITING) {
            s_event_group.clear(guard_t::WAITING);
        }
    }
}

void __cxa_guard_abort (__guard*) {
    /* We do not have exceptions enabled. This should not have happened */
    SPARK_ASSERT(false);
}

void __cxa_pure_virtual() {
    PANIC(PureVirtualCall, "Call on pure virtual");
    while (1);
}

} // extern "C"
