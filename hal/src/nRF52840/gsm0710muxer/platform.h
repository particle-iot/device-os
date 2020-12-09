/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef GSM0710_PLATFORM_H
#define GSM0710_PLATFORM_H

#include "service_debug.h"
#include "concurrent_hal.h"
#include "timer_hal.h"
#include "check.h"
#include "stream.h"
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include <mutex>
#include "usart_hal_private.h"
#include "hal_platform.h"

namespace gsm0710 {
namespace portable {

inline uint64_t getMillis() {
    return hal_timer_millis(nullptr);
}

const auto taskStackSize = 4096;
const auto taskPriority = OS_THREAD_PRIORITY_NETWORK;

#define GSM0710_ASSERT(x) SPARK_ASSERT(x)

// #define GSM0710_MODEM_STATUS_SEND_EMPTY_BREAK
// #define GSM0710_PUMP_INPUT_DATA 50
// #define GSM0710_ENABLE_DEBUG_LOGGING

#define GSM0710_SHARED_STREAM_EVENT_GROUP
#define GSM0710_SHARED_STREAM_EVENT_BASE (__builtin_ffs(HAL_USART_PVT_EVENT_MAX))

#if HAL_PLATFORM_WIFI
#define GSM0710_FLOW_CONTROL_RTR
#endif // HAL_PLATFORM_WIFI

#define GSM0710_RELAX_WHEN_CHANNEL_IN_FLOW_CONTROL (15)

} // portable
} // gsm0710

#endif // GSM0710_PLATFORM_H
