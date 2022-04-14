/*
 * @brief Repetitive Interrupt Timer registers and control functions
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __RITIMER_001_H_
#define __RITIMER_001_H_

#include "sys_config.h"
#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup IP_RITIMER_001 IP: RITimer register block and driver
 * @ingroup IP_Drivers
 * Repetitive Interrupt Timer
 * @{
 */

/**
 * @brief Repetitive Interrupt Timer register block structure
 */
typedef struct {				/*!< RITIMER Structure      */
	__IO uint32_t  COMPVAL;		/*!< Compare register       */
	__IO uint32_t  MASK;		/*!< Mask register. This register holds the 32-bit mask value. A 1 written to any bit will force a compare on the corresponding bit of the counter and compare register. */
	__IO uint32_t  CTRL;		/*!< Control register.      */
	__IO uint32_t  COUNTER;		/*!< 32-bit counter         */
#if defined(CHIP_LPC1347)
	__IO uint32_t  COMPVAL_H;	/*!< Compare upper register */
	__IO uint32_t  MASK_H;		/*!< Mask upper register    */
	__I  uint32_t  RESERVED0[1]; 
	__IO uint32_t  COUNTER_H;	/*!< Counter upper register */
#endif
} IP_RITIMER_001_T;

/**
 * @brief RITIMER register support bitfields and mask
 */

/*
 * RIT control register
 */
/**	Set by H/W when the counter value equals the masked compare value */
#define RIT_CTRL_INT    ((uint32_t) (1))
/** Set timer enable clear to 0 when the counter value equals the masked compare value  */
#define RIT_CTRL_ENCLR  ((uint32_t) _BIT(1))
/** Set timer enable on debug */
#define RIT_CTRL_ENBR   ((uint32_t) _BIT(2))
/** Set timer enable */
#define RIT_CTRL_TEN    ((uint32_t) _BIT(3))

/**
 * @brief	Initialize the RIT
 * @param	pRITimer	: RIT peripheral selected
 * @return	None
 */
void IP_RIT_Init(IP_RITIMER_001_T *pRITimer);

/**
 * @brief	DeInitialize the RIT
 * @param	pRITimer	: RIT peripheral selected
 * @return	None
 */
void IP_RIT_DeInit(IP_RITIMER_001_T *pRITimer);

/**
 * @brief	Enable Timer
 * @param	pRITimer	: RIT peripheral selected
 * @return	None
 */
STATIC INLINE void IP_RIT_Enable(IP_RITIMER_001_T *pRITimer)
{
	pRITimer->CTRL |= RIT_CTRL_TEN;
}

/**
 * @brief	Disable Timer
 * @param	pRITimer	: RIT peripheral selected
 * @return	None
 */
STATIC INLINE void IP_RIT_Disable(IP_RITIMER_001_T *pRITimer)
{
	pRITimer->CTRL &= ~RIT_CTRL_TEN;
}


/**
 * @brief	Timer Enable on debug
 * @param	pRITimer 	: RIT peripheral selected
 * @return	None
 */
STATIC INLINE void IP_RIT_TimerDebugEnable(IP_RITIMER_001_T *pRITimer)
{
	pRITimer->CTRL |= RIT_CTRL_ENBR;
}

/**
 * @brief	Timer Disable on debug
 * @param	pRITimer 	: RIT peripheral selected
 * @return	None
 */
STATIC INLINE void IP_RIT_TimerDebugDisable(IP_RITIMER_001_T *pRITimer)
{
	pRITimer->CTRL &= ~RIT_CTRL_ENBR;
}


/**
 * @brief	Check whether interrupt flag is set or not
 * @param	pRITimer	: RIT peripheral selected
 * @return	Current interrupt status, could be SET or UNSET
 */
IntStatus IP_RIT_GetIntStatus(IP_RITIMER_001_T *pRITimer);

/**
 * @brief	Set a tick value for the interrupt to time out
 * @param	pRITimer	: RIT peripheral selected
 * @param	val			: value (in ticks) of the interrupt to be set
 * @return	None
 */
STATIC INLINE void IP_RIT_SetCOMPVAL(IP_RITIMER_001_T *pRITimer, uint32_t val)
{
	pRITimer->COMPVAL = val;
}

/**
 * @brief	Enables or clears the RIT or interrupt
 * @param	pRITimer	: RIT peripheral selected
 * @param	val			: RIT to be set, one or more RIT_CTRL_* values
 * @return	None
 */
STATIC INLINE void IP_RIT_EnableCTRL(IP_RITIMER_001_T *pRITimer, uint32_t val)
{
	pRITimer->CTRL |= val;
}

/**
 * @brief	Get the RIT Counter value
 * @param	pRITimer	: RIT peripheral selected
 * @return	the counter value
 */
STATIC INLINE uint32_t IP_RIT_GetCounter(IP_RITIMER_001_T *pRITimer)
{
	return pRITimer->COUNTER;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RITIMER_001_H_ */
