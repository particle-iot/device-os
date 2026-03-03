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

#if HAL_PLATFORM_EXTERNAL_RTC

#include "am18x5.h"

// Tracker
void hal_exrtc_get_watchdog_limits_deprecated(system_tick_t* low, system_tick_t* high, void* reserved) {
    Am18x5::getInstance().getWatchdogLimits(low, high);
}

int hal_exrtc_enable_watchdog_deprecated(system_tick_t ms, void* reserved) {
    return Am18x5::getInstance().enableWatchdog(ms);
}

int hal_exrtc_disable_watchdog_deprecated(void* reserved) {
    return Am18x5::getInstance().disableWatchdog();
}

int hal_exrtc_feed_watchdog_deprecated(void* reserved) {
    return Am18x5::getInstance().feedWatchdog();
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
