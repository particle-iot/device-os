/*
 * @brief Error code returned by Boot ROM drivers/library functions
 *  @ingroup Common
 *
 *  This file contains unified error codes to be used across driver,
 *  middleware, applications, hal and demo software.
 *
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

#ifndef __LPC_ERROR_H__
#define __LPC_ERROR_H__

/** Error code returned by Boot ROM drivers/library functions
 *
 *  Error codes are a 32-bit value with :
 *      - The 16 MSB contains the peripheral code number
 *      - The 16 LSB contains an error code number associated to that peripheral
 *
 */
typedef enum {
	/**\b 0x00000000*/ LPC_OK = 0,	/**< enum value returned on Success */
	/**\b 0xFFFFFFFF*/ ERR_FAILED = -1,	/**< enum value returned on general failure */
	/**\b 0xFFFFFFFE*/ ERR_TIME_OUT = -2,	/**< enum value returned on general timeout */
	/**\b 0xFFFFFFFD*/ ERR_BUSY = -3,	/**< enum value returned when resource is busy */

	/* ISP related errors */
	ERR_ISP_BASE = 0x00000000,
	/*0x00000001*/ ERR_ISP_INVALID_COMMAND = ERR_ISP_BASE + 1,
	/*0x00000002*/ ERR_ISP_SRC_ADDR_ERROR,	/* Source address not on word boundary */
	/*0x00000003*/ ERR_ISP_DST_ADDR_ERROR,	/* Destination address not on word or 256 byte boundary */
	/*0x00000004*/ ERR_ISP_SRC_ADDR_NOT_MAPPED,
	/*0x00000005*/ ERR_ISP_DST_ADDR_NOT_MAPPED,
	/*0x00000006*/ ERR_ISP_COUNT_ERROR,	/* Byte count is not multiple of 4 or is not a permitted value */
	/*0x00000007*/ ERR_ISP_INVALID_SECTOR,
	/*0x00000008*/ ERR_ISP_SECTOR_NOT_BLANK,
	/*0x00000009*/ ERR_ISP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION,
	/*0x0000000A*/ ERR_ISP_COMPARE_ERROR,
	/*0x0000000B*/ ERR_ISP_BUSY,/* Flash programming hardware interface is busy */
	/*0x0000000C*/ ERR_ISP_PARAM_ERROR,	/* Insufficient number of parameters */
	/*0x0000000D*/ ERR_ISP_ADDR_ERROR,	/* Address not on word boundary */
	/*0x0000000E*/ ERR_ISP_ADDR_NOT_MAPPED,
	/*0x0000000F*/ ERR_ISP_CMD_LOCKED,	/* Command is locked */
	/*0x00000010*/ ERR_ISP_INVALID_CODE,/* Unlock code is invalid */
	/*0x00000011*/ ERR_ISP_INVALID_BAUD_RATE,
	/*0x00000012*/ ERR_ISP_INVALID_STOP_BIT,
	/*0x00000013*/ ERR_ISP_CODE_READ_PROTECTION_ENABLED,

	/* ROM API related errors */
	ERR_API_BASE = 0x00010000,
	/**\b 0x00010001*/ ERR_API_INVALID_PARAMS = ERR_API_BASE + 1,	/**< Invalid parameters*/
	/**\b 0x00010002*/ ERR_API_INVALID_PARAM1,	/**< PARAM1 is invalid */
	/**\b 0x00010003*/ ERR_API_INVALID_PARAM2,	/**< PARAM2 is invalid */
	/**\b 0x00010004*/ ERR_API_INVALID_PARAM3,	/**< PARAM3 is invalid */
	/**\b 0x00010005*/ ERR_API_MOD_INIT,/**< API is called before module init */

	/* SPIFI API related errors */
	ERR_SPIFI_BASE = 0x00020000,
	/*0x00020001*/ ERR_SPIFI_DEVICE_ERROR = ERR_SPIFI_BASE + 1,
	/*0x00020002*/ ERR_SPIFI_INTERNAL_ERROR,
	/*0x00020003*/ ERR_SPIFI_TIMEOUT,
	/*0x00020004*/ ERR_SPIFI_OPERAND_ERROR,
	/*0x00020005*/ ERR_SPIFI_STATUS_PROBLEM,
	/*0x00020006*/ ERR_SPIFI_UNKNOWN_EXT,
	/*0x00020007*/ ERR_SPIFI_UNKNOWN_ID,
	/*0x00020008*/ ERR_SPIFI_UNKNOWN_TYPE,
	/*0x00020009*/ ERR_SPIFI_UNKNOWN_MFG,

	/* Security API related errors */
	ERR_SEC_BASE = 0x00030000,
	/*0x00030001*/ ERR_SEC_AES_WRONG_CMD = ERR_SEC_BASE + 1,
	/*0x00030002*/ ERR_SEC_AES_NOT_SUPPORTED,
	/*0x00030003*/ ERR_SEC_AES_KEY_ALREADY_PROGRAMMED,

	/* USB device stack related errors */
	ERR_USBD_BASE = 0x00040000,
	/**\b 0x00040001*/ ERR_USBD_INVALID_REQ = ERR_USBD_BASE + 1,/**< invalid request */
	/**\b 0x00040002*/ ERR_USBD_UNHANDLED,	/**< Callback did not process the event */
	/**\b 0x00040003*/ ERR_USBD_STALL,	/**< Stall the endpoint on which the call back is called */
	/**\b 0x00040004*/ ERR_USBD_SEND_ZLP,	/**< Send ZLP packet on the endpoint on which the call back is called */
	/**\b 0x00040005*/ ERR_USBD_SEND_DATA,	/**< Send data packet on the endpoint on which the call back is called */
	/**\b 0x00040006*/ ERR_USBD_BAD_DESC,	/**< Bad descriptor*/
	/**\b 0x00040007*/ ERR_USBD_BAD_CFG_DESC,	/**< Bad config descriptor*/
	/**\b 0x00040008*/ ERR_USBD_BAD_INTF_DESC,	/**< Bad interface descriptor*/
	/**\b 0x00040009*/ ERR_USBD_BAD_EP_DESC,/**< Bad endpoint descriptor*/
	/**\b 0x0004000a*/ ERR_USBD_BAD_MEM_BUF,/**< Bad alignment of buffer passed. */
	/**\b 0x0004000b*/ ERR_USBD_TOO_MANY_CLASS_HDLR,/**< Too many class handlers. */

	/* CGU  related errors */
	ERR_CGU_BASE = 0x00050000,
	/*0x00050001*/ ERR_CGU_NOT_IMPL = ERR_CGU_BASE + 1,
	/*0x00050002*/ ERR_CGU_INVALID_PARAM,
	/*0x00050003*/ ERR_CGU_INVALID_SLICE,
	/*0x00050004*/ ERR_CGU_OUTPUT_GEN,
	/*0x00050005*/ ERR_CGU_DIV_SRC,
	/*0x00050006*/ ERR_CGU_DIV_VAL,
	/*0x00050007*/ ERR_CGU_SRC,

	/* I2C related errors */
	ERR_I2C_BASE = 0x00060000,
	/*0x00060001*/ ERR_I2C_NAK = ERR_I2C_BASE + 1,
	/*0x00060002*/ ERR_I2C_BUFFER_OVERFLOW,
	/*0x00060003*/ ERR_I2C_BYTE_COUNT_ERR,
	/*0x00060004*/ ERR_I2C_LOSS_OF_ARBRITRATION,
	/*0x00060005*/ ERR_I2C_SLAVE_NOT_ADDRESSED,
	/*0x00060006*/ ERR_I2C_LOSS_OF_ARBRITRATION_NAK_BIT,
	/*0x00060007*/ ERR_I2C_GENERAL_FAILURE,
	/*0x00060008*/ ERR_I2C_REGS_SET_TO_DEFAULT,
	/*0x00060009*/ ERR_I2C_TIMEOUT,
	/*0x0006000A*/ ERR_I2C_BUFFER_UNDERFLOW,
	/*0x0006000B*/ ERR_I2C_UNKNOWN_MODE,
	/*0x0006000C*/ ERR_I2C_PARAM,
	/*0x0006000D*/ ERR_I2C_DMA_SETUP,
	/*0x0006000E*/ ERR_I2C_BUS_ERROR,

	/* UART related errors */
	ERR_UART_BASE = 0x00080000,
	/**\b 0x00080001*/ ERR_UART_RXD_BUSY = ERR_UART_BASE + 1, /*!< Receive is busy */
	/**\b 0x00080002*/ ERR_UART_TXD_BUSY, /*!< Transmit is busy */
	/**\b 0x00080003*/ ERR_UART_OVERRUN_FRAME_PARITY_NOISE, /*!< Overrun, Frame, Parity , Receive Noise error */
	/**\b 0x00080004*/ ERR_UART_UNDERRUN, /*!< Underrun */
	/**\b 0x00080005*/ ERR_UART_PARAM, /*!< Parameter error */

	ERR_DMA_BASE = 0x000D0000,
	/*0x000D0001*/ ERR_DMA_ERROR_INT = ERR_DMA_BASE + 1,
	/*0x000D0002*/ ERR_DMA_CHANNEL_NUMBER,
	/*0x000D0003*/ ERR_DMA_CHANNEL_DISABLED,
	/*0x000D0004*/ ERR_DMA_BUSY,
	/*0x000D0005*/ ERR_DMA_NOT_ALIGNMENT,
	/*0x000D0006*/ ERR_DMA_PING_PONG_EN,
	/*0x000D0007*/ ERR_DMA_CHANNEL_VALID_PENDING,
	
	/* SPI related errors */
	ERR_SPI_BASE = 0x000E0000,
	/*0x000E0001*/ ERR_SPI_RXOVERRUN=ERR_SPI_BASE+1,
	/*0x000E0002*/ ERR_SPI_TXUNDERRUN,
	/*0x000E0003*/ ERR_SPI_SELNASSERT,
	/*0x000E0004*/ ERR_SPI_SELNDEASSERT,
	/*0x000E0005*/ ERR_SPI_CLKSTALL,
	/*0x000E0006*/ ERR_SPI_PARAM,
	/*0x000E0007*/ ERR_SPI_INVALID_LENGTH,

	/* ADC related errors */
	ERR_ADC_BASE = 0x000F0000,
	/*0x000F0001*/ ERR_ADC_OVERRUN = ERR_ADC_BASE + 1,
	/*0x000F0002*/ ERR_ADC_INVALID_CHANNEL,
	/*0x000F0003*/ ERR_ADC_INVALID_SEQUENCE,
	/*0x000F0004*/ ERR_ADC_INVALID_SETUP,
	/*0x000F0005*/ ERR_ADC_PARAM,
	/*0x000F0006*/ ERR_ADC_INVALID_LENGTH,
	/*0x000F0007*/ ERR_ADC_NO_POWER
} ErrorCode_t;

#ifndef offsetof
#define offsetof(s, m)   (int) &(((s *) 0)->m)
#endif

#define COMPILE_TIME_ASSERT(pred)    switch (0) { \
	case 0:	\
	case pred:; }

#endif /* __LPC_ERROR_H__ */
