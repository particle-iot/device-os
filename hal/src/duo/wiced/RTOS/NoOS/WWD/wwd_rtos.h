/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Definitions of the Wiced RTOS abstraction layer for the special case
 *  of having no RTOS
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include "platform_isr.h"
#include "noos.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (0)

#define RTOS_USE_DYNAMIC_THREAD_STACK
#define WWD_THREAD_STACK_SIZE            (544 + 1400)

/*
 * The number of system ticks per second
 */
#define SYSTICK_FREQUENCY  (1000)

/******************************************************
 *             Structures
 ******************************************************/

typedef volatile unsigned char   host_semaphore_type_t;  /** NoOS definition of a semaphore */
typedef volatile unsigned char   host_thread_type_t;     /** NoOS definition of a thread handle - Would be declared void but that is not allowed. */
typedef volatile unsigned char   host_queue_type_t;      /** NoOS definition of a message queue */

typedef struct
{
    uint8_t info;    /* not supported yet */
} host_rtos_thread_config_type_t;

/*@external@*/ void NoOS_setup_timing( void );
/*@external@*/ void NoOS_stop_timing( void );
/*@external@*/ void NoOS_systick_irq( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
