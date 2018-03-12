/*
 * @brief LPC17xx/40xx Cyclic Redundancy Check (CRC) Engine driver
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

#ifndef __CRC_17XX_40XX_H_
#define __CRC_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CRC_17XX_40XX CHIP: LPC17xx/40xx CRC Driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/** CRC polynomial definitions */
typedef IP_CRC_001_POLY_T CRC_POLY_T;

/**
 * @brief	Initializes the CRC Engine
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_Init(void)
{
	IP_CRC_Init(LPC_CRC);
}

/**
 * @brief	Deinitializes the CRC Engine
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_Deinit(void)
{
	IP_CRC_DeInit(LPC_CRC);
}

/**
 * @brief	Set the polynomial used for the CRC calculation
 * @param	poly	: The enumerated polynomial to be used
 * @param	flags	: An Or'ed value of flags that setup the mode
 * @return	Nothing
 * @note	Flags for setting up the mode word include CRC_MODE_WRDATA_BIT_RVS,
 * CRC_MODE_WRDATA_CMPL, CRC_MODE_SUM_BIT_RVS, and CRC_MODE_SUM_CMPL.
 */
STATIC INLINE void Chip_CRC_SetPoly(CRC_POLY_T poly, uint32_t flags)
{
	IP_CRC_SetPoly(LPC_CRC, poly, flags);
}

/**
 * @brief	Engage the CRC engine with defaults based on the polynomial to be used
 * @param	poly	: The enumerated polynomial to be used
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_UseDefaultConfig(CRC_POLY_T poly)
{
	IP_CRC_UseDefaultConfig(LPC_CRC, poly);
}

/**
 * @brief	Set the CRC Mode bits directly
 * @param	mode	: A value indicating the mode
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_SetMode(uint32_t mode)
{
	IP_CRC_SetMode(LPC_CRC, mode);
}

/**
 * @brief	Get the CRC Mode bits directly
 * @return	The current value of the CRC Mode bits
 */
STATIC INLINE uint32_t Chip_CRC_GetMode(void)
{
	return IP_CRC_GetMode(LPC_CRC);
}

/**
 * @brief	Set the seed bits used by the CRC_SUM register
 * @param	seed	: The seeded value
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_SetSeed(uint32_t seed)
{
	IP_CRC_SetMode(LPC_CRC, seed);
}

/**
 * @brief	Get the CRC seed value
 * @return	The seeded value
 */
STATIC INLINE uint32_t Chip_CRC_GetSeed(void)
{
	return IP_CRC_GetSeed(LPC_CRC);
}

/**
 * @brief	Convenience function for writing 8-bit data to the CRC engine
 * @param	data	: 8-bit data to write
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_Write8(uint8_t data)
{
	IP_CRC_Write8(LPC_CRC, data);
}

/**
 * @brief	Convenience function for writing 16-bit data to the CRC engine
 * @param	data	: 16-bit data to write
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_Write16(uint16_t data)
{
	IP_CRC_Write16(LPC_CRC, data);
}

/**
 * @brief	Convenience function for writing 32-bit data to the CRC engine
 * @param	data	: 32-bit data to write
 * @return	Nothing
 */
STATIC INLINE void Chip_CRC_Write32(uint32_t data)
{
	IP_CRC_Write32(LPC_CRC, data);
}

/**
 * @brief	Gets the CRC Sum based on the Mode and Seed as previously configured
 * @return	The CRC "Checksum."
 */
STATIC INLINE uint32_t Chip_CRC_Sum(void)
{
	return IP_CRC_ReadSum(LPC_CRC);
}

/**
 * @brief	Convenience function for computing a standard CCITT checksum from an 8-bit data block
 * @param	data	: A pointer to the block of 8 bit data
 * @param   bytes	: The number of bytes pointed to by data
 * @return	Nothing
 */
uint32_t Chip_CRC_CRC8(const uint8_t *data, uint32_t bytes);

/**
 * @brief	Convenience function for computing a standard CRC16 checksum from 16-bit data block
 * @param	data	: A pointer to the block of 16-bit data
 * @param   hwords	: The number of 16 byte entries pointed to by data
 * @return	Nothing
 */
uint32_t Chip_CRC_CRC16(const uint16_t *data, uint32_t hwords);

/**
 * @brief	Convenience function for computing a standard CRC32 checksum from 32-bit data block
 * @param	data	: A pointer to the block of 32-bit data
 * @param   words	: The number of 32 byte entries pointed to by data
 * @return	Nothing
 */
uint32_t Chip_CRC_CRC32(const uint32_t *data, uint32_t words);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CRC_17XX_40XX_H_ */
