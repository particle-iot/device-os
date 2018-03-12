/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_SYS_ARCH_H
#define INCLUDED_SYS_ARCH_H

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
//#include "semphr.h"
#if defined ( IAR_TOOLCHAIN )
#include "platform_cmis.h"
#endif

#include "embos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_MBOX_NULL ((struct e_mailbox *)0)
#define SYS_SEM_NULL  ((OS_CSEMA*)0)
typedef uint32_t portTickType;

// unlike FreeRTOS EMBOS expects the mailbox buffer to be supplied externally
typedef struct e_mailbox {
    OS_MAILBOX mbox;
    void* buffer;
} e_mailbox_t;
// unlike FreeRTOS EMBOS expects the stack buffer to be supplied externally
typedef struct emos_task {
    OS_TASK task;
    OS_STACKPTR int* stack;
} emos_task_t;


#define portSTACK_TYPE  unsigned long
typedef OS_CSEMA*  /*@only@*/ sys_sem_t;
typedef e_mailbox_t*      /*@only@*/ sys_mbox_t;
typedef emos_task_t*       /*@only@*/ sys_thread_t;

uint16_t sys_rand16( void );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_SYS_ARCH_H */

