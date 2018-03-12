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
 * This is the main include file for trace functionality. This file contains
 * functions to start and stop tracing, but no way of access trace data.
 */

#ifndef TRACE_HOOK_H_
#define TRACE_HOOK_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*trace_task_hook_f)( TRACE_TASK_HOOK_SIGNATURE );
typedef void (*trace_tick_hook_f)( TRACE_TICK_HOOK_SIGNATURE );

/**
 * @name These functions can be used to set and unset the hook functions for
 * scheduler tracing.
 */
/** @{ */
void set_trace_hooks( trace_task_hook_f task, trace_tick_hook_f tick );
void unset_trace_hooks( void );
/** @} */


/** Hook called every time the scheduler does something interesting. */
void trace_task_hook( TRACE_TASK_HOOK_SIGNATURE );

/** Hook called every time the tick count is incremented. */
void trace_tick_hook( TRACE_TICK_HOOK_SIGNATURE );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* TRACE_HOOK_H_ */
