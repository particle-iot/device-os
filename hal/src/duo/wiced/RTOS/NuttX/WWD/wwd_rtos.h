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

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include <nuttx/config.h>
#include <nuttx/wdog.h>
#include <nuttx/version.h>
#include <nuttx/arch.h>

#include <arch/chip/chip.h>
#include <arch/chip/irq.h>

#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define RTOS_HIGHER_PRIORTIY_THAN(x)     ((x) < RTOS_HIGHEST_PRIORITY ? (x)+1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      ((x) > RTOS_LOWEST_PRIORITY  ? (x)-1 : RTOS_LOWEST_PRIORITY )
#define RTOS_LOWEST_PRIORITY             SCHED_PRIORITY_MIN
#define RTOS_HIGHEST_PRIORITY            SCHED_PRIORITY_MAX
#define RTOS_DEFAULT_THREAD_PRIORITY     SCHED_PRIORITY_DEFAULT

/* RTOS allocates stack, no pre-allocated stacks */
#define RTOS_USE_DYNAMIC_THREAD_STACK

/* The number of system ticks per second */
#define SYSTICK_FREQUENCY  (1000 * 1000 / CONFIG_USEC_PER_TICK)

#ifndef WWD_LOGGING_STDOUT_ENABLE
#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (1248 + 1400)
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (1024 + 1400)
#endif /* ifdef DEBUG */
#else /* if WWD_LOGGING_STDOUT_ENABLE */
#define WWD_THREAD_STACK_SIZE        (1024 + 4096 + 1400) /* WWD_LOG uses printf and requires minimum 4K stack space */
#endif /* WWD_LOGGING_STDOUT_ENABLE */

/******************************************************
 *             Structures
 ******************************************************/

typedef sem_t    host_semaphore_type_t;

typedef struct
{
    uint32_t              message_num;
    uint32_t              message_size;
    uint8_t*              buffer;
    uint32_t              push_pos;
    uint32_t              pop_pos;
    host_semaphore_type_t push_sem;
    host_semaphore_type_t pop_sem;
    uint32_t              occupancy;
} host_queue_type_t;

typedef struct
{
    pthread_t handle;
    uint32_t  arg;
    void(*entry_function)( uint32_t );
} host_thread_type_t;

typedef struct
{
    uint8_t info;    /* not supported yet */
} host_rtos_thread_config_type_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
