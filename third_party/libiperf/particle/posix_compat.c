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

#include <string.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <stdint.h>
#include <netinet/in.h>

#include "timer_hal.h"

const struct in6_addr in6addr_any = {{{0}}};

int getrusage(int who, struct rusage* usage) {
    (void)who;
    if (usage) {
        memset(usage, 0, sizeof(*usage));
    }
    return 0;
}

clock_t _times(struct tms* buf) {
    if (buf) {
        memset(buf, 0, sizeof(*buf));
    }
    return (clock_t)(hal_timer_millis(NULL) / 10);
}
