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


#include <stdint.h>
#include <wwd_rtos.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_MBOX_NULL ((void*)0)
#define SYS_SEM_NULL  ((void*)0)

typedef host_semaphore_type_t  /*@only@*/ sys_sem_t;
typedef host_queue_type_t      /*@only@*/ sys_mbox_t;
typedef host_thread_type_t     /*@only@*/ sys_thread_t;

uint16_t sys_rand16( void );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_SYS_ARCH_H */

