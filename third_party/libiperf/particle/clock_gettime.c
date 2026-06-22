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

#include "iperf_config.h"

#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>

#include "timer_hal.h"

int clock_gettime(clockid_t clock_id, struct timespec* tp) {
    if (!tp) {
        errno = EFAULT;
        return -1;
    }
    switch (clock_id) {
        case CLOCK_MONOTONIC: {
            uint64_t us = hal_timer_micros(NULL);
            tp->tv_sec = us / 1000000ULL;
            tp->tv_nsec = (long)(us % 1000000ULL) * 1000L;
            return 0;
        }
        case CLOCK_REALTIME: {
            struct timeval tv;
            if (gettimeofday(&tv, NULL) != 0) {
                return -1;
            }
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = (long)tv.tv_usec * 1000L;
            return 0;
        }
        default: {
            errno = EINVAL;
            return -1;
        }
    }
}
