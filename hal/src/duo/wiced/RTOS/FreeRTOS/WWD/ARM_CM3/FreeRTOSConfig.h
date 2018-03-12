/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "platform_config.h"
#include "wwd_FreeRTOS_systick.h"
#include "wwd_assert.h"

#ifdef ENABLE_TASK_TRACE
#include "FreeRTOS_trace.h"
#endif /* ifdef ENABLE_TASK_TRACE */


#if defined ( __IAR_SYSTEMS_ICC__ )
/* This file is included from the IAR portasm.s, so must avoid C
declarations in that case */
#include "platform_sleep.h"
#endif /* if defined ( __IAR_SYSTEMS_ICC__ ) */

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configNO_MALLOC                             ( 0 )
#define configUSE_NEWLIB_MALLOC_LOCK                ( 0 )
#define configUSE_TIMERS                            ( 1 )
#define configTIMER_TASK_PRIORITY                   ( 2 )
#define configTIMER_QUEUE_LENGTH                    ( 5 )
#define configTIMER_TASK_STACK_DEPTH                ( ( unsigned short ) (1024 / sizeof( portSTACK_TYPE )) )
#define configUSE_PREEMPTION                        ( 1 )
#define configUSE_IDLE_HOOK                         ( 0 )
#define configUSE_TICK_HOOK                         ( 1 )
#define configCPU_CLOCK_HZ                          ( ( unsigned long ) CPU_CLOCK_HZ )
#define configTICK_RATE_HZ                          ( ( TickType_t ) SYSTICK_FREQUENCY )
#define configMAX_PRIORITIES                        ( 10 )
#define configMINIMAL_STACK_SIZE                    ( ( unsigned short ) (250 / sizeof( portSTACK_TYPE )) ) /* size of idle thread stack */
#define configMAX_TASK_NAME_LEN                     ( 16 )
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY                    ( 1 )
#endif /* configUSE_TRACE_FACILITY */
#define configUSE_16_BIT_TICKS                      ( 0 )
#define configIDLE_SHOULD_YIELD                     ( 1 )
#ifndef configUSE_MUTEXES
#define configUSE_MUTEXES                           ( 1 )
#endif /* ifndef configUSE_MUTEXES */
#define configUSE_COUNTING_SEMAPHORES               ( 1 )
#define configFREE_TASKS_IN_IDLE                    ( 0 )

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                       ( 0 )
#define configMAX_CO_ROUTINE_PRIORITIES             ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
 to exclude the API function. */

#define INCLUDE_vTaskPrioritySet                    ( 1 )
#define INCLUDE_uxTaskPriorityGet                   ( 1 )
#define INCLUDE_vTaskDelete                         ( 1 )
#define INCLUDE_vTaskCleanUpResources               ( 0 )
#define INCLUDE_vTaskSuspend                        ( 1 )
#define INCLUDE_vTaskDelayUntil                     ( 1 )
#define INCLUDE_vTaskDelay                          ( 1 )
#define INCLUDE_xTaskGetCurrentThread               ( 1 )
#define INCLUDE_vTaskForceAwake                     ( 1 )
#define INCLUDE_vTaskGetStackInfo                   ( 1 )
#define INCLUDE_xTaskIsTaskFinished                 ( 1 )
#define INCLUDE_xTaskGetCurrentTaskHandle           ( 1 )

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
 (lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY             ( 0xF0 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY        ( 0x20 )

/* This is the value being used as per the ST library which permits 16
 priority values, 0 to 15.  This must correspond to the
 configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
 NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY     ( 15 )

/* Check for stack overflows - requires defining vApplicationStackOverflowHook */
#define configCHECK_FOR_STACK_OVERFLOW              ( 2 )

/* Run a handler if a malloc fails - vApplicationMallocFailedHook */
#define configUSE_MALLOC_FAILED_HOOK                ( 1 )

#define configAPPLICATION_ALLOCATED_HEAP            ( 1 )
#define configDYNAMIC_HEAP_SIZE                     ( 1 )


#if defined( DEBUG ) && ( ! defined( UNIT_TESTER ) )
#define configASSERT( expr )   wiced_assert( "FreeRTOS assert", expr )
#endif /* ifdef DEBUG */


#ifdef WICED_DISABLE_MCU_POWERSAVE

#define configUSE_IDLE_SLEEP_HOOK ( 1 )

#else /* ifdef WICED_DISABLE_MCU_POWERSAVE */

#define configUSE_IDLE_NO_TICK_SLEEP_HOOK ( 1 )

#endif /* ifdef WICED_DISABLE_MCU_POWERSAVE */


#ifdef NETWORK_NOTIFY_RELEASED_PACKETS

#define MEMP_FREE_NOTIFY
extern void memp_free_notify( unsigned int type );

#endif /* ifdef NETWORK_NOTIFY_RELEASED_PACKETS */


#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* FREERTOS_CONFIG_H */

