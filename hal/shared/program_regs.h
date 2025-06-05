/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#include <stdint.h>

#ifdef __arm__

__attribute__((always_inline)) static inline uint32_t __get_LR(void) { 
    uint32_t result; 

    asm volatile ("mov %0, lr\n" : "=r" (result) ); 
    return result;
}

__attribute__((always_inline)) static inline uint32_t __get_PC(void) { 
    uint32_t result; 

    asm volatile ("mov %0, pc\n" : "=r" (result) ); 
    return result;
}

#else

__attribute__((always_inline)) static inline uint32_t __get_LR(void) { 
    return 0;
}

__attribute__((always_inline)) static inline uint32_t __get_PC(void) { 
    return 0;
}

#endif // __arm__