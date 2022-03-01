/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "rtc_hal.h"
#include "boost_posix_time_wrap.h"
#include "check.h"
#include "system_error.h"
#include <time.h>
#include <stdlib.h>

namespace {

void ptimeToTimeval(struct timeval* tv, boost::posix_time::ptime pt) {
    static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    auto diff = pt - epoch;
    tv->tv_sec = diff.ticks() / diff.ticks_per_second();
    tv->tv_usec = diff.fractional_seconds();
}

}
// anonymous

void hal_rtc_init(void) {
    // UTC by default
    setenv("TZ", "", 1);
    tzset();
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto now = boost::posix_time::microsec_clock::universal_time();
    ptimeToTimeval(tv, now);
    return 0;
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    return 0;
}

bool hal_rtc_time_is_valid(void* reserved) {
    return true;
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void hal_rtc_cancel_alarm(void) {
}
