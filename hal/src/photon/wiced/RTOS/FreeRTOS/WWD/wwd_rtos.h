/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#ifndef WWD_LOGGING_UART_ENABLE
#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (732)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (544)
#endif /* ifdef DEBUG */
#else /* if WWD_LOGGING_UART_ENABLE */
#define WWD_THREAD_STACK_SIZE        (544 + 4096) /* WWD_LOG uses printf and requires a minimum of 4K stack */
#endif /* WWD_LOGGING_UART_ENABLE */

/******************************************************
 *             Structures
 ******************************************************/

typedef xSemaphoreHandle    /*@abstract@*/ /*@only@*/ host_semaphore_type_t;  /** FreeRTOS definition of a semaphore */
typedef xTaskHandle         /*@abstract@*/ /*@only@*/ host_thread_type_t;     /** FreeRTOS definition of a thread handle */
typedef xQueueHandle        /*@abstract@*/ /*@only@*/ host_queue_type_t;      /** FreeRTOS definition of a message queue */

/*@external@*/ extern void vApplicationMallocFailedHook( void );
/*@external@*/ extern void vApplicationIdleSleepHook( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
