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

#include "exflash_hal.h"
#include "hal_irq_flag.h"
#include "rtl8721d.h"

// Do not define Particle's STATIC_ASSERT() to avoid conflicts with the nRF SDK's own macro
#define NO_STATIC_ASSERT
#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#include "static_recursive_mutex.h"

/* Because non-trivial designated initializers are not supported in GCC,
 * we can't have exflash_hal.c as exflash_hal.cpp and thus can't use
 * StaticRecursiveMutex in it.
 *
 * Initialize mutex here and provide helper functions for locking
 */
static StaticRecursiveMutex s_exflash_mutex;

__attribute__((section(".ram.text"), noinline))
int hal_exflash_lock(void) {
    return !s_exflash_mutex.lock();
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_unlock(void) {
    return !s_exflash_mutex.unlock();
}

#else

#include "service_debug.h"
// FIXME: figure out panic_, assertion failures etc on KM0 side
#if defined(MODULE_INDEX) && MODULE_INDEX > 0
#undef SPARK_ASSERT
#define SPARK_ASSERT(predicate)
#endif // defined(MODULE_INDEX) && MODULE_INDEX > 0
#include "static_recursive_cs.h"

static particle::StaticRecursiveCriticalSectionLock s_exflash_cs;
static uint32_t s_systick_ctrl = 0;

__attribute__((section(".ram.text"), noinline))
int hal_exflash_lock(void) {
    s_exflash_cs.lock();
    if (s_exflash_cs.counter() == 1) {
        s_systick_ctrl = SysTick->CTRL;
    }
    return 0;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_unlock(void) {
    if (s_exflash_cs.counter() == 1) {
        SysTick->CTRL = s_systick_ctrl;
    }
    s_exflash_cs.unlock();
    return 0;
}

#endif /* MODULE_FUNCTION != MOD_FUNC_BOOTLOADER */
