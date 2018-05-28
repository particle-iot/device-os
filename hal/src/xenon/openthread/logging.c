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

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include "logging.h"

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    int level;
    switch (aLogLevel) {
        case OT_LOG_LEVEL_CRIT: {
            level = LOG_LEVEL_PANIC;
            break;
        }

        case OT_LOG_LEVEL_WARN: {
            level = LOG_LEVEL_WARN;
            break;
        }

        case OT_LOG_LEVEL_INFO: {
            level = LOG_LEVEL_INFO;
            break;
        }

        case OT_LOG_LEVEL_DEBG:
        default: {
            level = LOG_LEVEL_TRACE;
            break;
        }
    }

    (void)aLogRegion;

    va_list args;
    va_start(args, aFormat);

    LogAttributes attr;
    attr.size = sizeof(LogAttributes);
    attr.flags = 0;

    log_message_v(level, "ot", &attr, NULL, aFormat, args);

    va_end(args);
}

#endif // (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
