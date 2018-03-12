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

#include "wiced_result.h"
#include "platform/wwd_platform_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_END_OF_CURRENT_THREAD( )

#define WICED_NETWORKING_WORKER_THREAD              ( &wiced_networking_worker_thread )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_result_t (*event_handler_t)(void *arg);
typedef void (*timer_handler_t)(void * arg);

typedef void * wiced_thread_t;
typedef void * wiced_semaphore_t;
typedef void * wiced_mutex_t;
typedef void * wiced_queue_t;
typedef void * wiced_timer_t;
typedef void * wiced_timed_event_t;
typedef uint32_t wiced_event_flags_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    void* thread;
} wiced_worker_thread_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern wiced_worker_thread_t wiced_networking_worker_thread;

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
