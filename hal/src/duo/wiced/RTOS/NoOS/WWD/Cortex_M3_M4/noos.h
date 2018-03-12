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
 *  of having no RTOS with Cortex-M3 / M4
 *
 */

#ifndef INCLUDED_NOOS_H_
#define INCLUDED_NOOS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Define System Tick interrupt handler needed by NoOS abstraction layer. This
 * defines is used by the vector table.
 */
#define SYSTICK_irq NoOS_systick_irq

#define SVC_irq UnhandledInterrupt
#define PENDSV_irq UnhandledInterrupt

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_NOOS_H_ */
