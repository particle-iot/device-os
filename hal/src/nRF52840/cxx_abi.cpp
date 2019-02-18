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

/* __guard type is already defined in cxxabi_tweaks.h for each architecture */
using __cxxabiv1::__guard;

namespace {

/* Using a global recursive mutex, should be enough for our use-cases */
StaticRecursiveMutex s_mutex;

struct __attribute__((packed)) guard_t {
    uint8_t done;
};

} /* anonymous */

/* http://refspecs.linuxbase.org/cxxabi-1.86.html#once-ctor */

extern "C" {

int __cxa_guard_acquire(__guard* g) {
    guard_t* guard = reinterpret_cast<guard_t*>(g);

    SPARK_ASSERT(!HAL_IsISR());

    /* Acquire mutex */
    SPARK_ASSERT(s_mutex.lock());
    if (!guard->done) {
        /* If there was no other thread doing the initialization, keep the lock and return 1.
         * Mutex will be released in __cxa_guard_release()
         */
        return 1;
    }

    /* If we were waiting for some other thread to finish initialization,
     * immediately unlock the mutex here and return 0 */
    s_mutex.unlock();

    return 0;
}

void __cxa_guard_release(__guard* g) {
    guard_t* guard = reinterpret_cast<guard_t*>(g);
    SPARK_ASSERT(!HAL_IsISR());
    SPARK_ASSERT(!guard->done);
    /* We were doing the initialization, so unlock the mutex */
    guard->done = 1;
    SPARK_ASSERT(s_mutex.unlock());
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
