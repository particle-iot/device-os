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

#if defined(DEBUG_BUILD)
#define LOG_LEVEL_AT_RUN_TIME LOG_LEVEL         // Set to allow all LOG_LEVEL and above messages to be displayed conditionally by levels.
#endif

#if defined(RELEASE_BUILD)
#define LOG_LEVEL_AT_RUN_TIME ERROR_LEVEL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef LogLevel LoggerOutputLevel; // Compatibility typedef

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...);

void log_direct_(const char* s);

/* log print with (formatting arguments) without any extra info or \r\n */
void log_print_direct_(int level, void* reserved, const char *msg, ...);

/**
 * The debug output function.
 */
typedef void (*debug_output_fn)(const char *);

/**
 * Set the debug logger function and optionally the logging level.
 * @param output                The output function. `NULL` to use the existing function.
 * @param debug_output_level    The log level to output.
 */
void set_logger_output(debug_output_fn output, LoggerOutputLevel level);

//int log_level_active(LoggerOutputLevel level, void* reserved);

extern debug_output_fn debug_output_;
extern LoggerOutputLevel log_level_at_run_time;

// Macros to use
#define DEBUG(fmt, ...) LOG(DEBUG, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) LOG(INFO, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG(WARN, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG(ERROR, fmt, ##__VA_ARGS__)
#define DEBUG_D(fmt, ...) LOG_FORMAT(LOG, fmt, ##__VA_ARGS__)

#define SPARK_ASSERT(predicate) do { if (!(predicate)) PANIC(AssertionFailure,"AssertionFailure "#predicate);} while(0);

#ifdef __cplusplus
}
#endif


#endif /* DEBUG_H_ */
