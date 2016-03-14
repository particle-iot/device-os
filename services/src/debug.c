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
    if (output) {
        log_compat_callback = output;
    }
    if (level == DEFAULT_LEVEL) {
        level = LOG_LEVEL_AT_RUN_TIME;
    }
    log_compat_level = level;
}

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...) {
    // Not implemented, use newer log_message() function instead
}

void log_print_direct_(int level, void* reserved, const char *msg, ...) {
    // Not implemented, use newer log_format() function instead
}

void log_direct_(const char* s) {
    // Not implemented, use newer log_write() function instead
}

int log_level_active(LoggerOutputLevel level, void* reserved) {
    // Not implemented, use newer log_enabled() function instead
	return 0;
}
