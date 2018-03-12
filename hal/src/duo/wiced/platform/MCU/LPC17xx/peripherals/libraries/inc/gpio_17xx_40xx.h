/*
 * @brief LPC17xx/40xx GPIO registers and control functions
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

#ifndef __GPIO_17XX_40XX_H_
#define __GPIO_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup GPIO_17XX_40XX CHIP: LPC17xx/40xx GPIO driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief	Initialize GPIO block
 * @param	pGPIO	: The base of GPIO peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_GPIO_Init(LPC_GPIO_T *pGPIO)
{
    ( (void)(pGPIO) );
}

/**
 * @brief	Set a GPIO port/bit state
 * @param	pGPIO	: The base of GPIO peripheral on the chip
 * @param	port	: GPIO port to set
 * @param	bit		: GPIO bit to set
 * @param	setting	: true for high, false for low
 * @return	Nothing
 */
STATIC INLINE void Chip_GPIO_WritePortBit(LPC_GPIO_T *pGPIO, uint32_t port, uint8_t bit, bool setting)
{
    ( (void)(pGPIO) );
	IP_GPIO_WritePortBit((LPC_GPIO_T *) (LPC_GPIO + port), bit, setting);
}

/**
 * @brief	Set a GPIO direction
 * @param	pGPIO	: The base of GPIO peripheral on the chip
 * @param	port	: GPIO port to set
 * @param	bit		: GPIO bit to set
 * @param	setting	: true for output, false for input
 * @return	Nothing
 */
STATIC INLINE void Chip_GPIO_WriteDirBit(LPC_GPIO_T *pGPIO, uint8_t port, uint8_t bit, bool setting)
{
	IP_GPIO_WriteDirBit((LPC_GPIO_T *) (pGPIO + port), bit, setting);
}

/**
 * @brief	Read a GPIO state
 * @param	pGPIO	: The base of GPIO peripheral on the chip
 * @param	port	: GPIO port to read
 * @param	bit		: GPIO bit to read
 * @return	true of the GPIO is high, false if low
 */
STATIC INLINE bool Chip_GPIO_ReadPortBit(LPC_GPIO_T *pGPIO, uint32_t port, uint8_t bit)
{
	return IP_GPIO_ReadPortBit((LPC_GPIO_T *) (pGPIO + port), bit);
}

/**
 * @brief	Read a GPIO direction (out or in)
 * @param	pGPIO	: The base of GPIO peripheral on the chip
 * @param	port	: GPIO port to read
 * @param	bit		: GPIO bit to read
 * @return	true of the GPIO is an output, false if input
 */
STATIC INLINE bool Chip_GPIO_ReadDirBit(LPC_GPIO_T *pGPIO, uint32_t port, uint8_t bit)
{
	return IP_GPIO_ReadDirBit((LPC_GPIO_T *) (pGPIO + port), bit);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_17XX_40XX_H_ */
