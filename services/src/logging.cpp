/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "timer_hal.h"
#include "service_debug.h"

namespace {

log_message_callback_type log_msg_callback = 0;
log_write_callback_type log_write_callback = 0;
log_enabled_callback_type log_enabled_callback = 0;

} // namespace

void log_set_callbacks(log_message_callback_type log_msg, log_write_callback_type log_write,
        log_enabled_callback_type log_enabled, void *reserved) {
    log_msg_callback = log_msg;
    log_write_callback = log_write;
    log_enabled_callback = log_enabled;
}

void log_message(int level, const char *category, const char *file, int line, const char *func, void *reserved,
        const char *fmt, ...) {
    if (log_msg_callback) {
        const uint32_t time = HAL_Timer_Get_Milli_Seconds();
        if (file) {
            // Strip directory path
            const char *p = strrchr(file, '/');
            if (p) {
                file = p + 1;
            }
        }
        char buf[LOG_FORMATTED_STRING_LENGTH];
        va_list args;
        va_start(args, fmt);
        const int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n > (int)sizeof(buf) - 1) {
            buf[sizeof(buf) - 2] = '~';
        }
        log_msg_callback(buf, level, category, time, file, line, func, 0);
    } else if (debug_output_ && level >= log_level_at_run_time) { // Compatibility callback
        const uint32_t time = HAL_Timer_Get_Milli_Seconds();
        const char* const levelName = log_level_name(level, 0);
        if (file) {
            const char *p = strrchr(file, '/');
            if (p) {
                file = p + 1;
            }
        }
        char buf[LOG_FORMATTED_STRING_LENGTH];
        int n = 0;
        if (file && func) {
            n = snprintf(buf, sizeof(buf), "%010u %s:%d, %s: %s", (unsigned)time, file, line, func, levelName);
        } else {
            n = snprintf(buf, sizeof(buf), "%010u %s", (unsigned)time, levelName);
        }
        if (n > (int)sizeof(buf) - 1) {
            buf[sizeof(buf) - 2] = '~';
        }
        debug_output_(buf);
        debug_output_(": ");
        va_list args;
        va_start(args, fmt);
        n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n > (int)sizeof(buf) - 1) {
            buf[sizeof(buf) - 2] = '~';
        }
        debug_output_(buf);
        debug_output_("\r\n");
    }
}

void log_write(int level, const char *category, const char *data, size_t size, void *reserved) {
    if (!log_write_callback || !size) {
        return;
    }
    log_write_callback(data, size, level, category, 0);
}

void log_format(int level, const char *category, void *reserved, const char *fmt, ...) {
    if (!log_write_callback && (!debug_output_ || level < log_level_at_run_time)) {
        return;
    }
    char buf[LOG_FORMATTED_STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > (int)sizeof(buf) - 1) {
        buf[sizeof(buf) - 2] = '~';
        n = sizeof(buf) - 1;
    }
    if (log_write_callback) {
        log_write_callback(buf, n, level, category, 0);
    } else {
        debug_output_(buf); // Compatibility callback
    }
}

void log_dump(int level, const char *category, const void *data, size_t size, void *reserved) {
    if (!log_write_callback || !size) {
        return;
    }
    static const char hex[] = "0123456789abcdef";
    char buf[32 * 2]; // Hex data is flushed in chunks
    size_t offs = 0;
    for (size_t i = 0; i < size; ++i) {
        const uint8_t b = ((const uint8_t*)data)[i];
        buf[offs++] = hex[b >> 4];
        buf[offs++] = hex[b & 0x0f];
        if (offs == sizeof(buf)) {
            log_write_callback(buf, sizeof(buf), level, category, 0);
            offs = 0;
        }
    }
    if (offs) {
        log_write_callback(buf, offs, level, category, 0);
    }
}

int log_enabled(int level, const char *category, void *reserved) {
    if (log_enabled_callback) {
        return log_enabled_callback(level, category, 0);
    }
    if (debug_output_ && level >= log_level_at_run_time) { // Compatibility callback
        return 1;
    }
    return 0;
}

const char* log_level_name(int level, void *reserved) {
    static const char* const names[] = {
        "TRACE",
        "LOG",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "PANIC"
    };
    const int i = std::max(0, std::min<int>(level / 10, sizeof(names) / sizeof(names[0]) - 1));
    return names[i];
}
