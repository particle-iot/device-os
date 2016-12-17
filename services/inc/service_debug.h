/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef SERVICES_DEBUG_H_
#define SERVICES_DEBUG_H_

/*
 * NOTE: This module is here for compatibility purposes, consider using functions and macros
 * defined in logging.h instead.
 *
 * The API:
 *
 * DEBUG(fmt,...)
 * WARN(fmt,...)
 * ERROR(fmt,...)
 * PANIC(code,fmt,...)
 */

#include "logging.h"

#if PLATFORM_ID == 3
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef LogLevel LoggerOutputLevel; // Compatibility typedef
typedef void(*debug_output_fn)(const char*);

extern LoggerOutputLevel log_compat_level;
extern debug_output_fn log_compat_callback;

// This function sets logging callback and global logging level that are used for binary compatibility
// with existent applications
void set_logger_output(debug_output_fn output, LoggerOutputLevel level);

// These functions do nothing. Use alternatives declared in logging.h header instead
void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...);
void log_print_direct_(int level, void* reserved, const char *msg, ...);

void set_thread_current_function_pointers(void* c1, void* c2, void* c3, void* c4, void* c5);
typedef uint8_t(*thread_current_fn)(void*);
extern thread_current_fn system_thread_current_;
extern thread_current_fn main_thread_current_;
extern thread_current_fn application_thread_current_;

#ifdef __cplusplus
}
#endif

#define DEBUG(fmt, ...) LOG_DEBUG(TRACE, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) LOG(INFO, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG(WARN, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG(ERROR, fmt, ##__VA_ARGS__)
#define DEBUG_D(fmt, ...) LOG_DEBUG_PRINTF(TRACE, fmt, ##__VA_ARGS__)

#if PLATFORM_ID != 3
#define SPARK_ASSERT(predicate) do { if (!(predicate)) PANIC(AssertionFailure,"AssertionFailure "#predicate);} while(0);
#else
#define SPARK_ASSERT(predicate) assert(predicate)
#endif

#if defined(DEBUG_BUILD) && defined(DEBUG_THREADING) && PLATFORM_THREADING == 1
# if defined(MODULE_INDEX) && MODULE_INDEX == 1
#  define ASSERT_ON_SYSTEM_THREAD() SPARK_ASSERT(system_thread_current_(nullptr))
#  define ASSERT_ON_SYSTEM_OR_MAIN_THREAD() SPARK_ASSERT(system_thread_current_(nullptr) || main_thread_current_(nullptr))
# else
#  define ASSERT_ON_SYSTEM_THREAD() SPARK_ASSERT(system_thread_current(nullptr))
#  define ASSERT_ON_SYSTEM_OR_MAIN_THREAD() SPARK_ASSERT(system_thread_current(nullptr) || main_thread_current(nullptr))
# endif
#else
# define ASSERT_ON_SYSTEM_THREAD()
# define ASSERT_ON_SYSTEM_OR_MAIN_THREAD()
#endif // defined(DEBUG_BUILD) && defined(DEBUG_THREADING) && PLATFORM_THREADING == 1

#endif /* SERVICES_DEBUG_H_ */
