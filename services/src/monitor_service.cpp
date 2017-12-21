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

#define SYSTEM_MONITOR_SKIP_PLATFORM
#include "monitor_service.h"

#if defined(SYSTEM_MONITOR_USE_CALLBACKS)

static system_monitor_callbacks_t s_callbacks = {};

int system_monitor_set_callbacks(system_monitor_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        s_callbacks = *cb;
        return 0;
    }

    return 1;
}

int system_monitor_kick_current_(void* reserved) {
    if (s_callbacks.system_monitor_kick_current != nullptr) {
        return s_callbacks.system_monitor_kick_current(reserved);
    }

    return 1;
}

system_tick_t system_monitor_get_timeout_current_(void* reserved) {
    if (s_callbacks.system_monitor_get_timeout_current != nullptr) {
        return s_callbacks.system_monitor_get_timeout_current(reserved);
    }

    return 1;
}

int system_monitor_set_timeout_current_(system_tick_t timeout, void* reserved) {
    if (s_callbacks.system_monitor_set_timeout_current != nullptr) {
        return s_callbacks.system_monitor_set_timeout_current(timeout, reserved);
    }

    return 1;
}

#endif /* defined(SYSTEM_MONITOR_USE_CALLBACKS) */

#if defined(SYSTEM_MONITOR_USE_CALLBACKS2)

static system_monitor_callbacks_t s_callbacks2 = {};

int system_monitor_set_callbacks_(system_monitor_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        s_callbacks2 = *cb;
        return 0;
    }

    return 1;
}

int system_monitor_kick_current__(void* reserved) {
    if (s_callbacks2.system_monitor_kick_current != nullptr) {
        return s_callbacks2.system_monitor_kick_current(reserved);
    }

    return 1;
}

system_tick_t system_monitor_get_timeout_current__(void* reserved) {
    if (s_callbacks2.system_monitor_get_timeout_current != nullptr) {
        return s_callbacks2.system_monitor_get_timeout_current(reserved);
    }

    return 1;
}

int system_monitor_set_timeout_current__(system_tick_t timeout, void* reserved) {
    if (s_callbacks2.system_monitor_set_timeout_current != nullptr) {
        return s_callbacks2.system_monitor_set_timeout_current(timeout, reserved);
    }

    return 1;
}

#endif /* defined(SYSTEM_MONITOR_USE_CALLBACKS2) */
