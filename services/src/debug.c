/*
 * debug.c
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */

#include "debug.h"

#if defined(DEBUG_BUILD)
#define LOG_LEVEL_AT_RUN_TIME LOG_LEVEL
#endif

#if defined(RELEASE_BUILD)
#define LOG_LEVEL_AT_RUN_TIME ERROR_LEVEL
#endif

LoggerOutputLevel log_compat_level = LOG_LEVEL_AT_RUN_TIME;
debug_output_fn log_compat_callback = NULL;

void set_logger_output(debug_output_fn output, LoggerOutputLevel level) {
    if (level == DEFAULT_LEVEL) {
        level = LOG_LEVEL_AT_RUN_TIME;
    }
    log_compat_level = level;
    log_compat_callback = output;
}

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...) {
    // Deprecated, use newer log_message() function instead
    // This function may not be removed because it is exported in dynalib
}

void log_print_direct_(int level, void* reserved, const char *msg, ...) {
    // Deprecated, use newer log_printf() function instead
    // This function may not be removed because it is exported in dynalib
}
