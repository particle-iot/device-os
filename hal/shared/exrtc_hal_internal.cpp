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

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "exrtc_hal.h"
#include "rtc_hal.h"
#include "check.h"

// Just mimicking rtc_hal to simplify rtc_hal <-> exrtc_hal coupling

// FIXME: This is again all very AM18x5-specific, but good enough for now

int hal_exrtc_init(void) {
    // TODO
}

int hal_exrtc_get_time(struct timeval* tv) {
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().getTime(tv);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_set_time(const struct timeval* tv) {
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().setTime(tv);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_cancel_alarm(void) {
    if (Am18x5::getInstance().isDefault()) {
        Am18x5::getInstance().setAlarm(false);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
