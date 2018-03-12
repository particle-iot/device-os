/*
 * @brief LPC17xx/40xx SD Card Interface driver
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

#ifndef __SDC_17XX_40XX_H_
#define __SDC_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SDC_17XX_40XX CHIP: LPC17xx/40xx SDC driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief SDC Return code definitions
 */
typedef enum CHIP_SDC_RET_CODE {
	SDC_RET_OK = 0,
	SDC_RET_CMD_FAILED = -1,
	SDC_RET_BAD_PARAMETERS = -2,
	SDC_RET_BUS_NOT_IDLE = -3,
	SDC_RET_TIMEOUT = -4,
	SDC_RET_ERR_STATE = -5,
	SDC_RET_NOT_READY = -6,
	SDC_RET_FAILED = -7,
} CHIP_SDC_RET_CODE_T;

/**
 * @brief	Initialize SDC peripheral
 * @param	pSDC	: Pointer to SDC peripheral base address
 * @return	None
 */
void Chip_SDC_Init(LPC_SDC_T *pSDC);

/**
 * @brief	Set the clock frequency for SDC peripheral
 * @param	pSDC	: Pointer to SDC peripheral base address
 * @param	freq	: Expected clock frequency
 * @return	None
 */
void Chip_SDC_SetClock(LPC_SDC_T *pSDC, uint32_t freq);

/**
 * @brief	Deinitialise SDC peripheral
 * @param	pSDC	: Pointer to SDC peripheral base address
 * @return	None
 */
void Chip_SDC_DeInit(LPC_SDC_T *pSDC);

/**
 * @brief	Sets the interrupt mask in SDC peripheral
 * @param	pSDC	: Pointer to SDC peripheral base address
 * @param	iVal	: Interrupts to enable, Or'ed values SDC_MASK0_*
 * @return	None
 */
STATIC INLINE void Chip_SDC_SetIntMask(LPC_SDC_T *pSDC, uint32_t iVal)
{
	IP_SDC_SetIntMask(pSDC, iVal);
}

/**
 * @brief	Returns the current SDC status
 * @param	pSDC	: Pointer to SDC peripheral base address
 * @return	Current status of Or'ed values SDC_STATUS_*
 */
STATIC INLINE uint32_t Chip_SDC_GetStatus(LPC_SDC_T *pSDC)
{
	return IP_SDC_GetStatus(pSDC);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __SDC_17XX_40XX_H_ */
