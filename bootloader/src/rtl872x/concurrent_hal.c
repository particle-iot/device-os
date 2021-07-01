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

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "hal_irq_flag.h"

static struct hal_exflash_lock_state {
    atomic_int locked;
    int32_t irq_state;
} s_hal_exflash_lock_state;

int os_thread_yield(void) {
    return 0;
}

void __flash_acquire(void) {
}

void __flash_release(void) {
}

void periph_lock(void) {
}

void periph_unlock(void) {
}

int hal_exflash_lock(void) {
    if (atomic_fetch_add(&s_hal_exflash_lock_state.locked, 1) == 0) {
        s_hal_exflash_lock_state.irq_state = HAL_disable_irq();
    }
    return 0;
}

int hal_exflash_unlock(void) {
    if (atomic_fetch_sub(&s_hal_exflash_lock_state.locked, 1) == 1) {
        HAL_enable_irq(s_hal_exflash_lock_state.irq_state);
    }
    return 0;
}
