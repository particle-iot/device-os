/*
 * @brief LPC17xx/40xx EEPROM driver
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

#ifndef EEPROM_17XX_40XX_H_
#define EEPROM_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup EEPROM_17XX_40XX CHIP: LPC17xx/40xx EEPROM Driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/* EEPROM read write size type */
typedef IP_EEPROM_RWSIZE_T EEPROM_RWSIZE_T;

/**
 * @brief	Initializes EEPROM
 * @param	pEEPROM	: The base of EEPROM peripheral on the chip
 * @return	Nothing
 */
void Chip_EEPROM_Init(LPC_EEPROM_T *pEEPROM);

/**
 * @brief	De-initializes EEPROM
 * @param	pEEPROM	: The base of EEPROM peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_DeInit(LPC_EEPROM_T *pEEPROM)
{
	IP_EEPROM_DeInit(pEEPROM);
}

/**
 * @brief	Set EEPROM wait state
 * @param	pEEPROM	: pointer to EEPROM peripheral block
 * @param	ws	: Wait State value.
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_SetWaitState(LPC_EEPROM_T *pEEPROM, uint32_t ws)
{
	IP_EEPROM_SetWaitState(pEEPROM, ws);
}

/**
 * @brief	Write data to EEPROM at specific address
 * @param	pEEPROM		: The base of EEPROM peripheral on the chip
 * @param	pageOffset	: offset of data in page register(0 - 63)
 * @param	pageAddress: page address (0-62)
 * @param	pData		: buffer that contain data that will be written to buffer
 * @param	wsize			: Write size:<br>
 *                  - EEPROM_RWSIZE_8BITS    : 8-bit read/write mode<br>
 *                  - EEPROM_RWSIZE_16BITS   : 16-bit read/write mode<br>
 *                  - EEPROM_RWSIZE_32BITS   : 32-bit read/write mode<br>
 * @param	byteNum		: number written data (bytes)
 * @return	SUCCESS on successful write of data, or ERROR
 * @note	This function actually write data into EEPROM memory and automatically
 * write into next page if current page is overflowed
 */
STATIC INLINE Status Chip_EEPROM_Write(LPC_EEPROM_T *pEEPROM, uint16_t pageOffset,
									   uint16_t pageAddress,
									   void *pData,
									   EEPROM_RWSIZE_T wsize,
									   uint32_t byteNum)
{
	return IP_EEPROM_Write(pEEPROM, pageOffset, pageAddress, pData, wsize, byteNum);
}

/**
 * @brief	Read data to EEPROM at specific address
 * @param	pEEPROM		: The base of EEPROM peripheral on the chip
 * @param	pageOffset	: offset of data in page register(0 - 63)
 * @param	pageAddress: page address (0-62)
 * @param	pData		: buffer that contain data read from read data register
 * @param	rsize		: Read size:<br>
 *                  - EEPROM_RWSIZE_8BITS    : 8-bit read/write mode<br>
 *                  - EEPROM_RWSIZE_16BITS   : 16-bit read/write mode<br>
 *                  - EEPROM_RWSIZE_32BITS   : 32-bit read/write mode<br>
 * @param	byteNum		: number read data (bytes)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_Read(LPC_EEPROM_T *pEEPROM, uint16_t pageOffset,
									uint16_t pageAddress,
									void *pData,
									EEPROM_RWSIZE_T rsize,
									uint32_t byteNum)
{
	IP_EEPROM_Read(pEEPROM, pageOffset, pageAddress, pData, rsize, byteNum);
}

/**
 * @brief	Erase a page at the specific address
 * @param	pEEPROM		: The base of EEPROM peripheral on the chip
 * @param	address		: EEPROM page address (0-62)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_Erase(LPC_EEPROM_T *pEEPROM, uint16_t address)
{
	IP_EEPROM_Erase(pEEPROM, address);
}

/**
 * @brief	Put EEPROM device in power down mode
 * @param	pEEPROM		: The base of EEPROM peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_EnablePowerDown(LPC_EEPROM_T *pEEPROM)
{
	IP_EEPROM_EnablePowerDown(pEEPROM);
}

/**
 * @brief	Bring EEPROM device out of power down mode
 * @param	pEEPROM		: The base of EEPROM peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_DisablePowerDown(LPC_EEPROM_T *pEEPROM)
{
	IP_EEPROM_DisablePowerDown(pEEPROM);
}

/**
 * @brief	Enable EEPROM interrupt
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @param	mask		: interrupt mask (or-ed bits value of EEPROM_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_EnableInt(LPC_EEPROM_T *pEEPROM, uint32_t mask)
{
	IP_EEPROM_EnableInt(pEEPROM, mask);
}

/**
 * @brief	Disable EEPROM interrupt
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @param	mask		: interrupt mask (or-ed bits value of EEPROM_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_DisableInt(LPC_EEPROM_T *pEEPROM, uint32_t mask)
{
	IP_EEPROM_DisableInt(pEEPROM, mask);
}

/**
 * @brief	Get the value of the EEPROM interrupt enable register
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @return	Or-ed bits value of EEPROM_INT_*
 */
STATIC INLINE uint32_t Chip_EEPROM_GetIntEnable(LPC_EEPROM_T *pEEPROM)
{
	return IP_EEPROM_GetIntEnable(pEEPROM);
}

/**
 * @brief	Get EEPROM interrupt status
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @return	Or-ed bits value of EEPROM_INT_*
 */
STATIC INLINE uint32_t Chip_EEPROM_GetIntStatus(LPC_EEPROM_T *pEEPROM)
{
	return IP_EEPROM_GetIntStatus(pEEPROM);
}

/**
 * @brief	Set EEPROM interrupt status
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @param	mask		: interrupt mask (or-ed bits value of EEPROM_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_SetIntStatus(LPC_EEPROM_T *pEEPROM, uint32_t mask)
{
	IP_EEPROM_SetIntStatus(pEEPROM, mask);
}

/**
 * @brief	Clear EEPROM interrupt status
 * @param	pEEPROM		: pointer to EEPROM peripheral block
 * @param	mask		: interrupt mask (or-ed bits value of EEPROM_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_EEPROM_ClearIntStatus(LPC_EEPROM_T *pEEPROM, uint32_t mask)
{
	IP_EEPROM_ClearIntStatus(pEEPROM, mask);
}

/**
 * @}
 */

 #ifdef __cplusplus
}
#endif

#endif /* EEPROM_17XX_40XX_H_ */
