/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define RTOS_HIGHER_PRIORTIY_THAN(x)     ((x) > RTOS_HIGHEST_PRIORITY ? (x)-1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      ((x) < RTOS_LOWEST_PRIORITY  ? (x)+1 : RTOS_LOWEST_PRIORITY )
#define RTOS_LOWEST_PRIORITY             (1023)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (4)

#define RTOS_USE_STATIC_THREAD_STACK

/* The number of system ticks per second */
#define SYSTICK_FREQUENCY  (1000)

#ifndef WWD_LOGGING_STDOUT_ENABLE
#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (1248 + 1400) /*(632)*/   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (1024 + 1400) /*(544)*/
#endif /* ifdef DEBUG */
#else /* if WWD_LOGGING_STDOUT_ENABLE */
#define WWD_THREAD_STACK_SIZE        (544 + 4096 + 1400)   /* WWD_LOG uses printf and requires minimum 4K stack space */
#endif /* WWD_LOGGING_STDOUT_ENABLE */


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct
{
    ULONG time_slice;    /* 0 to disable timeslice among same prio threads; other value to specify slice length */
    UINT arg;
} host_rtos_thread_config_type_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef TX_SEMAPHORE  host_semaphore_type_t; /** ThreadX definition of a semaphore */
typedef TX_THREAD     host_thread_type_t;    /** ThreadX definition of a thread handle */
typedef TX_QUEUE      host_queue_type_t;     /** ThreadX definition of a message queue */

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
