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
 *  Definitions for the FreeRTOS implementation of the Wiced RTOS
 *  abstraction layer.
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include "platform_isr.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wwd_FreeRTOS_systick.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern void vPortSVCHandler    ( void );
extern void xPortPendSVHandler ( void );
extern void xPortSysTickHandler( void );

/* Define interrupt handlers needed by FreeRTOS. These defines are used by the
 * vector table.
 */
#define SVC_irq     vPortSVCHandler
#define PENDSV_irq  xPortPendSVHandler
#define SYSTICK_irq xPortSysTickHandler


#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x < RTOS_HIGHEST_PRIORITY ? x+1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x > RTOS_LOWEST_PRIORITY ? x-1 : RTOS_LOWEST_PRIORITY)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (configMAX_PRIORITIES-1)
#define RTOS_DEFAULT_THREAD_PRIORITY     (1)

#define RTOS_USE_DYNAMIC_THREAD_STACK

#ifndef WWD_LOGGING_STDOUT_ENABLE
#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (732 + 1400)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (544 + 1400)
#endif /* ifdef DEBUG */
#else /* if WWD_LOGGING_STDOUT_ENABLE */
#define WWD_THREAD_STACK_SIZE        (544 + 4096 + 1400) /* WWD_LOG uses printf and requires a minimum of 4K stack */
#endif /* WWD_LOGGING_STDOUT_ENABLE */

/******************************************************
 *             Structures
 ******************************************************/

typedef SemaphoreHandle_t   /*@abstract@*/ /*@only@*/ host_semaphore_type_t;  /** FreeRTOS definition of a semaphore */
typedef TaskHandle_t        /*@abstract@*/ /*@only@*/ host_thread_type_t;     /** FreeRTOS definition of a thread handle */
typedef QueueHandle_t       /*@abstract@*/ /*@only@*/ host_queue_type_t;      /** FreeRTOS definition of a message queue */

/*@external@*/ extern void vApplicationMallocFailedHook( void );
/*@external@*/ extern void vApplicationIdleSleepHook( void );

typedef struct
{
    uint8_t info;    /* not supported yet */
} host_rtos_thread_config_type_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
