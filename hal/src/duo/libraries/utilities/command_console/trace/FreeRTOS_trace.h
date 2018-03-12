/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef FREERTOS_TRACE_H_
#define FREERTOS_TRACE_H_

/**
 * This is the main include file to enable FreeRTOS trace functionality.
 */

#include "portmacro.h"
#include "trace_action.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @remarks CLOCKTIME_T must be defined for the processor. This is currently
 * done using makefiles.
 */

/******************************************************************************/
/** @name Hook function signatures and variables                              */
/******************************************************************************/
/** @{ */

/** The signature of the task hook function. */
#define TRACE_TASK_HOOK_SIGNATURE           \
    void *p,                                \
    signed char TaskName[],                 \
    unsigned long RunTimeCounter,           \
    unsigned portBASE_TYPE TCBNumber,       \
    unsigned portBASE_TYPE TaskNumber,      \
    unsigned portBASE_TYPE Priority,        \
    unsigned portBASE_TYPE NewPriority,     \
    trace_action_t action

/**
 * The variables of TRACE_TASK_HOOK_ARGS.  Used to pass the variables on to the
 * currently active handler hook. */
#define TRACE_TASK_HOOK_VARIABLES           \
    p,                                      \
    TaskName,                               \
    RunTimeCounter,                         \
    TCBNumber,                              \
    TaskNumber,                             \
    Priority,                               \
    NewPriority,                            \
    action

/** The signature of the tick hook function. */
#define TRACE_TICK_HOOK_SIGNATURE           \
    portTickType TickCount

/**
 * The variables of TRACE_TICK_HOOK_ARGS. Used to pass the variables on to the
 * currently active handler hook.
 */
#define TRACE_TICK_HOOK_VARIABLES           \
    TickCount

/** @} */
/******************************************************************************/

/******************************************************************************/
/** @name RTOS-specific types                                                 */
/******************************************************************************/
/** @{ */
#define TCB_NUMBER_T        unsigned portBASE_TYPE
#define MAX_TASKNAME_LEN    configMAX_TASK_NAME_LEN
#define TICK_T              portTickType
#define TICKS_PER_SEC       configTICK_RATE_HZ
/** @} */
/******************************************************************************/

/**
 * configUSE_TRACE_FACILITY must be set to enable some FreeRTOS trace-specific
 * functionality.
 */
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY (1)
#endif /* configUSE_TRACE_FACILITY */

#include "trace_hook.h"

/** @name Macros to be called by the scheduler to provide trace functionality */
/** @{ */
#define traceTASK_CREATE( pxNewTCB ) \
    traceTASK( pxNewTCB, 0, Trace_Create )
#define traceTASK_DELETE( pxTCB ) \
    traceTASK( pxTCB, 0, Trace_Delete )
#define traceTASK_SUSPEND( pxTCB ) \
    traceTASK( pxTCB, 0, Trace_Suspend )
#define traceTASK_RESUME( pxTCB ) \
    traceTASK( pxTCB, 0, Trace_Resume )
#define traceTASK_RESUME_FROM_ISR( pxTCB ) \
    traceTASK( pxTCB, 0, Trace_ResumeFromISR )
#define traceTASK_DELAY() \
    traceTASK( pxCurrentTCB, 0, Trace_Delay )
#define traceTASK_DELAY_UNTIL() \
    traceTASK( pxCurrentTCB, 0, Trace_Delay )
#define traceTASK_CREATE_FAILED() \
    traceTASK( pxNewTCB, 0,  Trace_Die )
#define traceTASK_PRIORITY_SET( pxTask, uxNewPriority ) \
    traceTASK( pxTask, uxNewPriority, Trace_PrioritySet )
#define traceTASK_SWITCHED_OUT() \
    traceTASK( pxCurrentTCB, 0, Trace_SwitchOut )
#define traceTASK_SWITCHED_IN() \
    traceTASK( pxCurrentTCB, 0, Trace_SwitchIn )
/** @} */

#define traceTASK( pxTCB, uxNewPriority, action )       \
    do                                                  \
    {                                                   \
        trace_task_hook( (void*) pxTCB,                 \
                         pxTCB->pcTaskName,             \
                         0,                             \
                         pxTCB->uxTCBNumber,            \
                         pxTCB->uxTaskNumber,           \
                         pxTCB->uxPriority,             \
                         uxNewPriority,                 \
                         action );                      \
    }                                                   \
    while( 0 )

#define traceTASK_INCREMENT_TICK( xTickCount )          \
    do                                                  \
    {                                                   \
        trace_tick_hook( xTickCount );                  \
    }                                                   \
    while( 0 )

#include "trace_hook.h"

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* FREERTOS_TRACE_H_ */
