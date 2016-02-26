/*
 * debug.c
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "spark_macros.h"
#include "debug.h"
#include "timer_hal.h"

LoggerOutputLevel log_level_at_run_time = LOG_LEVEL_AT_RUN_TIME;
debug_output_fn debug_output_;

void set_logger_output(debug_output_fn output, LoggerOutputLevel level)
{
    if (output)
        debug_output_ = output;
    if (level==DEFAULT_LEVEL)
        level = LOG_LEVEL_AT_RUN_TIME;
    log_level_at_run_time = level;
}

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...)
{
    if (level<log_level_at_run_time || !debug_output_)
        return;

    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
    static char * levels[] = {
            "",
            "LOG  ",
            "DEBUG",
            "INFO ",
            "WARN ",
            "ERROR",
            "PANIC",
    };
    va_list args;
    va_start(args, msg);
    file = file ? strrchr(file,'/') + 1 : "";
    int trunc = snprintf(_buffer, arraySize(_buffer), "%010u:%s: %s %s(%d):", (unsigned)HAL_Timer_Get_Milli_Seconds(), levels[level/10], func, file, line);
	debug_output_(_buffer);
	if (trunc > arraySize(_buffer))
	{
		debug_output_("...");
	}
    trunc = vsnprintf(_buffer,arraySize(_buffer), msg, args);
	debug_output_(_buffer);
	if (trunc > arraySize(_buffer))
	{
		debug_output_("...");
	}
	debug_output_("\r\n");
	va_end(args);
}

void log_print_direct_(int level, void* reserved, const char *msg, ...)
{
    if (level<log_level_at_run_time || !debug_output_)
        return;

    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
    va_list args;
    va_start(args, msg);
    int trunc = vsnprintf(_buffer, arraySize(_buffer), msg, args);
	debug_output_(_buffer);
	if (trunc > arraySize(_buffer))
	{
		debug_output_("...");
	}
	va_end(args);
}

void log_direct_(const char* s) {

    if (LOG_LEVEL<log_level_at_run_time || !debug_output_)
        return;

	debug_output_(s);
}

int log_level_active(LoggerOutputLevel level, void* reserved)
{
	return (level>=log_level_at_run_time && debug_output_);
}



