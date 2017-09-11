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

#ifndef PLATFORM_DIAGNOSTIC_H
#define PLATFORM_DIAGNOSTIC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noinline__)) void* platform_get_current_pc(void);

inline void* platform_get_return_address(int idx) {
    return __builtin_return_address(idx);
}

#ifdef __cplusplus
}
#endif

inline bool platform_is_branching_instruction(void* ptr)
{
  uintptr_t p = *((uintptr_t*)ptr);
  if ((p & 0xff800000) == 0x47800000 || (p & 0xe0000000) == 0xe0000000) {
    return true;
  }

  return false;
}

#if defined(STM32F2XX)

#define PLATFORM_DIAGNOSTIC_ENABLED 1

extern char link_global_retained_system_end;
extern char link_global_retained_system_end_section;

#define DIAGNOSTIC_LOCATION_BEGIN (&link_global_retained_system_end)
#define DIAGNOSTIC_LOCATION_END (&link_global_retained_system_end_section)
// Due to the fact that we need DIAGNOSTIC_LOCATION_SIZE to be a constexpr, and a
// difference between two pointers is not constexpr, we define DIAGNOSTIC_LOCATION_SIZE manually here
// and verify that we fit using an assert in linker file
#if PLATFORM_ID != 10
# define DIAGNOSTIC_LOCATION_SIZE 1024
#else
# define DIAGNOSTIC_LOCATION_SIZE 804
#endif

#endif

#endif // PLATFORM_DIAGNOSTIC_H
