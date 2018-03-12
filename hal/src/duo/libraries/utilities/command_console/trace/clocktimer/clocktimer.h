/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef CLOCKTIME_T
#define CLOCKTIME_T     unsigned long
#endif /* CLOCKTIME_T */

/**
 * This file defines functions to get an accurate timestamp from the procesor,
 * for situations in which the SysTick time resolution provided by the RTOS
 * isn't accurate enough.
 */

extern CLOCKTIME_T get_clocktime( void );
extern void start_clocktimer( void );
extern void reset_clocktimer( void );
extern void end_clocktimer( void );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* TIMER_H_ */
