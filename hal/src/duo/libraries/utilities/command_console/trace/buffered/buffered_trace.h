/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * This file describes the trace buffer API, which allows access to the trace
 * buffer data.
 *
 * This file defines functions to start, pause and end tracing, but does not
 * provide any data processing.
 *
 * The functions in this file should be RTOS independent, abstracting data
 * types using macro definitions.
 */

#ifndef BUFFERED_TRACE_H_
#define BUFFERED_TRACE_H_

#include "../trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *        Configuration
 ******************************************************/
/**
 * Using "clocktime" provides more accurate timing values by using the
 * processor clock for timing. However, this uses a lot more space in the
 * buffer.
 */
#define BUFFERED_TRACE_USE_CLOCKTIME    (1)

#include "print/buffered_trace_print.h"
#ifdef TRACE_ENABLE_BUFFERED
#define TRACE_T_BUFFER                                              \
    {                                                               \
            (char*) "buffered",                                     \
            buffered_trace_start_trace,                             \
            buffered_trace_stop_trace,                              \
            buffered_trace_preprocess_trace,                        \
            buffered_trace_cleanup_trace,                           \
            buffered_trace_task_hook,                               \
            buffered_trace_tick_hook,                               \
            (trace_process_t []) {                                  \
                    TRACE_PROCESS_T_BUFFERED_PRINT                  \
                    { NULL, NULL, NULL }                            \
            }                                                       \
    },
#else
#define TRACE_T_BUFFER
#endif /* TRACE_ENABLE_BUFFERED */

/******************************************************
 *        Macros
 ******************************************************/
#define TASKTRACE_PRINT( msg )  \
    do                          \
    {                           \
        printf msg ;            \
    }                           \
    while( 0 )


/******************************************************
 *      Compiler directives
 ******************************************************/
#define PACKED_STRUCT       __attribute__ ((__packed__));


/******************************************************
 *        Structures
 ******************************************************/

/** Forward declarations */
struct task_info_t;
struct trace_log_action_t;
struct trace_log_tick_t;
struct trace_log_clocktime_t;

/**
 * Stores information about all tasks appearing in the trace log. There will
 * be one instance of this structure for each task in existence. Since the
 * number of tasks is low, we need not worry too much about the size of this
 * structure.
 */
struct task_info_t
{
    void *p;
    char task_name[MAX_TASKNAME_LEN];
    TCB_NUMBER_T task_tcb_number;
};
typedef struct task_info_t task_info_t;

/**
 * Stores all scheduler actions that occurred. The scheduler action will be
 * associated with a parent trace log tick, which defines the time at which
 * the scheduler action occurred.
 *
 * There will potentially be many of these structures. For this reason, we will
 * discard scheduler actions that aren't particularly interesting (such as the
 * switching out of a task followed immediately by the switching in of the same
 * task).
 *
 * @remarks Since there will be so many of these created, we instruct the
 * compiler to not perform any alignment (optimizing for space).
 */
struct trace_log_action_t
{
    /**
     * @remarks This assumes that 0 <= task_tcb <= 15 (ie. can be represented
     * by 4-bits).
     * @remarks This assumes that trace_action_t can be stored in 4-bits.
     */
    unsigned int task_tcb : 4;
    unsigned int action   : 4; /* this is a trace_action_t */
} PACKED_STRUCT;
typedef struct trace_log_action_t trace_log_action_t;

/**
 * Stores information regarding a scheduler action at a specific time. There
 * will potentially be many of these structures (one for every clock tick).
 * However, if nothing particularly interesting happens during a clock cycle,
 * then we will avoiding creating a trace log tick for that clock tick.
 *
 * Each trace log tick has an associated list of trace log actions which
 * occurred at that tick time. This list should be non-empty, else the trace log
 * tick should not have been kept.
 *
 * @remarks Since there will be so many of these created, we instruct the
 * compiler to not perform any alignment (optimizing for space).
 */
struct trace_log_tick_t
{
    TICK_T ticks;               /**< The tick time. Note that this will only
                                     give us a time resolution of the SysTick
                                     timer (1ms on FreeRTOS). */
    unsigned int action_count;  /**< Number of actions that follow this
                                     structure in the buffer. */
} PACKED_STRUCT;
typedef struct trace_log_tick_t trace_log_tick_t;

#if BUFFERED_TRACE_USE_CLOCKTIME
/**
 * More accurate timing information from the processor. This is only stored for
 * Trace_SwitchOut actions, to record the total time that a task was executing.
 */
struct trace_log_clocktime_t
{
    CLOCKTIME_T clocktime;
} PACKED_STRUCT;
typedef struct trace_log_clocktime_t trace_log_clocktime_t;
#endif /* BUFFERED_TRACE_USE_CLOCKTIME */

/******************************************************
 *        Buffers
 ******************************************************/
/** Size of buffers */
#define TRACE_LOG_TASK_COUNT        (10)
#define TRACE_LOG_BUFFER_SIZE       (8 * 1024)


/******************************************************
 *        Buffer flags
 ******************************************************/
#define TRACE_BUFFER_FULL           0x00000001
#define TRACE_BUFFER_INCOMPLETE     0x00000002

#define TraceBuffer_IsFull(flags)           ((flags & TRACE_BUFFER_FULL) != 0)
#define TraceBuffer_IsIncomplete(flags)     ((flags & TRACE_BUFFER_INCOMPLETE) != 0)

#define TraceBuffer_SetFull(flags)          (flags |= TRACE_BUFFER_FULL)
#define TraceBuffer_SetIncomplete(flags)    (flags |= TRACE_BUFFER_INCOMPLETE)


/******************************************************
 *        Hook functions
 ******************************************************/
void buffered_trace_task_hook( TRACE_TASK_HOOK_SIGNATURE );
void buffered_trace_tick_hook( TRACE_TICK_HOOK_SIGNATURE );


/******************************************************
 *        API functions to start/pause/end tracing
 ******************************************************/
void buffered_trace_start_trace( trace_flush_function_t flush_f );
void buffered_trace_stop_trace( void );
void buffered_trace_preprocess_trace( void );
void buffered_trace_cleanup_trace( void );


/******************************************************
 *        API functions to access the buffers
 ******************************************************/
void *buffered_trace_get_tracebuffer_start( void );
void *buffered_trace_get_tracebuffer_end( void );
void *buffered_trace_get_taskinfo( void );
unsigned int buffered_trace_get_taskcount( void );
int buffered_trace_get_tracebuffer_flags( void );

trace_log_tick_t *buffered_trace_get_tick_from_buffer( void ** );
trace_log_action_t *buffered_trace_get_action_from_buffer( void ** );
#if BUFFERED_TRACE_USE_CLOCKTIME
trace_log_clocktime_t *buffered_trace_get_clocktime_from_buffer( void ** );
#endif /* BUFFERED_TRACE_USE_CLOCKTIME */


#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* BUFFERED_TRACE_H_ */
