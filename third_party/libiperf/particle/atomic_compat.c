/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "hal_irq_flag.h"

#ifndef ATOMIC_WEAK
#define ATOMIC_WEAK __attribute__((weak))
#endif // ATOMIC_WEAK

ATOMIC_WEAK uint64_t __atomic_load_8(const volatile void* ptr, int memorder) {
    (void)memorder;
    int st = HAL_disable_irq();
    uint64_t val = *(const volatile uint64_t*)ptr;
    HAL_enable_irq(st);
    return val;
}

ATOMIC_WEAK void __atomic_store_8(volatile void* ptr, uint64_t val, int memorder) {
    (void)memorder;
    int st = HAL_disable_irq();
    *(volatile uint64_t*)ptr = val;
    HAL_enable_irq(st);
}

ATOMIC_WEAK uint64_t __atomic_exchange_8(volatile void* ptr, uint64_t val, int memorder) {
    (void)memorder;
    int st = HAL_disable_irq();
    uint64_t old = *(volatile uint64_t*)ptr;
    *(volatile uint64_t*)ptr = val;
    HAL_enable_irq(st);
    return old;
}

ATOMIC_WEAK uint64_t __atomic_fetch_add_8(volatile void* ptr, uint64_t val, int memorder) {
    (void)memorder;
    int st = HAL_disable_irq();
    uint64_t old = *(volatile uint64_t*)ptr;
    *(volatile uint64_t*)ptr = old + val;
    HAL_enable_irq(st);
    return old;
}

ATOMIC_WEAK uint64_t __atomic_fetch_sub_8(volatile void* ptr, uint64_t val, int memorder) {
    (void)memorder;
    int st = HAL_disable_irq();
    uint64_t old = *(volatile uint64_t*)ptr;
    *(volatile uint64_t*)ptr = old - val;
    HAL_enable_irq(st);
    return old;
}

ATOMIC_WEAK bool __atomic_compare_exchange_8(volatile void* ptr, void* expected, uint64_t desired,
        bool weak, int success_memorder, int failure_memorder) {
    (void)weak;
    (void)success_memorder;
    (void)failure_memorder;
    bool ok = false;
    int st = HAL_disable_irq();
    uint64_t old = *(volatile uint64_t*)ptr;
    if (old == *(uint64_t*)expected) {
        *(volatile uint64_t*)ptr = desired;
        ok = true;
    } else {
        *(uint64_t*)expected = old;
    }
    HAL_enable_irq(st);
    return ok;
}
