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

#ifndef _LOGGING_H
#define _LOGGING_H

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "panic.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LogLevel {
    // Compatibility levels
    DEFAULT_LEVEL = 0,
    ALL_LEVEL = 1,
    TRACE_LEVEL = 1,
    LOG_LEVEL = 10,
    DEBUG_LEVEL = 20,
    INFO_LEVEL = 30,
    WARN_LEVEL = 40,
    ERROR_LEVEL = 50,
    PANIC_LEVEL = 60,
    NO_LOG_LEVEL = 70,
    // Public API
    LOG_LEVEL_ALL = ALL_LEVEL, // Log all messages
    LOG_LEVEL_TRACE = TRACE_LEVEL,
    LOG_LEVEL_INFO = INFO_LEVEL,
    LOG_LEVEL_WARN = WARN_LEVEL,
    LOG_LEVEL_ERROR = ERROR_LEVEL,
    LOG_LEVEL_PANIC = PANIC_LEVEL,
    LOG_LEVEL_NONE = NO_LOG_LEVEL // Do not log any messages
} LogLevel;

// Logger callbacks
typedef void (*log_message_callback_type)(const char *msg, int level, const char *category, uint32_t time,
        const char *file, int line, const char *func, void *reserved);
typedef void (*log_write_callback_type)(const char *data, size_t size, int level, const char *category, void *reserved);
typedef int (*log_enabled_callback_type)(int level, const char *category, void *reserved);

// Generates log message and forwards it to backend logger. This function performs only printf-alike
// formatting of 'fmt' string, all other parameters are forwarded as is
void log_message(int level, const char *category, const char *file, int line, const char *func, void *reserved,
        const char *fmt, ...);

// Forwards buffer to backend logger
void log_write(int level, const char *category, const char *data, size_t size, void *reserved);

// Performs printf-alike formatting and forwards resulting string to backend logger
void log_format(int level, const char *category, void *reserved, const char *fmt, ...);

// Encodes data as hex string and forwards it to backend logger
void log_dump(int level, const char *category, const void *data, size_t size, int flags, void *reserved);

// Returns 1 if logging is enabled for specified level and category
int log_enabled(int level, const char *category, void *reserved);

// Returns log level name
const char* log_level_name(int level, void *reserved);

// Sets logger callbacks
void log_set_callbacks(log_message_callback_type log_msg, log_write_callback_type log_write,
        log_enabled_callback_type log_enabled, void *reserved);

extern void HAL_Delay_Microseconds(uint32_t delay);

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL
#endif

#ifndef LOG_MAX_STRING_LENGTH
#define LOG_MAX_STRING_LENGTH 160
#endif

#ifndef LOG_DISABLED

#ifndef LOG_MODULE_CATEGORY
// #warning "Log category is not defined for a module"
#define LOG_MODULE_CATEGORY NULL
#endif

#ifdef __cplusplus

// Module category
template<typename T>
struct _LogCategoryWrapper {
    static const char* name() {
        return LOG_MODULE_CATEGORY;
    }
};

struct _LogGlobalCategory;
typedef _LogCategoryWrapper<_LogGlobalCategory> _LogCategory;

// Source file category
#define LOG_SOURCE_CATEGORY(_name) \
        template<> \
        struct _LogCategoryWrapper<_LogGlobalCategory> { \
            static const char* name() { \
                return _name; \
            } \
        };

// Scoped category
#define LOG_CATEGORY(_name) \
        struct _LogCategory { \
            static const char* name() { \
                return _name; \
            } \
        }

// Expands to current category name
#define _LOG_CATEGORY _LogCategory::name()

#else // !defined(__cplusplus)

// Module category
//
// All constants defined in this header file are static, and implementation expects that GCC
// merges identical string constants defined in different translation units during linking
// (requires optimization enabled)
static const char* const _log_module_category = LOG_MODULE_CATEGORY;

// Source file category
//
// weakref allows to have different implementations of the same function in different translation
// units if target function is declared as static
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static const char* _log_source_category() __attribute__((weakref("_log_source_category_impl")));
#pragma GCC diagnostic pop

#define LOG_SOURCE_CATEGORY(_name) \
        static const char* _log_source_category_impl() { \
            return _name; \
        }

// Dummy constant shadowed when scoped category is defined
static const char* const _log_category = NULL;

// Scoped category
#define LOG_CATEGORY(_name) \
        static const char* const _log_category = _name

// Expands to current category name
#define _LOG_CATEGORY \
        (_log_category ? _log_category : (_log_source_category ? _log_source_category() : _log_module_category))

#endif // !defined(__cplusplus)

#ifdef LOG_INCLUDE_SOURCE_INFO
#define _LOG_SOURCE_INFO __FILE__, __LINE__, __PRETTY_FUNCTION__
#else
#define _LOG_SOURCE_INFO NULL, 0, NULL
#endif

// Wrapper macros
#define LOG_C(_level, _category, _fmt, ...) \
        do { \
            if (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL) { \
                log_message(_level##_LEVEL, _category, _LOG_SOURCE_INFO, NULL, _fmt, ##__VA_ARGS__); \
            } \
        } while (0)

#define LOG_WRITE_C(_level, _category, _data, _size) \
        do { \
            if (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL) { \
                log_write(_level##_LEVEL, _category, _data, _size, NULL); \
            } \
        } while (0)

#define LOG_FORMAT_C(_level, _category, _fmt, ...) \
        do { \
            if (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL) { \
                log_format(_level##_LEVEL, _category, NULL, _fmt, ##__VA_ARGS__); \
            } \
        } while (0)

#define LOG_PRINT_C(_level, _category, _str) \
        do { \
            if (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL) { \
                const char* const _s = _str; \
                log_write(_level##_LEVEL, _category, _s, strlen(_s), NULL); \
            } \
        } while (0)

#define LOG_DUMP_C(_level, _category, _data, _size) \
        do { \
            if (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL) { \
                log_dump(_level##_LEVEL, _category, _data, _size, 0, NULL); \
            } \
        } while (0)

#define LOG_ENABLED_C(_level, _category) \
        (_level##_LEVEL >= LOG_COMPILE_TIME_LEVEL && log_enabled(_level##_LEVEL, _category, NULL))

// Macros using current category
#define LOG(_level, _fmt, ...) LOG_C(_level, _LOG_CATEGORY, _fmt, ##__VA_ARGS__)
#define LOG_WRITE(_level, _data, _size) LOG_WRITE_C(_level, _LOG_CATEGORY, _data, _size)
#define LOG_FORMAT(_level, _fmt, ...) LOG_FORMAT_C(_level, _LOG_CATEGORY, _fmt, ##__VA_ARGS__)
#define LOG_PRINT(_level, _str) LOG_PRINT_C(_level, _LOG_CATEGORY, _str)
#define LOG_DUMP(_level, _data, _size) LOG_DUMP_C(_level, _LOG_CATEGORY, _data, _size)
#define LOG_ENABLED(_level) LOG_ENABLED_C(_level, _LOG_CATEGORY)

#else // LOG_DISABLED

#define LOG_CATEGORY(_name)
#define LOG_SOURCE_CATEGORY(_name)

#define LOG(_level, _fmt, ...)
#define LOG_C(_level, _category, _fmt, ...)
#define LOG_WRITE(_level, _data, _size)
#define LOG_WRITE_C(_level, _category, _data, _size)
#define LOG_FORMAT(_level, _fmt, ...)
#define LOG_FORMAT_C(_level, _category, _fmt, ...)
#define LOG_PRINT(_level, _str)
#define LOG_PRINT_C(_level, _category, _str)
#define LOG_DUMP(_level, _data, _size)
#define LOG_DUMP_C(_level, _category, _data, _size)
#define LOG_ENABLED(_level) (0)
#define LOG_ENABLED_C(_level, _category) (0)

#endif

#ifdef DEBUG_BUILD
#define LOG_DEBUG(_level, _fmt, ...) LOG(_level, _fmt, ##__VA_ARGS__)
#define LOG_DEBUG_C(_level, _category, _fmt, ...) LOG_C(_level, _category, _fmt, ##__VA_ARGS__)
#define LOG_DEBUG_WRITE(_level, _data, _size) LOG_WRITE(_level, _data, _size)
#define LOG_DEBUG_WRITE_C(_level, _category, _data, _size) LOG_WRITE_C(_level, _category, _data, _size)
#define LOG_DEBUG_FORMAT(_level, _fmt, ...) LOG_FORMAT(_level, _fmt, ##__VA_ARGS__)
#define LOG_DEBUG_FORMAT_C(_level, _category, _fmt, ...) LOG_FORMAT_C(_level, _category, _fmt, ##__VA_ARGS__)
#define LOG_DEBUG_PRINT(_level, _str) LOG_PRINT(_level, _str)
#define LOG_DEBUG_PRINT_C(_level, _category, _str) LOG_PRINT_C(_level, _category, _str)
#define LOG_DEBUG_DUMP(_level, _data, _size) LOG_DUMP(_level, _data, _size)
#define LOG_DEBUG_DUMP_C(_level, _category, _data, _size) LOG_DUMP_C(_level, _category, _data, _size)
#else
#define LOG_DEBUG(_level, _fmt, ...)
#define LOG_DEBUG_C(_level, _category, _fmt, ...)
#define LOG_DEBUG_WRITE(_level, _data, _size)
#define LOG_DEBUG_WRITE_C(_level, _category, _data, _size)
#define LOG_DEBUG_FORMAT(_level, _fmt, ...)
#define LOG_DEBUG_FORMAT_C(_level, _category, _fmt, ...)
#define LOG_DEBUG_PRINT(_level, _str)
#define LOG_DEBUG_PRINT_C(_level, _category, _str)
#define LOG_DEBUG_DUMP(_level, _data, _size)
#define LOG_DEBUG_DUMP_C(_level, _category, _data, _size)
#endif

#define PANIC(_code, _fmt, ...) \
        do { \
            LOG(PANIC, _fmt, ##__VA_ARGS__); \
            panic_(_code, NULL, HAL_Delay_Microseconds); \
        } while (0)

#endif // _LOGGING_H
