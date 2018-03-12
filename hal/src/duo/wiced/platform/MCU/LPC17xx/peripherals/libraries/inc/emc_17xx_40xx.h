/*
 * @brief LPC17xx/40xx EMC driver
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

#ifndef __EMC_17XX_40XX_H_
#define __EMC_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup EMC_17XX_40XX CHIP: LPC17xx/40xx EMC Driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 * The EMC interface clocks must be enabled outside this driver prior to
 * calling any function of this driver with the
 * Chip_Clock_EnablePeripheralClock() function.
 */

/**
 * Dynamic Chip Select Address
 */
#define EMC_ADDRESS_DYCS0   (0xA0000000)
#define EMC_ADDRESS_DYCS1   (0xB0000000)
#define EMC_ADDRESS_DYCS2   (0xC0000000)
#define EMC_ADDRESS_DYCS3   (0xD0000000)

/**
 * Static Chip Select Address
 */
#define EMC_ADDRESS_CS0     (0x80000000)
#define EMC_ADDRESS_CS1     (0x90000000)
#define EMC_ADDRESS_CS2     (0x98000000)
#define EMC_ADDRESS_CS3     (0x9C000000)

/**
 * @brief	Dyanmic memory setup
 * @param	Dynamic_Config	: Pointer to dynamic memory setup data
 * @return	None
 */
void Chip_EMC_Dynamic_Init(IP_EMC_DYN_CONFIG_T *Dynamic_Config);

/**
 * @brief	Static memory setup
 * @param	Static_Config	: Pointer to static memory setup data
 * @return	None
 */
void Chip_EMC_Static_Init(IP_EMC_STATIC_CONFIG_T *Static_Config);

/**
 * @brief	Enable Dynamic Memory Controller
 * @param	Enable	: 1 = Enable Dynamic Memory Controller, 0 = Disable
 * @return	None
 */
STATIC INLINE void Chip_EMC_Dynamic_Enable(uint8_t Enable)
{
	IP_EMC_Dynamic_Enable(LPC_EMC, Enable);
}

/**
 * @brief	Mirror CS1 to CS0 and DYCS0
 * @param	Enable	: 1 = Mirror, 0 = Normal Memory Map
 * @return	None
 */
STATIC INLINE void Chip_EMC_Mirror(uint8_t Enable)
{
	IP_EMC_Mirror(LPC_EMC, Enable);
}

/**
 * @brief	Enable EMC
 * @param	Enable	: 1 = Enable, 0 = Disable
 * @return	None
 */
STATIC INLINE void Chip_EMC_Enable(uint8_t Enable)
{
	IP_EMC_Enable(LPC_EMC, Enable);
}

/**
 * @brief	Set EMC LowPower Mode
 * @param	Enable	: 1 = Enable, 0 = Disable
 * @return	None
 * @note	This function should only be called when the memory
 * controller is not busy (bit 0 of the status register is not set).
 */
STATIC INLINE void Chip_EMC_LowPowerMode(uint8_t Enable)
{
	IP_EMC_LowPowerMode(LPC_EMC, Enable);
}

/**
 * @brief	Initialize EMC
 * @param	Enable		: 1 = Enable, 0 = Disable
 * @param	ClockRatio	: clock out ratio, 0 = 1:1, 1 = 1:2
 * @param	EndianMode	: Endian Mode, 0 = Little, 1 = Big
 * @return	None
 */
STATIC INLINE void Chip_EMC_Init(uint32_t Enable, uint32_t ClockRatio, uint32_t EndianMode)
{
	IP_EMC_Init(LPC_EMC, Enable, ClockRatio, EndianMode);
}

/**
 * @brief	Set Static Memory Extended Wait in Clock
 * @param	Wait16Clks	: Number of '16 clock' delay cycles
 * @return	None
 */
STATIC INLINE void Chip_EMC_SetStaticExtendedWait(uint32_t Wait16Clks)
{
	IP_EMC_SetStaticExtendedWait(LPC_EMC, Wait16Clks);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __EMC_17XX_40XX_H_ */
