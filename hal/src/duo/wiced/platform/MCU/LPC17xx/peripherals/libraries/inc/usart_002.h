/*
 * @brief	UART/USART2 Registers and control functions
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

#ifndef __USART_002_H_
#define __USART_002_H_

#include "sys_config.h"
#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup IP_USART_002 IP: USART2 register block and driver
 * @ingroup IP_Drivers
 * @{
 */

/**
 * @brief USART2 register block structure
 */
typedef struct {
	__IO uint32_t  CFG;				/*!< Configuration register */
	__IO uint32_t  CTRL;			/*!< Control register */
	__IO uint32_t  STAT;			/*!< Status register */
	__IO uint32_t  INTENSET;		/*!< Interrupt Enable read and set register */
	__O  uint32_t  INTENCLR;		/*!< Interrupt Enable clear register */
	__I  uint32_t  RXDATA;			/*!< Receive Data register */
	__I  uint32_t  RXDATA_STAT;		/*!< Receive Data with status register */
	__IO uint32_t  TXDATA;			/*!< Transmit data register */
	__IO uint32_t  BRG;				/*!< Baud Rate Generator register */
	__IO uint32_t  INTSTAT;			/*!< Interrupt status register */
} IP_USART_002_T;

/**
 * @brief UART Parity type definitions
 */
typedef enum IP_UART_002_PARITY {
	UART_PARITY_NONE = (0x00 << 4),		/*!< No parity */
	UART_PARITY_EVEN = (0x02 << 4),		/*!< Odd parity */
	UART_PARITY_ODD  = (0x03 << 4)		/*!< Even parity */
} IP_UART_002_PARITY_T;

/**
 * @brief FIFO Level type definitions
 */
typedef enum IP_UART_002_FITO_LEVEL {
	UART_FIFO_TRGLEV0 = 0,	/*!< UART FIFO trigger level 0: 1 character */
	UART_FIFO_TRGLEV1,		/*!< UART FIFO trigger level 1: 4 character */
	UART_FIFO_TRGLEV2,		/*!< UART FIFO trigger level 2: 8 character */
	UART_FIFO_TRGLEV3		/*!< UART FIFO trigger level 3: 14 character */
} IP_UART_002_FITO_LEVEL_T;

/**
 * @brief UART Stop bit type definitions
 */
typedef enum IP_UART_002_STOPLEN {
	UART_STOPLEN_1 = (0 << 6),		/*!< UART One Stop Bit Select */
	UART_STOPLEN_2 = (1 << 6)		/*!< UART Two Stop Bits Select */
} IP_UART_002_STOPLEN_T;

/**
 * @brief UART Databit length type definitions
 */
typedef enum IP_UART_002_DATALEN {
	UART_DATALEN_7  = (0x00 << 2),	/*!< UART 7 bit length mode */
	UART_DATALEN_8  = (0x01 << 2),	/*!< UART 8 bit length mode */
	UART_DATALEN_9  = (0x02 << 2)	/*!< UART 9 bit length mode */
} IP_UART_002_DATALEN_T;

/**
 * @brief UART clock polarity and sampling edge of received data type definitions
 */
typedef enum IP_UART_002_CLKPOL {
	UART_CLKPOL_FALLING = (0 << 12),/*!< Falling edge. Un_RXD is sampled on the falling edge of SCLK */
	UART_CLKPOL_RISING = (1 << 12)	/*!< Rising edge. Un_RXD is sampled on the rising edge of SCLK */
} IP_UART_002_CLKPOL_T;

/* UART interrupt bit definitions */
#define RXRDY_INT         (0x01 << 0)		/*!< Receive Ready interrupt */
#define TXRDY_INT         (0x01 << 2)		/*!< Transmit Ready interrupt */
#define CTS_DELTA_INT     (0x01 << 5)		/*!< Change in CTS state interrupt */
#define TXINT_DIS_INT     (0x01 << 6)		/*!< Transmitter disable interrupt */
#define OVRN_ERR_INT      (0x01 << 8)		/*!< Overrun error interrupt */
#define DELTA_RXBRK_INT   (0x01 << 11)		/*!< Change in receiver break detection interrupt */
#define START_DETECT_INT  (0x01 << 12)		/*!< Start detect interrupt */
#define FRM_ERR_INT       (0x01 << 13)		/*!< Frame error interrupt */
#define PAR_ERR_INT       (0x01 << 14)		/*!< Parity error interrupt */
#define RXNOISE_INT       (0x01 << 15)		/*!< Received noise interrupt */

/* UART status bit definitions */
#define RXRDY         (0x01 << 0)		/*!< Receiver ready */
#define RXIDLE        (0x01 << 1)			/*!< Receiver idle */
#define TXRDY         (0x01 << 2)			/*!< Transmitter ready */
#define TXIDLE        (0x01 << 3)			/*!< Transmitter idle */
#define CTS           (0x01 << 4)			/*!< Status of CTS signal */
#define CTS_DELTA     (0x01 << 5)			/*!< Change in CTS state */
#define TXINT_DIS     (0x01 << 6)			/*!< Transmitter disabled */
#define OVRN_ERR      (0x01 << 8)			/*!< Overrun error */
#define RXBRK         (0x01 << 10)		/*!< Received break */
#define DELTA_RXBRK   (0x01 << 11)		/*!< Change in receive break detection */
#define START_DETECT  (0x01 << 12)		/*!< Start detected */
#define FRM_ERR       (0x01 << 13)		/*!< Frame error */
#define PAR_ERR       (0x01 << 14)		/*!< Parity error */
#define RXNOISE       (0x01 << 15)		/*!< Received noise */

#define UART_002_SYNCEN(n) ((n) == ENABLE ? (1 << 11) : 0)
#define UART_002_ENABLE (1UL)

/**
 * @brief	Initialise the UART
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @return	Nothing
 */
STATIC INLINE void IP_UART_Init(IP_USART_002_T *pUART)
{
	pUART->CFG |= UART_002_ENABLE;
}

/**
 * @brief	Set USART Baud Rate Generator register
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @param	baudrate: Value for Baud Rate Generator register
 * @return	Nothing
 */
STATIC INLINE void IP_UART_SetBaudRate(IP_USART_002_T *pUART, uint32_t baudrate)
{
	pUART->BRG = baudrate;
}

/**
 * @brief	Get the USART Baud Rate Generator register
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @return	Baud Rate Generator register value
 */
STATIC INLINE uint32_t IP_UART_GetBaudRate(IP_USART_002_T *pUART)
{
	return pUART->BRG;
}

/**
 * @brief	Write data to transmit register
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @param	data	: Transmit Data
 * @return	Nothing
 */
STATIC INLINE void IP_UART_Transmit(IP_USART_002_T *pUART, uint8_t data)
{
	pUART->TXDATA = data;
}

/**
 * @brief	Get received UART data
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @return	Received data
 */
STATIC INLINE uint8_t IP_UART_Receive(IP_USART_002_T *pUART)
{
	return (uint8_t) (pUART->RXDATA & 0xFF);
}

/**
 * @brief	Get the interrupt status register
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @return	Interrupt status register
 */
STATIC INLINE uint32_t IP_UART_GetIntStatus(IP_USART_002_T *pUART)
{
	return pUART->INTSTAT;
}

/**
 * @brief	Get the UART status register
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @return	UART status register
 */
STATIC INLINE uint32_t IP_UART_GetStatus(IP_USART_002_T *pUART)
{
	return pUART->STAT;
}

/**
 * @brief	Clear the UART status
 * @param	pUART		: Pointer to selected UARTx peripheral
 * @param	StatusFlag	: status flag to be cleared
 * @return	Nothing
 */
STATIC INLINE void IP_UART_ClearStatus(IP_USART_002_T *pUART, uint32_t StatusFlag)
{
	pUART->STAT |= StatusFlag;
}

/**
 * @brief	Configure the UART protocol
 * @param	pUART	: Pointer to selected UARTx peripheral
 * @param	Databits: Selects the data size for the USART
 * @param	Parity	: Selects what type of parity is used by the USART
 * @param	Stopbits: Selects number of stop bits appended to transmitted data
 * @return	Nothing
 */
void IP_UART_Config(IP_USART_002_T *pUART, IP_UART_002_DATALEN_T Databits,
					IP_UART_002_PARITY_T Parity, IP_UART_002_STOPLEN_T Stopbits);

/**
 * @brief	Enable/Disable UART Interrupts
 * @param	pUART	        : Pointer to selected UARTx peripheral
 * @param	UARTIntCfg		: Specifies the interrupt flag
 * @param	NewState		: New state, ENABLE or DISABLE
 * @return	Nothing
 */
void IP_UART_IntEnable(IP_USART_002_T *pUART, uint32_t UARTIntCfg, FunctionalState NewState);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __USART_002_H_ */
