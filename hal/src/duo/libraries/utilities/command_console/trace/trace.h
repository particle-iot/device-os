/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef TRACE_H_
#define TRACE_H_

#include "trace_hook.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * ORDER OF EXECUTION
 * 1. trace_start_function_t
 * 2. set_trace_hooks
 * 3. COMMAND
 * 4. unset_trace_hooks
 * 5. trace_stop_function_t
 * 6. trace_process_function_t
 * 7. trace_cleanup_function_t
 */

/** A function to process trace data. */
typedef void (*trace_process_function_t) ( void );

/** A function to prematurely process trace data. */
typedef void (*trace_flush_function_t) ( void );

/** Details about a method for processing trace data. */
typedef struct trace_process_t
{
    char *name;
    trace_process_function_t process_command;
    trace_flush_function_t flush_command;
} trace_process_t;

/** A function to start tracing. */
typedef void (*trace_start_function_t) ( trace_flush_function_t );

/** A method to pause/stop tracing. */
typedef void (*trace_stop_function_t) ( void );

/** A method to be executed immediately before processing trace data. */
typedef void (*trace_preprocess_function_t) ( void );

/** A method to clean up after processing a trace. */
typedef void (*trace_cleanup_function_t) ( void );

/** Details about a method for tracing thread execution. */
typedef struct trace_t
{
    char *name;
    trace_start_function_t start_command;
    trace_stop_function_t stop_command;
    trace_preprocess_function_t preprocess_command;
    trace_cleanup_function_t cleanup_command;

    trace_task_hook_f task_hook;
    trace_tick_hook_f tick_hook;

    trace_process_t *process_types;
} trace_t;

/**
 * Trace the thread execution of a command.
 *
 * Prefix a command with `trace' to execute the command and then print out
 * thread tracing information in a graphical format.
 *
 * This function is a stub which is network and RTOS independent.
 */
int console_trace( int argc, char *argv[] );

/**
 * Starts or resumes the tracing of the RTOS scheduler.
 */
int console_start_trace( int argc, char *argv[] );

/**
 * Temporarily pauses scheduler tracing without removing any of the existing
 * trace data.
 */
int console_end_trace( int argc, char *argv[] );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef TRACE_H_ */
