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
                "PANIC",
        };
        va_list args;
        va_start(args, msg);
        file = file ? strrchr(file,'/') + 1 : "";
        int trunc = snprintf(_buffer, arraySize(_buffer), "%010u:<%s> %s %s(%d):", (unsigned)HAL_Timer_Get_Milli_Seconds(), levels[level], func, file, line);
        if (debug_output_)
        {
            debug_output_(_buffer);
          if (trunc > arraySize(_buffer))
          {
              debug_output_("...");
          }
        }
        trunc = vsnprintf(_buffer,arraySize(_buffer), msg, args);
        if (debug_output_)
        {
          debug_output_(_buffer);
          if (trunc > arraySize(_buffer))
          {
              debug_output_("...");
          }
          debug_output_("\r\n");
        }
}


