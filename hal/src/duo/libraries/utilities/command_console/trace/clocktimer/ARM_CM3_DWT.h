/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef ARM_CM3_DWT_H_
#define ARM_CM3_DWT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** We will use the DWT for "clocktimer" functions */
#define DWT_CTRL            0xE0001000
#define DWT_CYCCNT          0xE0001004

#define CLOCKS_PER_SECOND   CPU_CLOCK_HZ
#define CLOCKTIME_T         unsigned long

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ARM_CM3_DWT_H_ */
