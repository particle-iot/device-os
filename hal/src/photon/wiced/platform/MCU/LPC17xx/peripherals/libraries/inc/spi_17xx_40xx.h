/*
 * @brief LPC17xx/40xx SPI driver
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

#ifndef __SPI_17XX_40XX_H_
#define __SPI_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SPI_17XX_40XX CHIP: LPC17xx/40xx SPI driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

#define CHIP_SPI_CLOCK_MODE_T       IP_SPI_CLOCK_MODE_T
#define CHIP_SPI_MODE_T             IP_SPI_MODE_T
#define CHIP_SPI_DATA_ORDER_T       IP_SPI_DATA_ORDER_T
#define CHIP_SPI_BITS_T             IP_SPI_BITS_T

/** SPI callback function type*/
typedef void (*CHIP_SPI_CALLBACK_T)(void);
/*
 * @brief SPI config format
 */
typedef struct CHIP_SPI_CONFIG_FORMAT_T {
	CHIP_SPI_BITS_T bits;						/*!< bits/frame */
	CHIP_SPI_CLOCK_MODE_T clockMode;	/*!< Format config: clock phase/polarity */
	CHIP_SPI_DATA_ORDER_T dataOrder;	/*!< Data order (MSB first/LSB first) */
} CHIP_SPI_CONFIG_FORMAT_T;

/*
 * @brief SPI data setup structure
 */
typedef struct {
	uint8_t      *pTxData;					/*!< Pointer to transmit data */
	uint8_t      *pRxData;					/*!< Pointer to receive data */
	uint32_t  cnt;							/*!< Transfer counter */
	uint32_t  length;						/*!< Length of transfer data */
	CHIP_SPI_CALLBACK_T    fnBefFrame;				/*!< Function to call before sending frame */
	CHIP_SPI_CALLBACK_T    fnAftFrame;				/*!< Function to call after sending frame */
	CHIP_SPI_CALLBACK_T    fnBefTransfer;			/*!< Function to call before starting a transfer */
	CHIP_SPI_CALLBACK_T    fnAftTransfer;			/*!< Function to call after a transfer complete */
} CHIP_SPI_DATA_SETUP_T;

/**
 * @brief	Get the current status of SPI controller
 * @return	SPI controller status (Or-ed value of SPI_SR_*)
 */
STATIC INLINE uint32_t Chip_SPI_GetStatus(LPC_SPI_T *pSPI)
{
	return IP_SPI_GetStatus(pSPI);
}

/**
 * @brief   Clean all data in RX FIFO of SPI
 * @param	pSPI			: The base SPI peripheral on the chip
 * @return	Nothing
 */
void Chip_SPI_Int_FlushData(LPC_SPI_T *pSPI);

/**
 * @brief   SPI Interrupt Read/Write with 8-bit frame width
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	xf_setup		: Pointer to a SPI_DATA_SETUP_T structure that contains specified
 *                          information about transmit/receive data	configuration
 * @return	SUCCESS or ERROR
 */
Status Chip_SPI_Int_RWFrames8Bits(LPC_SPI_T *pSPI, CHIP_SPI_DATA_SETUP_T *xf_setup);

/**
 * @brief   SPI Interrupt Read/Write with 16-bit frame width
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	xf_setup		: Pointer to a SPI_DATA_SETUP_T structure that contains specified
 *                          information about transmit/receive data	configuration
 * @return	SUCCESS or ERROR
 */
Status Chip_SPI_Int_RWFrames16Bits(LPC_SPI_T *pSPI, CHIP_SPI_DATA_SETUP_T *xf_setup);

/**
 * @brief   SPI Polling Read/Write in blocking mode
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	pXfSetup		: Pointer to a SPI_DATA_SETUP_T structure that contains specified
 *                          information about transmit/receive data	configuration
 * @return	Actual data length has been transferred
 * @note
 * This function can be used in both master and slave mode. It starts with writing phase and after that,
 * a reading phase is generated to read any data available in RX_FIFO. All needed information is prepared
 * through xf_setup param.
 */
uint32_t Chip_SPI_RWFrames_Blocking(LPC_SPI_T *pSPI, CHIP_SPI_DATA_SETUP_T *pXfSetup);

/**
 * @brief   SPI Polling Write in blocking mode
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	buffer			: Buffer address
 * @param	buffer_len		: Buffer length
 * @return	Actual data length has been transferred
 * @note
 * This function can be used in both master and slave mode. First, a writing operation will send
 * the needed data. After that, a dummy reading operation is generated to clear data buffer
 */
uint32_t Chip_SPI_WriteFrames_Blocking(LPC_SPI_T *pSPI, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief   SPI Polling Read in blocking mode
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	buffer			: Buffer address
 * @param	buffer_len		: The length of buffer
 * @return	Actual data length has been transferred
 * @note
 * This function can be used in both master and slave mode. First, a dummy writing operation is generated
 * to clear data buffer. After that, a reading operation will receive the needed data
 */
uint32_t Chip_SPI_ReadFrames_Blocking(LPC_SPI_T *pSPI, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief   Initialize the SPI
 * @param	pSPI			: The base SPI peripheral on the chip
 * @return	Nothing
 */
void Chip_SPI_Init(LPC_SPI_T *pSPI);

/**
 * @brief	Deinitialise the SPI
 * @param	pSPI	: The base of SPI peripheral on the chip
 * @return	Nothing
 * @note	The SPI controller is disabled
 */
void Chip_SPI_DeInit(LPC_SPI_T *pSPI);

/**
 * @brief   Set the SPI operating modes, master or slave
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	mode		: master mode/slave mode
 * @return	Nothing
 */
STATIC INLINE void Chip_SPI_SetMode(LPC_SPI_T *pSPI, CHIP_SPI_MODE_T mode)
{
	IP_SPI_SetMode(pSPI, mode);
}

/**
 * @brief   Set the clock frequency for SPI interface
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	bitRate		: The SPI bit rate
 * @return	Nothing
 */
void Chip_SPI_SetBitRate(LPC_SPI_T *pSPI, uint32_t bitRate);

/**
 * @brief   Set up the SPI frame format
 * @param	pSPI			: The base SPI peripheral on the chip
 * @param	format			: Pointer to Frame format structure
 * @return	Nothing
 */
STATIC INLINE void Chip_SPI_SetFormat(LPC_SPI_T *pSPI, CHIP_SPI_CONFIG_FORMAT_T *format)
{
	IP_SPI_SetFormat(pSPI, format->bits, format->clockMode, format->dataOrder);
}

/**
 * @brief   Enable SPI interrupt
 * @param	pSPI			: The base SPI peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_SPI_Int_Enable(LPC_SPI_T *pSPI)
{
	IP_SPI_IntEnable(pSPI);
}

/**
 * @brief   Disable SPI interrupt
 * @param	pSPI			: The base SPI peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_SPI_Int_Disable(LPC_SPI_T *pSPI)
{
	IP_SPI_IntDisable(pSPI);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_17XX_40XX_H_ */
