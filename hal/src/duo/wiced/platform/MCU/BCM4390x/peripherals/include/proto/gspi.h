/*
 * gSPI Protocol
 *
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: gspi.h 382882 2013-02-04 23:24:31Z xwei $
 */

#define SPI_RW_FLAG_M			BITFIELD_MASK(1)	/* Bit [31] - R/W Command Bit */
#define SPI_RW_FLAG_S			31
#define SPI_ACCESS_M			BITFIELD_MASK(1)	/* Bit [30] - Fixed/Incr Access */
#define SPI_ACCESS_S			30
#define SPI_FUNCTION_M			BITFIELD_MASK(2)	/* Bits [29:28]	- Function Number */
#define SPI_FUNCTION_S			28
#define SPI_REG_ADDR_M			BITFIELD_MASK(17)	/* Bit [27:11] - Address */
#define SPI_REG_ADDR_S			11
#define SPI_LEN_M			BITFIELD_MASK(11)	/* Bit [10:0] - Packet length */
#define SPI_LEN_S			0

#define SPI_RSP_START_M		BITFIELD_MASK(1)	/* Bit [7] 	- Start Bit (always 0) */
#define SPI_RSP_START_S		7
#define SPI_RSP_PARAM_ERR_M	BITFIELD_MASK(1)	/* Bit [6] 	- Parameter Error */
#define SPI_RSP_PARAM_ERR_S	6
#define SPI_RSP_RFU5_M		BITFIELD_MASK(1)	/* Bit [5] 	- RFU (Always 0) */
#define SPI_RSP_RFU5_S		5
#define SPI_RSP_FUNC_ERR_M	BITFIELD_MASK(1)	/* Bit [4] 	- Function number error */
#define SPI_RSP_FUNC_ERR_S	4
#define SPI_RSP_CRC_ERR_M	BITFIELD_MASK(1)	/* Bit [3] 	- COM CRC Error */
#define SPI_RSP_CRC_ERR_S	3
#define SPI_RSP_ILL_CMD_M	BITFIELD_MASK(1)	/* Bit [2] 	- Illegal Command error */
#define SPI_RSP_ILL_CMD_S	2
#define SPI_RSP_RFU1_M		BITFIELD_MASK(1)	/* Bit [1] 	- RFU (Always 0) */
#define SPI_RSP_RFU1_S		1
#define SPI_RSP_IDLE_M		BITFIELD_MASK(1)	/* Bit [0] 	- In idle state */
#define SPI_RSP_IDLE_S		0
