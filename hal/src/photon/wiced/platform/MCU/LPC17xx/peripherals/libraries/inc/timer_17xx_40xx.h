/*
 * @brief LPC17xx/40xx Timer/PWM control functions
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

#ifndef __TIMER_17XX_40XX_H_
#define __TIMER_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup TIMER_17XX_40XX CHIP: LPC17xx/40xx Timer driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief	Determine if a match interrupt is pending
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match interrupt number to check
 * @return	false if the interrupt is not pending, otherwise true
 * @note	Determine if the match interrupt for the specified timer and match
 * counter is pending.
 */
STATIC INLINE bool Chip_TIMER_MatchPending(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	return IP_TIMER_MatchPending(pTMR, matchnum);
}

/**
 * @brief	Determine if a capture interrupt is pending
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture interrupt number to check
 * @return	false if the interrupt is not pending, otherwise true
 * @note	Determine if the capture interrupt for the passed capture pin is
 * pending.
 */
STATIC INLINE bool Chip_TIMER_CapturePending(LPC_TIMER_T *pTMR, int8_t capnum)
{
	return IP_TIMER_CapturePending(pTMR, capnum);
}

/**
 * @brief	Clears a (pending) match interrupt
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match interrupt number to clear
 * @return	Nothing
 * @note	Clears a pending timer match interrupt.
 */
STATIC INLINE void Chip_TIMER_ClearMatch(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_ClearMatch(pTMR, matchnum);
}

/**
 * @brief	Clears a (pending) capture interrupt
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture interrupt number to clear
 * @return	Nothing
 * @note	Clears a pending timer capture interrupt.
 */
STATIC INLINE void Chip_TIMER_ClearCapture(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_ClearCapture(pTMR, capnum);
}

/**
 * @brief	Enables the timer (starts count)
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Nothing
 * @note	Enables the timer to start counting.
 */
STATIC INLINE void Chip_TIMER_Enable(LPC_TIMER_T *pTMR)
{
	IP_TIMER_Enable(pTMR);
}

/**
 * @brief	Disables the timer (stops count)
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Nothing
 * @note	Disables the timer to stop counting.
 */
STATIC INLINE void Chip_TIMER_Disable(LPC_TIMER_T *pTMR)
{
	IP_TIMER_Disable(pTMR);
}

/**
 * @brief	Returns the current timer count
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Current timer terminal count value
 * @note	Returns the current timer terminal count.
 */
STATIC INLINE uint32_t Chip_TIMER_ReadCount(LPC_TIMER_T *pTMR)
{
	return IP_TIMER_ReadCount(pTMR);
}

/**
 * @brief	Returns the current prescale count
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Current timer prescale count value
 * @note	Returns the current prescale count.
 */
STATIC INLINE uint32_t Chip_TIMER_ReadPrescale(LPC_TIMER_T *pTMR)
{
	return IP_TIMER_ReadPrescale(pTMR);
}

/**
 * @brief	Sets the prescaler value
 * @param	pTMR		: Pointer to timer IP register address
 * @param	prescale	: Prescale value to set the prescale register to
 * @return	Nothing
 * @note	Sets the prescale count value.
 */
STATIC INLINE void Chip_TIMER_PrescaleSet(LPC_TIMER_T *pTMR, uint32_t prescale)
{
	IP_TIMER_PrescaleSet(pTMR, prescale);
}

/**
 * @brief	Sets a timer match value
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer to set match count for
 * @param	matchval	: Match value for the selected match count
 * @return	Nothing
 * @note	Sets one of the timer match values.
 */
STATIC INLINE void Chip_TIMER_SetMatch(LPC_TIMER_T *pTMR, int8_t matchnum, uint32_t matchval)
{
	IP_TIMER_SetMatch(pTMR, matchnum, matchval);
}

/**
 * @brief	Reads a capture register
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture register to read
 * @return	The selected capture register value
 * @note	Returns the selected capture register value.
 */
STATIC INLINE uint32_t Chip_TIMER_ReadCapture(LPC_TIMER_T *pTMR, int8_t capnum)
{
	return IP_TIMER_ReadCapture(pTMR, capnum);
}

/**
 * @brief	Resets the timer terminal and prescale counts to 0
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_Reset(LPC_TIMER_T *pTMR)
{
	IP_TIMER_Reset(pTMR);
}

/**
 * @brief	Enables a match interrupt that fires when the terminal count
 *			matches the match counter value
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_MatchEnableInt(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_MatchEnableInt(pTMR, matchnum);
}

/**
 * @brief	Disables a match interrupt for a match counter
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_MatchDisableInt(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_MatchDisableInt(pTMR, matchnum);
}

/**
 * @brief	For the specific match counter, enables reset of the terminal count register when a match occurs
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_ResetOnMatchEnable(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_ResetOnMatchEnable(pTMR, matchnum);
}

/**
 * @brief	For the specific match counter, disables reset of the terminal count register when a match occurs
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_ResetOnMatchDisable(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_ResetOnMatchDisable(pTMR, matchnum);
}

/**
 * @brief	Enable a match timer to stop the terminal count when a match count equals the terminal count
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_StopOnMatchEnable(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_StopOnMatchEnable(pTMR, matchnum);
}

/**
 * @brief	Disable stop on match for a match timer. Disables a match timer
 *			to stop the terminal count when a match count equals the terminal count
 * @param	pTMR		: Pointer to timer IP register address
 * @param	matchnum	: Match timer, 0 to 3
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_StopOnMatchDisable(LPC_TIMER_T *pTMR, int8_t matchnum)
{
	IP_TIMER_StopOnMatchDisable(pTMR, matchnum);
}

/**
 * @brief	Enables capture on on rising edge of selected CAP signal for the
 *			selected capture register, enables the selected CAPn.capnum signal to load
 *			the capture register with the terminal coount on a rising edge
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureRisingEdgeEnable(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureRisingEdgeEnable(pTMR, capnum);
}

/**
 * @brief	Disables capture on on rising edge of selected CAP signal. For the
 *			selected capture register, disables the selected CAPn.capnum signal to load
 *			the capture register with the terminal coount on a rising edge
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureRisingEdgeDisable(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureRisingEdgeDisable(pTMR, capnum);
}

/**
 * @brief	Enables capture on on falling edge of selected CAP signal. For the
 *			selected capture register, enables the selected CAPn.capnum signal to load
 *			the capture register with the terminal coount on a falling edge
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureFallingEdgeEnable(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureFallingEdgeEnable(pTMR, capnum);
}

/**
 * @brief	Disables capture on on falling edge of selected CAP signal. For the
 *			selected capture register, disables the selected CAPn.capnum signal to load
 *			the capture register with the terminal coount on a falling edge
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureFallingEdgeDisable(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureFallingEdgeDisable(pTMR, capnum);
}

/**
 * @brief	Enables interrupt on capture of selected CAP signal. For the
 *			selected capture register, an interrupt will be generated when the enabled
 *			rising or falling edge on CAPn.capnum is detected
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureEnableInt(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureEnableInt(pTMR, capnum);
}

/**
 * @brief	Disables interrupt on capture of selected CAP signal
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capnum	: Capture signal/register to use
 * @return	Nothing
 */
STATIC INLINE void Chip_TIMER_CaptureDisableInt(LPC_TIMER_T *pTMR, int8_t capnum)
{
	IP_TIMER_CaptureDisableInt(pTMR, capnum);
}

/**
 * @brief	Sets external match control (MATn.matchnum) pin control
 * @param	pTMR			: Pointer to timer IP register address
 * @param	initial_state	: Initial state of the pin, high(1) or low(0)
 * @param	matchState		: Selects the match state for the pin
 * @param	matchnum		: MATn.matchnum signal to use
 * @return	Nothing
 * @note	For the pin selected with matchnum, sets the function of the pin that occurs on
 * a terminal count match for the match count.
 */
STATIC INLINE void Chip_TIMER_ExtMatchControlSet(LPC_TIMER_T *pTMR, int8_t initial_state,
												 IP_TIMER_PIN_MATCH_STATE_T matchState, int8_t matchnum)
{
	IP_TIMER_ExtMatchControlSet(pTMR, initial_state, matchState, matchnum);
}

/**
 * @brief	Sets timer count source and edge with the selected passed from CapSrc
 * @param	pTMR	: Pointer to timer IP register address
 * @param	capSrc	: timer clock source and edge
 * @param	capnum	: CAPn.capnum pin to use (if used)
 * @return	Nothing
 * @note	If CapSrc selected a CAPn pin, select the specific CAPn pin with the capnum value.
 */
STATIC INLINE void Chip_TIMER_TIMER_SetCountClockSrc(LPC_TIMER_T *pTMR,
													 IP_TIMER_CAP_SRC_STATE_T capSrc,
													 int8_t capnum)
{
	IP_TIMER_SetCountClockSrc(pTMR, capSrc, capnum);
}

/**
 * @brief	Initialize a timer
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Nothing
 */
void Chip_TIMER_Init(LPC_TIMER_T *pTMR);

/**
 * @brief	Shutdown a timer
 * @param	pTMR	: Pointer to timer IP register address
 * @return	Nothing
 */
void Chip_TIMER_DeInit(LPC_TIMER_T *pTMR);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_17XX_40XX_H_ */
