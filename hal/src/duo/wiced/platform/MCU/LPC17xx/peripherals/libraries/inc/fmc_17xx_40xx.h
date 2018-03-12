/*
 * @brief LPC17xx/40xx FLASH Memory Controller (FMC) driver
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

#ifndef __FMC_17XX_40XX_H_
#define __FMC_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup FMC_17XX_40XX CHIP: LPC17xx/40xx FLASH Memory Controller driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief FLASH Access time definitions
 */
typedef enum {
	FLASHTIM_20MHZ_CPU = 0,		/*!< Flash accesses use 1 CPU clocks. Use for up to 20 MHz CPU clock */
	FLASHTIM_40MHZ_CPU = 1,		/*!< Flash accesses use 2 CPU clocks. Use for up to 40 MHz CPU clock */
	FLASHTIM_60MHZ_CPU = 2,		/*!< Flash accesses use 3 CPU clocks. Use for up to 60 MHz CPU clock */
	FLASHTIM_80MHZ_CPU = 3,		/*!< Flash accesses use 4 CPU clocks. Use for up to 80 MHz CPU clock */
	FLASHTIM_100MHZ_CPU = 4,	/*!< Flash accesses use 5 CPU clocks. Use for up to 100 MHz CPU clock */
#if defined(CHIP_LPC177X_8X) || defined(CHIP_LPC407X_8X)
	FLASHTIM_120MHZ_CPU = 3,	/*!< Flash accesses use 4 CPU clocks. Use for up to 120 MHz CPU clock with power boot on*/
#else
	FLASHTIM_120MHZ_CPU = 4,	/*!< Flash accesses use 5 CPU clocks. Use for up to 120 Mhz for LPC1759 and LPC1769 only.*/
#endif
	FLASHTIM_SAFE_SETTING = 5,	/*!< Flash accesses use 6 CPU clocks. Safe setting for any allowed conditions */
} FMC_FLASHTIM_T;

/**
 * @brief	Set FLASH memory access time in clocks
 * @param	clks	: Clock cycles for FLASH access (minus 1)
 * @return	Nothing
 * @note	See the user manual for valid settings for this register for when
 * power boot is enabled or off.
 */
STATIC INLINE void Chip_FMC_SetFLASHAccess(FMC_FLASHTIM_T clks)
{
	uint32_t tmp = LPC_SYSCTL->FLASHCFG & 0xFFF;

	/* Don't alter lower bits */
	LPC_SYSCTL->FLASHCFG = tmp | (uint32_t)(clks << 12);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __FMC_17XX_40XX_H_ */
