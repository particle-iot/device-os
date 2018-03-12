/*
 * @brief  GPIO Registers and Functions
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

#ifndef __GPIO_002_H_
#define __GPIO_002_H_

#include "sys_config.h"
#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup IP_GPIO_002 IP: GPIO register block and driver (002)
 * @ingroup IP_Drivers
 * Typically used for 175x/6x/7x/8x and 407x/8x devices.
 * @{
 */

#define GPIO_PORT_BITS 32

/**
 * @brief GPIO port  (GPIO_PORT) for LPC175x_6x, LPC177x_8x and LPC407x_8x
 */

typedef struct {				/* GPIO_PORT Structure */
	__IO uint32_t DIR;			/*!< Offset 0x0000: GPIO Port Direction control register */
	uint32_t RESERVED0[3];
	__IO uint32_t MASK;			/*!< Offset 0x0010: GPIO Mask register */
	__IO uint32_t PIN;			/*!< Offset 0x0014: Pin value register using FIOMASK */
	__IO uint32_t SET;			/*!< Offset 0x0018: Output Set register using FIOMASK */
	__O  uint32_t CLR;			/*!< Offset 0x001C: Output Clear register using FIOMASK */
} IP_GPIO_002_T;

/**
 * @brief	Initialize GPIO block
 * @param	pGPIO	: The Base Address of the GPIO block (from GPIO0 to GPIO5)
 * @return	Nothing
 */
STATIC INLINE void IP_GPIO_Init(IP_GPIO_002_T *pGPIO)
{
    ( (void)(pGPIO) );
}

/**
 * @brief	Set a GPIO port/bit state
 * @param	pGPIO	: The Base Address of the GPIO block (from GPIO0 to GPIO5)
 * @param	Bit		: GPIO bit number to be set/cleared
 * @param	Setting	: true for high, false for low
 * @return	Nothing
 */
STATIC INLINE void IP_GPIO_WritePortBit(IP_GPIO_002_T *pGPIO, uint8_t Bit, bool Setting)
{
	if (Setting) {	/* Set Port */
		pGPIO->SET |= 1UL << Bit;
	}
	else {	/* Clear Port */
		pGPIO->CLR |= 1UL << Bit;
	}
}

/**
 * @brief	Set a GPIO direction
 * @param	pGPIO	: The Base Address of the GPIO block (from GPIO0 to GPIO5)
 * @param	Bit		: GPIO bit number to be configured
 * @param	Setting	: true for output, false for input
 * @return	Nothing
 */
STATIC INLINE void IP_GPIO_WriteDirBit(IP_GPIO_002_T *pGPIO, uint8_t Bit, bool Setting)
{
	if (Setting) {
		pGPIO->DIR |= 1UL << Bit;
	}
	else {
		pGPIO->DIR &= ~(1UL << Bit);
	}
}

/**
 * @brief	Read a GPIO state
 * @param	pGPIO	: The Base Address of the GPIO block (from GPIO0 to GPIO5)
 * @param	Bit		: GPIO bit to read
 * @return	true of the GPIO is high, false if low
 */
STATIC INLINE bool IP_GPIO_ReadPortBit(IP_GPIO_002_T *pGPIO, uint8_t Bit)
{
	return (bool) ((pGPIO->PIN >> Bit) & 1);
}

/**
 * @brief	Read a GPIO direction (out or in)
 * @param	pGPIO	: The Base Address of the GPIO block (from GPIO0 to GPIO5)
 * @param	Bit		: GPIO bit to read
 * @return	true of the GPIO is an output, false if input
 */
STATIC INLINE bool IP_GPIO_ReadDirBit(IP_GPIO_002_T *pGPIO, uint8_t Bit)
{
	return (bool) (((pGPIO->DIR) >> Bit) & 1);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_002_H_ */
