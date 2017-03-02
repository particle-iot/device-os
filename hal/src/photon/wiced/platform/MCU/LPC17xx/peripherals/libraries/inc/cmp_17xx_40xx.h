/*
 * @brief LPC17xx/40xx Analog Comparator driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licenser disclaim any and
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

#ifndef __CMP_17XX_40XX_H_
#define __CMP_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CHIP_LPC407X_8X)

/** @defgroup CMP_17XX_40XX CHIP: LPC17xx/40xx CMP Driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief	Initializes the CMP
 * @return	Nothing
 */
void Chip_CMP_Init(void);

/**
 * @brief	Deinitializes the CMP
 * @return	Nothing
 */
void Chip_CMP_DeInit(void);

/**
 * @brief	Enables comparator current source
 * @param	en		: Enable mode
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_EnableCurrentSrc(IP_CMP_ENCTRL_T en)
{
	IP_CMP_EnableCurrentSrc(LPC_CMP, en);
}

/**
 * @brief	Enables comparator bandgap reference
 * @param	en		: Enable mode
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_EnableBandGap(IP_CMP_ENCTRL_T en)
{
	IP_CMP_EnableBandGap(LPC_CMP, en);
}

/**
 * @brief	Control CMP_ROSC
 * @param	flag		: Or-ed bit value of CMP_CTRL_ROSCCTL_* and CMP_CTRL_EXT_RESET_*
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_ControlROSC(uint32_t flag)
{
	IP_CMP_ControlROSC(LPC_CMP, flag);
}

/**
 * @brief	Control CMP_ROSC
 * @param	flag		: Or-ed bit value of CMP_CTRL_T*CAP*
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_SetTimerCapInput(uint32_t flag)
{
	IP_CMP_SetTimerCapInput(LPC_CMP, flag);
}

/**
 * @brief	Sets up voltage ladder
 * @param	id				: Comparator ID
 * @param	ladSel			: Voltage ladder value (0~31).
 * @param	flag				:0(CMP_VREF used)/CMP_CTRLx_VLADREF_VDDA (VDDA used)
 * @return	Nothing
 * @note		VREF divider 0 = ladSel*VRef0/31
 */
STATIC INLINE void Chip_CMP_SetupVoltLadder(uint8_t id,
											uint16_t ladSel, uint32_t flag)
{
	IP_CMP_SetupVoltLadder(LPC_CMP, id, ladSel, flag);
}

/**
 * @brief	Enables voltage ladder
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_EnableVoltLadder(uint8_t id, IP_CMP_ENCTRL_T en)
{
	IP_CMP_EnableVoltLadder(LPC_CMP, id, en);
}

/**
 * @brief	Selects positive voltage input
 * @param	id		: Comparator ID
 * @param       input	: Selected input
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_SetPosVoltRef(uint8_t id, IP_CMP_INPUT_T input)
{
	IP_CMP_SetPosVoltRef(LPC_CMP, id, input);
}

/**
 * @brief	Selects negative voltage input
 * @param	id		: Comparator ID
 * @param	input	: Selected input
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_SetNegVoltRef(uint8_t id, IP_CMP_INPUT_T input)
{
	IP_CMP_SetNegVoltRef(LPC_CMP, id, input);
}

/**
 * @brief	Selects hysteresis level
 * @param	id		: Comparator ID
 * @param	hys		: Selected Hysteresis level
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_SetHysteresis(uint8_t id, IP_CMP_HYS_T hys)
{
	IP_CMP_SetHysteresis(LPC_CMP, id, hys);
}

/**
 * @brief	Enables specified comparator
 * @param	id		: Comparator ID
 * @param	en		: Enable mode
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_Enable(uint8_t id, IP_CMP_ENCTRL_T en)
{
	IP_CMP_Enable(LPC_CMP, id, en);
}

/**
 * @brief	Returns the current comparator status
 * @param	id	: Comparator Id (0/1)
 * @return	SET/RESET
 */
STATIC INLINE FlagStatus Chip_CMP_GetCmpStatus(uint8_t id)
{
	return IP_CMP_GetCmpStatus(LPC_CMP, id);
}

/**
 * @brief	Enable comparator output
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_EnableOuput(uint8_t id)
{
	IP_CMP_EnableOuput(LPC_CMP, id);
}

/**
 * @brief		Disable comparator output
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_DisableOutput(uint8_t id)
{
	IP_CMP_DisableOutput(LPC_CMP, id);
}

/**
 * @brief	Synchronizes Comparator output to bus clock
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_EnableSyncCmpOut(uint8_t id)
{
	IP_CMP_EnableSyncCmpOut(LPC_CMP, id);
}

/**
 * @brief	Sets comparator output to be used directly (no sync)
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_DisableSyncCmpOut(uint8_t id)
{
	IP_CMP_DisableSyncCmpOut(LPC_CMP, id);
}

/**
 * @brief	Sets up comparator interrupt
 * @param	id		: Comparator ID
 * @param	intFlag	: Or-ed value of CMP_CTRLx_INTTYPE_*, CMP_CTRLx_INTPOL_*
 * @param	edgeSel	: the edge on which interrupt occurs.
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_ConfigInt(uint8_t id,
									  uint32_t intFlag,
									  IP_CMP_INTEDGE_T edgeSel)
{
	IP_CMP_ConfigInt(LPC_CMP, id, intFlag, edgeSel);
}

/**
 * @brief	Get the CMP interrupt status
 * @param	id		: Comparator ID
 * @return	SET/RESET
 */
STATIC INLINE FlagStatus Chip_CMP_GetIntStatus(uint8_t id)
{
	return IP_CMP_GetIntStatus(LPC_CMP, id);
}

/**
 * @brief	Clears the CMP interrupt
 * @param	id		: Comparator ID
 * @return	Nothing
 */
STATIC INLINE void Chip_CMP_ClearIntStatus(uint8_t id)
{
	IP_CMP_CMP_ClearIntStatus(LPC_CMP, id);
}

#endif /*defined(CHIP_LPC407X_8X)*/

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CMP_17XX_40XX_H_ */
