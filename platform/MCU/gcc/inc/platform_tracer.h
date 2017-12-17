/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#ifndef PLATFORM_TRACER_H
#define PLATFORM_TRACER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

inline void* platform_get_current_pc(void) {
    return NULL;
}

#define __SW_RETURN_ADDRESS(i) case i: return __builtin_return_address(i)

inline void* platform_get_return_address(int idx) {
    switch(idx) {
        __SW_RETURN_ADDRESS(0);
        __SW_RETURN_ADDRESS(1);
        __SW_RETURN_ADDRESS(2);
        __SW_RETURN_ADDRESS(3);
    };
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_TRACER_H
