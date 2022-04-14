/*
 * @brief LPC17xx/40xx UART chip driver
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

#ifndef __UART_17XX_40XX_H_
#define __UART_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup 17XX_40XX CHIP: LPC17xx/40xx UART Driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief	Configure data width, parity mode and stop bits
 * @param	pUART		: Pointer to selected pUART peripheral
 * @param	Databits	: UART Data width, should be:
 *                          UART_DATABIT_5: UART 5 bit data mode
 *                          UART_DATABIT_6: UART 6 bit data mode
 *                          UART_DATABIT_7: UART 7 bit data mode
 *                          UART_DATABIT_8: UART 8 bit data mode
 * @param	Parity		: UART Parity mode, should be:
 *                          UART_PARITY_NONE: No parity
 *                          UART_PARITY_ODD:  Odd parity
 *                          UART_PARITY_EVEN: Even parity
 *                          UART_PARITY_SP_1: Forced "1" stick parity
 *                          UART_PARITY_SP_0: Forced "0" stick parity
 * @param	Stopbits	: Number of stop bits, should be:
 *                          UART_STOPBIT_1: One Stop Bit Select
 *                          UART_STOPBIT_2: Two Stop Bits Select
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_ConfigData(LPC_USART_T *pUART,
										IP_UART_DATABIT_T Databits,
										IP_UART_PARITY_T Parity,
										IP_UART_STOPBIT_T Stopbits)
{
	IP_UART_ConfigData(pUART, Databits, Parity, Stopbits);
}

/**
 * @brief	Send a block of data via UART peripheral
 * @param	pUART	: Pointer to selected pUART peripheral
 * @param	txbuf	: Pointer to Transmit buffer
 * @param	buflen	: Length of Transmit buffer
 * @param	flag	: Flag used in  UART transfer, should be NONE_BLOCKING or BLOCKING
 * @return	Number of bytes sent
 */
STATIC INLINE uint32_t Chip_UART_Send(LPC_USART_T *pUART, uint8_t *txbuf, uint32_t buflen, TRANSFER_BLOCK_T flag)
{
	return IP_UART_Send(pUART, txbuf, buflen, flag);
}

/**
 * @brief	Receive a block of data via UART peripheral
 * @param	pUART	: Pointer to selected pUART peripheral
 * @param	rxbuf	: Pointer to Received buffer
 * @param	buflen	: Length of Received buffer
 * @param	flag	: Flag mode, should be NONE_BLOCKING or BLOCKING
 * @return	Number of bytes received
 */
STATIC INLINE uint32_t Chip_UART_Receive(LPC_USART_T *pUART,
										 uint8_t *rxbuf,
										 uint32_t buflen,
										 TRANSFER_BLOCK_T flag)
{
	return IP_UART_Receive(pUART, rxbuf, buflen, flag);
}

/* UART FIFO functions ----------------------------------------------------------*/
/**
 * @brief	Configure FIFO function on selected UART peripheral
 * @param	pUART	: Pointer to selected pUART peripheral
 * @param	FIFOCfg	: Pointer to a UART_FIFO_CFG_T Structure that contains specified information about FIFO configuration
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_FIFOConfig(LPC_USART_T *pUART, UART_FIFO_CFG_T *FIFOCfg)
{
	IP_UART_FIFOConfig(pUART, FIFOCfg);
}

/**
 * @brief	Fills each UART_FIFOInitStruct member with its default value:
 *			- FIFO_DMAMode = DISABLE
 *			- FIFO_Level = UART_FIFO_TRGLEV0
 *			- FIFO_ResetRxBuf = ENABLE
 *			- FIFO_ResetTxBuf = ENABLE
 *			- FIFO_State = ENABLE
 * @param	pUART	: Pointer to selected UART peripheral
 * @param	UART_FIFOInitStruct	: Pointer to a UART_FIFO_CFG_T structure which will be initialized.
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_FIFOConfigStructInit(LPC_USART_T *pUART, UART_FIFO_CFG_T *UART_FIFOInitStruct)
{
    ( (void)(pUART) );
	IP_UART_FIFOConfigStructInit(UART_FIFOInitStruct);
}

/* UART operate functions -------------------------------------------------------*/
/**
 * @brief	Enable or disable specified UART interrupt.
 * @param	pUART		: Pointer to selected pUART peripheral
 * @param	UARTIntCfg	: Specifies the interrupt flag, should be one of the following:
 *                  - UART_INTCFG_RBR   : RBR Interrupt enable
 *                  - UART_INTCFG_THRE  : THR Interrupt enable
 *                  - UART_INTCFG_RLS   : RX line status interrupt enable
 *                  - UART1_INTCFG_MS	: Modem status interrupt enable (UART1 only)
 *                  - UART1_INTCFG_CTS	: CTS1 signal transition interrupt enable (UART1 only)
 *                  - UART_INTCFG_ABEO  : Enables the end of auto-baud interrupt
 *                  - UART_INTCFG_ABTO  : Enables the auto-baud time-out interrupt
 * @param	NewState	: New state of specified UART interrupt type, should be ENALBE or DISALBE
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_IntConfig(LPC_USART_T *pUART, IP_UART_INT_T UARTIntCfg, FunctionalState NewState)
{
	IP_UART_IntConfig(pUART, UARTIntCfg, NewState);
}

/**
 * @brief	Get Source Interrupt
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Return the value of IIR register
 */
STATIC INLINE uint32_t Chip_UART_IntGetStatus(LPC_USART_T *pUART)
{
	return IP_UART_IntGetStatus(pUART);
}

/**
 * @brief	Get current value of Line Status register in UART peripheral.
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Current value of Line Status register in UART peripheral
 */
STATIC INLINE uint8_t Chip_UART_GetLineStatus(LPC_USART_T *pUART)
{
	return IP_UART_GetLineStatus(pUART);
}

/**
 * @brief	Check whether if UART is busy or not
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	RESET if UART is not busy, otherwise return SET.
 */
STATIC INLINE FlagStatus Chip_UART_CheckBusy(LPC_USART_T *pUART)
{
	return IP_UART_CheckBusy(pUART);
}

/**
 * @brief	Force BREAK character on UART line, output pin pUART TXD is forced to logic 0
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_ForceBreak(LPC_USART_T *pUART)
{
	IP_UART_ForceBreak(pUART);
}

/**
 * @brief	Transmit a single data through UART peripheral
 * @param	pUART	: Pointer to selected pUART peripheral
 * @param	data	: Data to transmit (must be 8-bit long)
 * @return	Status, should be ERROR (THR is empty, ready to send) or SUCCESS (THR is not empty)
 */
STATIC INLINE Status Chip_UART_SendByte(LPC_USART_T *pUART, uint8_t data)
{
	return IP_UART_SendByte(pUART, data);
}

/**
 * @brief	Receive a single data from UART peripheral
 * @param	pUART	: Pointer to selected pUART peripheral
 * @param	Data	: Pointer to Data to receive (must be 8-bit long)
 * @return	Status, should be ERROR or (Receive data is ready) or SUCCESS (Receive data is not ready yet)
 */
STATIC INLINE Status Chip_UART_ReceiveByte(LPC_USART_T *pUART, uint8_t *Data)
{
	return IP_UART_ReceiveByte(pUART, Data);
}

/**
 * @brief	Start/Stop Auto Baudrate activity
 * @param	pUART			: Pointer to selected pUART peripheral
 * @param	ABConfigStruct	: A pointer to UART_AB_CFG_T structure that
 *          contains specified information about UAR auto baud configuration
 * @param	NewState		: New State of Auto baudrate activity, should be ENABLE or DISABLE
 * @return	Nothing
 * @note	Auto-baudrate mode enable bit will be cleared once this mode completed.
 */
STATIC INLINE void Chip_UART_ABCmd(LPC_USART_T *pUART, UART_AB_CFG_T *ABConfigStruct, FunctionalState NewState)
{
	IP_UART_ABCmd(pUART, ABConfigStruct, NewState);
}

/**
 * @brief	Clear Autobaud Interrupt
 * @param	pUART		: Pointer to selected pUART peripheral
 * @param	ABIntType	: type of auto-baud interrupt, should be:
 *				- UART_AUTOBAUD_INTSTAT_ABEO: End of Auto-baud interrupt
 *				- UART_AUTOBAUD_INTSTAT_ABTO: Auto-baud time out interrupt
 * @return	Nothing
 */
STATIC INLINE void Chip_UART_ABClearIntPending(LPC_USART_T *pUART, IP_UART_INT_STATUS_T ABIntType)
{
	IP_UART_ABClearIntPending(pUART, ABIntType);
}

/**
 * @brief	Initializes the pUART peripheral.
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Nothing
 */
void Chip_UART_Init(LPC_USART_T *pUART);

/**
 * @brief	De-initializes the pUART peripheral.
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Nothing
 */
void Chip_UART_DeInit(LPC_USART_T *pUART);

/**
 * @brief	Determines best dividers to get a target baud rate
 * @param	pUART		: Pointer to selected pUART peripheral
 * @param	baudrate	: Desired UART baud rate.
 * @return	Error status, could be SUCCESS or ERROR
 */
Status Chip_UART_SetBaud(LPC_USART_T *pUART, uint32_t baudrate);

/**
 * @brief	Enable/Disable transmission on UART TxD pin
 * @param	pUART		: Pointer to selected pUART peripheral
 * @param	NewState	: New State of Tx transmission function, should be ENABLE or DISABLE
 * @return Nothing
 */
void Chip_UART_TxCmd(LPC_USART_T *pUART, FunctionalState NewState);

/**
 * @brief	Get Interrupt Stream Status
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Return the interrupt status, should be:
 *              - UART_INTSTS_ERROR
 *              - UART_INTSTS_RTR
 *              - UART_INTSTS_RTS
 */
IP_UART_INT_STATUS_T Chip_UART_GetIntStatus(LPC_USART_T *pUART);

/**
 * @brief	Uart interrupt service routine (chip layer)
 * @param	pUART	: Pointer to selected pUART peripheral
 * @return	Nothing
 */
void Chip_UART_Interrupt_Handler (LPC_USART_T *pUART);

/**
 * @brief	UART transmit function for interrupt mode (using ring buffers)
 * @param	pUART	: Selected UART peripheral used to send data, should be UART0
 * @param	txbuf	: Pointer to Transmit buffer
 * @param	buflen	: Length of Transmit buffer
 * @return	Number of bytes actually sent to the ring buffer
 */
uint32_t Chip_UART_Interrupt_Transmit(LPC_USART_T *pUART, uint8_t *txbuf, uint8_t buflen);

/**
 * @brief	UART read function for interrupt mode (using ring buffers)
 * @param	pUART	: Selected UART peripheral used to send data, should be UART0
 * @param	rxbuf	: Pointer to Received buffer
 * @param	buflen	: Length of Received buffer
 * @return	Number of bytes actually read from the ring buffer
 */
uint32_t Chip_UART_Interrupt_Receive(LPC_USART_T *pUART, uint8_t *rxbuf, uint8_t buflen);

/**
 * @brief	Reset Tx and Rx ring buffer (head and tail)
 * @param	pUART	: Pointer to selected UART peripheral
 * @return	Nothing
 */
void Chip_UART_InitRingBuffer(LPC_USART_T *pUART);

// FIXME - this function is probably not interrupt related and needs a DoxyGen header
/* UART interrupt service routine */
FlagStatus Chip_UART_GetABEOStatus(LPC_USART_T *pUART);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif /* __UART_17XX_40XX_H_ */
