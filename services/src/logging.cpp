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
#include "timer_hal.h"
#include "hal_platform.h"
#include "service_debug.h"
#include "static_assert.h"

#include "../../system/src/boot_log.h" // FIXME

#define STATIC_ASSERT_FIELD_SIZE(struct, field, size) \
        STATIC_ASSERT(field_size_changed_##struct##_##field, sizeof(struct::field) == size);

#define STATIC_ASSERT_FIELD_OFFSET(struct, field, offset) \
        STATIC_ASSERT(field_offset_changed_##struct##_##field, offsetof(struct, field) == offset);

#define STATIC_ASSERT_FIELD_ORDER(struct, field1, field2) \
        STATIC_ASSERT(field_offset_changed_##struct##_##field2, \
                offsetof(struct, field2) == offsetof(struct, field1) + sizeof(struct::field1) + /* Padding */ \
                (__alignof__(struct::field2) - (offsetof(struct, field1) + sizeof(struct::field1)) % \
                __alignof__(struct::field2)) % __alignof__(struct::field2));

// LogAttributes::size
STATIC_ASSERT_FIELD_SIZE(LogAttributes, size, sizeof(size_t));
STATIC_ASSERT_FIELD_OFFSET(LogAttributes, size, 0);
// LogAttributes::flags
STATIC_ASSERT_FIELD_SIZE(LogAttributes, flags, sizeof(uint32_t));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, size, flags);
// LogAttributes::file
STATIC_ASSERT_FIELD_SIZE(LogAttributes, file, sizeof(const char*));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, flags, file);
// LogAttributes::line
STATIC_ASSERT_FIELD_SIZE(LogAttributes, line, sizeof(int));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, file, line);
// LogAttributes::function
STATIC_ASSERT_FIELD_SIZE(LogAttributes, function, sizeof(const char*));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, line, function);
// LogAttributes::time
STATIC_ASSERT_FIELD_SIZE(LogAttributes, time, sizeof(uint32_t));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, function, time);
// LogAttributes::code
STATIC_ASSERT_FIELD_SIZE(LogAttributes, code, sizeof(intptr_t));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, time, code);
// LogAttributes::details
STATIC_ASSERT_FIELD_SIZE(LogAttributes, details, sizeof(const char*));
STATIC_ASSERT_FIELD_ORDER(LogAttributes, code, details);
// LogAttributes::end
STATIC_ASSERT_FIELD_ORDER(LogAttributes, details, end);

namespace {

volatile log_message_callback_type log_msg_callback = 0;
volatile log_write_callback_type log_write_callback = 0;
volatile log_enabled_callback_type log_enabled_callback = 0;

#if HAL_PLATFORM_BOOT_LOG
using namespace particle::system;
#else
inline void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
}

inline void writeBootLog(const char* data, size_t size, int level, const char* category) {
}

inline bool isBootLogEnabledForLevel(int level, const char* category) {
    return false;
}

inline bool isBootLogEnabled() {
    return false;
}
#endif // !HAL_PLATFORM_BOOT_LOG

} // namespace

void log_set_callbacks(log_message_callback_type log_msg, log_write_callback_type log_write,
        log_enabled_callback_type log_enabled, void *reserved) {
    log_msg_callback = log_msg;
    log_write_callback = log_write;
    log_enabled_callback = log_enabled;
}

void log_message_v(int level, const char *category, LogAttributes *attr, void *reserved, const char *fmt, va_list args) {
    if (!log_msg_callback && !isBootLogEnabled()) {
        return;
    }
    // Set default attributes
    if (!attr->has_time) {
        LOG_ATTR_SET(*attr, time, HAL_Timer_Get_Milli_Seconds());
    }
    char buf[LOG_MAX_STRING_LENGTH];
    const int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > (int)sizeof(buf) - 1) {
        buf[sizeof(buf) - 2] = '~';
    }
    if (log_msg_callback) {
        log_msg_callback(buf, level, category, attr, nullptr /* reserved */);
    }
    bootLogMessage(buf, level, category, attr);
}

void log_message(int level, const char *category, LogAttributes *attr, void *reserved, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message_v(level, category, attr, reserved, fmt, args);
    va_end(args);
}

void log_write(int level, const char *category, const char *data, size_t size, void *reserved) {
    if (!size) {
        return;
    }
    if (log_write_callback) {
        log_write_callback(data, size, level, category, nullptr /* reserved */);
    }
    writeBootLog(data, size, level, category);
}

void log_printf_v(int level, const char *category, void *reserved, const char *fmt, va_list args) {
    if (!log_write_callback && !isBootLogEnabled()) {
        return;
    }
    char buf[LOG_MAX_STRING_LENGTH];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > (int)sizeof(buf) - 1) {
        buf[sizeof(buf) - 2] = '~';
        n = sizeof(buf) - 1;
    }
    if (log_write_callback) {
        log_write_callback(buf, n, level, category, nullptr /* reserved */);
    }
    writeBootLog(buf, n, level, category);
}

void log_printf(int level, const char *category, void *reserved, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_printf_v(level, category, reserved, fmt, args);
    va_end(args);
}

void log_dump(int level, const char *category, const void *data, size_t size, int flags, void *reserved) {
    if (!size || (!log_write_callback && !isBootLogEnabled())) {
        return;
    }
    static const char hex[] = "0123456789abcdef";
    char buf[LOG_MAX_STRING_LENGTH / 2 * 2 + 1]; // Hex data is flushed in chunks
    buf[sizeof(buf) - 1] = 0; // Compatibility callback expects null-terminated strings
    size_t offs = 0;
    for (size_t i = 0; i < size; ++i) {
        const uint8_t b = ((const uint8_t*)data)[i];
        buf[offs++] = hex[b >> 4];
        buf[offs++] = hex[b & 0x0f];
        if (offs == sizeof(buf) - 1) {
            if (log_write_callback) {
                log_write_callback(buf, sizeof(buf) - 1, level, category, nullptr /* reserved */);
            }
            writeBootLog(buf, sizeof(buf) - 1, level, category);
            offs = 0;
        }
    }
    if (offs) {
        if (log_write_callback) {
            log_write_callback(buf, offs, level, category, nullptr /* reserved */);
        }
        writeBootLog(buf, offs, level, category);
    }
}

int log_enabled(int level, const char *category, void *reserved) {
    if (isBootLogEnabledForLevel(level, category)) {
        return 1;
    }
    if (log_enabled_callback) {
        return log_enabled_callback(level, category, nullptr /* reserved */);
    }
    return 0;
}

const char* log_level_name(int level, void *reserved) {
    static const char* const names[] = {
        "TRACE",
        "TRACE", // LOG (deprecated)
        "TRACE", // DEBUG (deprecated)
        "INFO",
        "WARN",
        "ERROR",
        "PANIC"
    };
    const int i = std::max(0, std::min<int>(level / 10, sizeof(names) / sizeof(names[0]) - 1));
    return names[i];
}
