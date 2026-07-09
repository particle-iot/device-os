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

#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "delay_hal.h"
#include "system_tick_hal.h"

int nanosleep(const struct timespec* req, struct timespec* rem) {
    if (!req || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }
    uint64_t ms = (uint64_t)req->tv_sec * 1000 + ((uint64_t)req->tv_nsec + 999999) / 1000000;
    if (ms > 0) {
        HAL_Delay_Milliseconds((system_tick_t)ms);
    }
    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}
