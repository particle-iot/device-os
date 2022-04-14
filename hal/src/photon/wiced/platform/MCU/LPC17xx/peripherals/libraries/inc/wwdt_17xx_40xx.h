/*
 * @brief LPC17xx/40xx WWDT chip driver
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

#ifndef __WWDT_17XX_40XX_H_
#define __WWDT_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup WWDT_17XX_40XX CHIP: LPC17xx/40xx WWDT driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/** WDT oscillator frequency value */
#define WDT_OSC     (500000 / 4)		/*!< Dedicated oscillator that provides a 500 kHz clock to Watchdog timer*/

/*!< Minimum tick count for timer value and window value */
#define WWDT_TICKS_MIN 0xFF

/*!< Maximum tick count for timer value and window value */
#define WWDT_TICKS_MAX 0xFFFFFF
/**
 * @brief	Initialize the Watchdog timer
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	None
 */
void Chip_WWDT_Init(LPC_WWDT_T *pWWDT);

/**
 * @brief	Shutdown the Watchdog timer
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	None
 */
STATIC INLINE void Chip_WWDT_DeInit(LPC_WWDT_T *pWWDT)
{
	IP_WWDT_DeInit(pWWDT);
}

/**
 * @brief	Set WDT timeout constant value used for feed
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	timeout	: WDT timeout in ticks, between WWDT_TICKS_MIN and WWDT_TICKS_MAX
 * @return	none
 */
STATIC INLINE void Chip_WWDT_SetTimeOut(LPC_WWDT_T *pWWDT, uint32_t timeout)
{
	IP_WWDT_SetTimeOut(pWWDT, timeout);
}

/**
 * @brief	Feed watchdog timer
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	None
 * @note	If this function isn't called, a watchdog timer warning will occur.
 * After the warning, a timeout will occur if a feed has happened.
 */
STATIC INLINE void Chip_WWDT_Feed(LPC_WWDT_T *pWWDT)
{
	IP_WWDT_Feed(pWWDT);
}

#if defined(WATCHDOG_WINDOW_SUPPORT)
/**
 * @brief	Set WWDT warning interrupt
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	timeout	: WDT warning in ticks, between 0 and 1023
 * @return	None
 * @note	This is the number of ticks after the watchdog interrupt that the
 * warning interrupt will be generated.
 */
STATIC INLINE void Chip_WWDT_SetWarning(LPC_WWDT_T *pWWDT, uint32_t timeout)
{
	IP_WWDT_SetWarning(pWWDT, timeout);
}

/**
 * @brief	Set WWDT window time
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	timeout	: WDT window in ticks, between WWDT_TICKS_MIN and WWDT_TICKS_MAX
 * @return	none
 * @note	The watchdog timer must be fed between the timeout from the Chip_WWDT_SetTimeOut()
 * function and this function, with this function defining the last tick before the
 * watchdog window interrupt occurs.
 */
STATIC INLINE void Chip_WWDT_SetWindow(LPC_WWDT_T *pWWDT, uint32_t timeout)
{
	IP_WWDT_SetWindow(pWWDT, timeout);
}
#endif /*defined(WATCHDOG_WINDOW_SUPPORT)*/

/**
 * @brief	Enable watchdog timer options
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	options	: An or'ed set of options of values
 *						WWDT_WDMOD_WDEN, WWDT_WDMOD_WDRESET, and WWDT_WDMOD_WDPROTECT
 * @return	None
 * @note	You can enable more than one option at once (ie, WWDT_WDMOD_WDRESET |
 * WWDT_WDMOD_WDPROTECT), but use the WWDT_WDMOD_WDEN after all other options
 * are set (or unset) with no other options.
 */
STATIC INLINE void Chip_WWDT_SetOption(LPC_WWDT_T *pWWDT, uint32_t options)
{
	IP_WWDT_SetOption(pWWDT, options);
}

/**
 * @brief	Disable/clear watchdog timer options
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	options	: An or'ed set of options of values
 *						WWDT_WDMOD_WDEN, WWDT_WDMOD_WDRESET, and WWDT_WDMOD_WDPROTECT
 * @return	None
 * @note	You can disable more than one option at once (ie, WWDT_WDMOD_WDRESET |
 * WWDT_WDMOD_WDTOF).
 */
STATIC INLINE void Chip_WWDT_UnsetOption(LPC_WWDT_T *pWWDT, uint32_t options)
{
	IP_WWDT_UnsetOption(pWWDT, options);
}

/**
 * @brief	Enable WWDT activity
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	None
 */
STATIC INLINE void Chip_WWDT_Start(LPC_WWDT_T *pWWDT)
{
	IP_WWDT_Start(pWWDT);
}

/**
 * @brief	Read WWDT status flag
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	Watchdog status, an Or'ed value of WWDT_WDMOD_*
 */
STATIC INLINE uint32_t Chip_WWDT_GetStatus(LPC_WWDT_T *pWWDT)
{
	return IP_WWDT_GetStatus(pWWDT);
}

/**
 * @brief	Clear WWDT interrupt status flags
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @param	status	: Or'ed value of status flag(s) that you want to clear, should be:
 *              - WWDT_WDMOD_WDTOF: Clear watchdog timeout flag
 *              - WWDT_WDMOD_WDINT: Clear watchdog warning flag
 * @return	None
 */
STATIC INLINE void Chip_WWDT_ClearStatusFlag(LPC_WWDT_T *pWWDT, uint32_t status)
{
	IP_WWDT_ClearStatusFlag(pWWDT, status);
}

/**
 * @brief	Get the current value of WDT
 * @param	pWWDT	: The base of WatchDog Timer peripheral on the chip
 * @return	current value of WDT
 */
STATIC INLINE uint32_t Chip_WWDT_GetCurrentCount(LPC_WWDT_T *pWWDT)
{
	return IP_WWDT_GetCurrentCount(pWWDT);
}

#if defined(WATCHDOG_CLKSEL_SUPPORT)
/**
 * @brief Watchdog Clock Source definitions
 */
typedef enum CHIP_WWDT_CLK_SRC {
	WWDT_CLKSRC_IRC = WWDT_CLKSEL_SOURCE(0),				/*!< Internal RC oscillator */
	WWDT_CLKSRC_WATCHDOG_PCLK = WWDT_CLKSEL_SOURCE(1),		/*!< APB peripheral clock (watchdog pclk) */
	WWDT_CLKSRC_RTC_CLK = WWDT_CLKSEL_SOURCE(2),			/*!< RTC oscillator (rtc_clk) */
} CHIP_WWDT_CLK_SRC_T;

/**
 * @brief	Get the current value of WDT
 * @param	pWWDT		: The base of WatchDog Timer peripheral on the chip
 * @param	wdtClkSrc	: Selected watchdog clock source
 * @return	Nothing
 */
STATIC INLINE void Chip_WWDT_SelClockSource(LPC_WWDT_T *pWWDT, CHIP_WWDT_CLK_SRC_T wdtClkSrc)
{
	IP_WWDT_SelClockSource(pWWDT, (uint32_t) wdtClkSrc);
}

#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __WWDT_17XX_40XX_H_ */
