/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include <stdlib.h>
#include <stdint.h>
#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void __malloc_lock(struct _reent *ptr);
void __malloc_unlock(struct _reent *ptr);

void malloc_enable(uint8_t val);

void malloc_set_heap_start(void* addr);
void* malloc_heap_start();
void malloc_set_heap_end(void* addr);
void* malloc_heap_end();

typedef struct malloc_heap_region {
    void* start;
    void* end;
} malloc_heap_region;

extern malloc_heap_region malloc_heap_regions[HAL_PLATFORM_HEAP_REGIONS];

void malloc_set_heap_regions(const malloc_heap_region* regions, size_t count);

#ifdef __cplusplus
}
#endif // __cplusplus
