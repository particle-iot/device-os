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
#include "debug.h"

extern unsigned long millis(void);

uint32_t log_level_at_run_time = LOG_LEVEL_AT_RUN_TIME;

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...)
{
       char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
        static char * levels[] = {
                "",
                "LOG  ",
                "DEBUG",
                "WARN ",
                "ERROR",
        };
        va_list args;
        va_start(args, msg);
        file = file ? strrchr(file,'/') + 1 : "";
        snprintf(_buffer, sizeof(_buffer), "%010lu:<%s> %s %s(%d):", millis(), levels[level], func, file, line);
        if (debug_output_) debug_output_(_buffer);
        vsprintf(_buffer, msg, args);
        strcat(_buffer,"\r\n");
        if (debug_output_) debug_output_(_buffer);
}


