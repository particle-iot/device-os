/*
 * debug.h
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */
#ifndef DEBUG_H_
#define DEBUG_H_

#include "config.h"

/*
 * This module supports runtime and compile time message filtering.
 *
 * For a message be compiled in the message type has to be >= to then the LOG_LEVEL_AT_COMPILE_TIME
 * For a message to be output the message type has to be >= then the LOG_LEVEL_AT_RUN_TIME
 *
 * This module assumes one of two #defines RELEASE_BUILD or DEBUG_BUILD exist
 * Then the LOG_LEVEL_AT_COMPILE_TIME and LOG_LEVEL_AT_RUN_TIME are set based on RELEASE or DEBUG
 * For a release build
 *     LOG_LEVEL_AT_COMPILE_TIME = WARN_LEVEL
 *         LOG_LEVEL_AT_RUN_TIME = WARN_LEVEL
 * For a debug build:
 *     LOG_LEVEL_AT_COMPILE_TIME = LOG_LEVEL
 *     LOG_LEVEL_AT_RUN_TIME = LOG_LEVEL
 * The API
 * LOG(fmt,...)
 * DEBUG(fmt,...)
 * WARN(fmt,...)
 * ERROR(fmt,...)
 * PANIC(code,fmt,...)
 */

#include <stdint.h>
#include <stddef.h>
#include "panic.h"

// Debug Levels
#define LOG_LEVEL       1
#define DEBUG_LEVEL     2
#define WARN_LEVEL      3
#define ERROR_LEVEL     4
#define PANIC_LEVEL     5

#if !defined(INCLUDE_FILE_INFO_IN_DEBUG)
#define _FILE_PATH          __FILE__
#else
#define _FILE_PATH          NULL
#endif  //

#if defined(DEBUG_BUILD)
#define LOG_LEVEL_AT_COMPILE_TIME LOG_LEVEL
#define LOG_LEVEL_AT_RUN_TIME LOG_LEVEL         // Set to allow all LOG_LEVEL and above messages to be displayed conditionally by levels.
#endif

#if defined(RELEASE_BUILD)
#define LOG_LEVEL_AT_COMPILE_TIME ERROR_LEVEL
#define LOG_LEVEL_AT_RUN_TIME ERROR_LEVEL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Must be provided by main if wanted as extern C definitions
extern uint32_t log_level_at_run_time __attribute__ ((weak));;
void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...);
void debug_output_(const char *) __attribute__ ((weak));
#ifdef __cplusplus
}
#endif

// Short Cuts
#define __LOG_LEVEL_TEST(level) (level >= LOG_LEVEL_AT_COMPILE_TIME && level >= LOG_LEVEL_AT_RUN_TIME)

#if defined(USE_ONLY_PANIC)
#define LOG(fmt, ...)
#define DEBUG(fmt, ...)
#define WARN(fmt, ...)
#define ERROR(fmt, ...)
#define PANIC(code,fmt, ...) do {panic_(code);}while(0)
#else
// Macros to use
#define LOG(fmt, ...)    do { if ( __LOG_LEVEL_TEST(LOG_LEVEL)  )  {log_print_(LOG_LEVEL,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt, ##__VA_ARGS__);}}while(0)
#define DEBUG(fmt, ...)  do { if ( __LOG_LEVEL_TEST(DEBUG_LEVEL))  {log_print_(DEBUG_LEVEL,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
#define WARN(fmt, ...)   do { if ( __LOG_LEVEL_TEST(WARN_LEVEL) )  {log_print_(WARN_LEVEL,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
#define ERROR(fmt, ...)  do { if ( __LOG_LEVEL_TEST(ERROR_LEVEL) ) {log_print_(ERROR_LEVEL,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
#define PANIC(code,fmt, ...)  do { if ( __LOG_LEVEL_TEST(PANIC_LEVEL) ) {log_print_(PANIC_LEVEL,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);} panic_(code);}while(0)
#endif
#define SPARK_ASSERT(predicate) do { if (!(predicate)) PANIC(AssertionFailure,"AssertionFailure "#predicate);} while(0);

#endif /* DEBUG_H_ */
