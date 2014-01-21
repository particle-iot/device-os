/**
  ******************************************************************************
  * @file    cc3000_spi.c
  * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
  * @version V1.0.0
  * @date    29-March-2013
  * @brief   This file contains all the functions prototypes for the
  *          CC3000 SPI firmware driver.
  ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

#include "cc3000_spi.h"

unsigned char wlan_rx_buffer[SPI_BUFFER_SIZE];	//CC3000_RX_BUFFER_SIZE
unsigned char wlan_tx_buffer[SPI_BUFFER_SIZE];	//CC3000_TX_BUFFER_SIZE

#define eSPI_STATE_POWERUP				(0)
#define eSPI_STATE_INITIALIZED			(1)
#define eSPI_STATE_IDLE					(2)
#define eSPI_STATE_WRITE_IRQ			(3)
#define eSPI_STATE_WRITE_FIRST_PORTION	(4)
#define eSPI_STATE_WRITE_EOT			(5)
#define eSPI_STATE_READ_IRQ				(6)
#define eSPI_STATE_READ_FIRST_PORTION	(7)
#define eSPI_STATE_READ_EOT				(8)

typedef struct
{
	gcSpiHandleRx SPIRxHandler;
	unsigned short usTxPacketLength;
	unsigned short usRxPacketLength;
	volatile unsigned long ulSpiState;
	unsigned char *pTxPacket;
	unsigned char *pRxPacket;
} tSpiInformation;

tSpiInformation sSpiInformation;

// Static buffer for 5 bytes of SPI HEADER
unsigned char tSpiReadHeader[] = { READ, 0, 0, 0, 0 };

void SpiPauseSpi(void);
void SpiReadData(unsigned char *data, unsigned short size);
void SpiTriggerRxProcessing(void);
void SpiWriteAsync(const unsigned char *data, unsigned short size);
void SpiReadWriteStringInt(uint32_t ulTrueFalse, const uint8_t *ptrData, uint32_t ulDataSize);
void SpiReadWriteString(uint32_t ulTrueFalse, const uint8_t *ptrData, uint32_t ulDataSize);
long SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength);
long SpiReadDataCont(void);
void SpiReadHeader(void);
void SpiContReadOperation(void);

/****************************************************************************
 CC3000 SPI Protocol API
 ****************************************************************************/
void SpiOpen(gcSpiHandleRx pfRxHandler)
{
	sSpiInformation.ulSpiState = eSPI_STATE_POWERUP;

	sSpiInformation.SPIRxHandler = pfRxHandler;
	sSpiInformation.pRxPacket = wlan_rx_buffer;
	sSpiInformation.usRxPacketLength = 0;
	sSpiInformation.pTxPacket = NULL;
	sSpiInformation.usTxPacketLength = 0;

	/* Enable Interrupt */
	tSLInformation.WlanInterruptEnable();
}

void SpiClose(void)
{
	if (sSpiInformation.pRxPacket)
	{
		sSpiInformation.pRxPacket = 0;
	}

	/* Disable Interrupt */
	tSLInformation.WlanInterruptDisable();
}

void SpiResumeSpi(void)
{
	//
	//Enable CC3000 SPI IRQ Line Interrupt
	//
	NVIC_EnableIRQ(CC3000_WIFI_INT_EXTI_IRQn);
}

void SpiPauseSpi(void)
{
	//
	//Disable CC3000 SPI IRQ Line Interrupt
	//
	NVIC_DisableIRQ(CC3000_WIFI_INT_EXTI_IRQn);
}

/**
 * @brief  This indicate the end of a receive and calls a registered handler
 to process the received data
 * @param  None
 * @retval None
 */
void SpiTriggerRxProcessing(void)
{
	SpiPauseSpi();

	//
	// Trigger Rx processing
	//
	DEASSERT_CS();
	sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
	sSpiInformation.SPIRxHandler(sSpiInformation.pRxPacket + SPI_HEADER_SIZE);
}

/**
 * @brief  Sends data on SPI to generate interrupt on reception
 * @param  The pointer to data buffer
 * @param  This size of data
 * @retval None
 */
void SpiReadData(unsigned char *data, unsigned short size)
{
	SpiReadWriteStringInt(TRUE, data, size);
}

/**
 * @brief  This sends data over the SPI transport layer with
 * @param  None
 * @retval None
 */
void SpiWriteAsync(const unsigned char *data, unsigned short size)
{
	//
	// The DMA TX/RX channel must be disabled.
	//
	SpiReadWriteString(FALSE, data, size);
}

/**
 * @brief  This function TX and RX SPI data and configures interrupt generation
 * 		at the end of the TX interrupt
 * @param  ulTrueFlase True for a read or False for write
 * @param  ptrData Pointer to data to be written
 * @param  ulDataSize The size of the data to be written or read
 * @retval None
 */
void SpiReadWriteStringInt(uint32_t ulTrueFalse, const uint8_t *ptrData, uint32_t ulDataSize)
{
	/* Disable DMA Channels */
	CC3000_SPI_DMA_Channels(DISABLE);

	if (ulTrueFalse == TRUE)
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) ptrData, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) tSpiReadHeader, ulDataSize);
	}
	else
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) sSpiInformation.pRxPacket, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) ptrData, ulDataSize);
	}

	/* Enable DMA SPI Interrupt */
	DMA_ITConfig(CC3000_SPI_TX_DMA_CHANNEL, DMA_IT_TC, ENABLE);

	/* Enable DMA Channels */
	CC3000_SPI_DMA_Channels(ENABLE);

	/* Wait until DMA Transfer Completes */
	while(DMA_GetCurrDataCounter(CC3000_SPI_TX_DMA_CHANNEL))
	{
	}

	while(DMA_GetCurrDataCounter(CC3000_SPI_RX_DMA_CHANNEL))
	{
	}

	/* Loop until SPI busy */
	while (SPI_I2S_GetFlagStatus(CC3000_SPI, SPI_I2S_FLAG_BSY ) != RESET)
	{
	}
}

/**
 * @brief  This function TX and RX SPI data with no interrupt at end of SPI TX
 * @param  ulTrueFlase True for a read or False for write
 * @param  ptrData Pointer to data to be written
 * @param  ulDataSize The size of the data to be written or read
 * @retval None
 */
void SpiReadWriteString(uint32_t ulTrueFalse, const uint8_t *ptrData, uint32_t ulDataSize)
{
	/* Disable DMA RX Channels */
	CC3000_SPI_DMA_Channels(DISABLE);

	/* Specify the DMA Read/Write buffer and size */
	if (ulTrueFalse == TRUE)
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) ptrData, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) tSpiReadHeader, ulDataSize);
	}
	else
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) sSpiInformation.pRxPacket, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) ptrData, ulDataSize);
	}

	/* Enable DMA Channels */
	CC3000_SPI_DMA_Channels(ENABLE);

	/* Wait until DMA Transfer Completes */
	while(DMA_GetCurrDataCounter(CC3000_SPI_TX_DMA_CHANNEL))
	{
	}

	while(DMA_GetCurrDataCounter(CC3000_SPI_RX_DMA_CHANNEL))
	{
	}

	/* Loop until SPI busy */
	while (SPI_I2S_GetFlagStatus(CC3000_SPI, SPI_I2S_FLAG_BSY ) != RESET)
	{
	}
}

/**
 * @brief  Sends header information to CC3000
 * @param  None
 * @retval None
 */
long SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength)
{
	//
	// workaround for first transaction
	//
	ASSERT_CS();

	//Delay for at least 50 us
	Delay_Microsecond(50);

	//SPI writes first 4 bytes of data
	SpiReadWriteString(FALSE, ucBuf, 4);

	//Delay for at least 50 us
	Delay_Microsecond(50);

	//SPI writes next 4 bytes of data
	SpiReadWriteString(FALSE, ucBuf + 4, usLength - 4);

	// From this point on - operate in a regular way
	sSpiInformation.ulSpiState = eSPI_STATE_IDLE;

	DEASSERT_CS();
	return (0);
}

/**
 * @brief  Writes data over SPI  transport link to CC3000
 * @param  pUserBuffer: pointer to data
 * @param usLength: length of data that will be sent to CC3000
 * @retval None
 */
long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength)
{
	unsigned char ucPad = 0;
	//
	// Figure out the total length of the packet in order to figure out if there is padding or not
	//
	if (!(usLength & 0x0001))
	{
		ucPad++;
	}

	pUserBuffer[0] = WRITE;
	pUserBuffer[1] = HI(usLength + ucPad);
	pUserBuffer[2] = LO(usLength + ucPad);
	pUserBuffer[3] = 0;
	pUserBuffer[4] = 0;

	usLength += (SPI_HEADER_SIZE + ucPad);

	if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP)
	{
		while (sSpiInformation.ulSpiState != eSPI_STATE_INITIALIZED)
		{
		}
	}
	if (sSpiInformation.ulSpiState == eSPI_STATE_INITIALIZED)
	{

		//
		// This is time for first TX/RX transactions over SPI: the IRQ is down - so need to send read buffer size command
		//
		SpiFirstWrite(pUserBuffer, usLength);

		//
		// Due to the fact that we are currently implementing a blocking situation
		// here we will wait till end of transaction
		//
		while (eSPI_STATE_IDLE != sSpiInformation.ulSpiState)
		{
		}
	}
	else
	{

		//
		// We need to prevent here race that can occur in case 2 back to back packets are sent to the
		// device, so the state will move to IDLE and once again to not IDLE due to IRQ
		//
		while (sSpiInformation.ulSpiState != eSPI_STATE_IDLE)
		{
		}

		/* Loop until SPI busy */
		while (SPI_I2S_GetFlagStatus(CC3000_SPI, SPI_I2S_FLAG_BSY ) != RESET)
		{
		}

		while (!tSLInformation.ReadWlanInterruptPin())
		{
		}

		sSpiInformation.ulSpiState = eSPI_STATE_WRITE_IRQ;
		sSpiInformation.pTxPacket = pUserBuffer;
		sSpiInformation.usTxPacketLength = usLength;

		//
		// Assert the CS line and wait till IRQ line is active and then initialize write operation
		//
		ASSERT_CS();

		while (!tSLInformation.ReadWlanInterruptPin())
		{
		}

		//
		// Due to the fact that we are currently implementing a blocking situation
		// here we will wait till end of transaction
		//
		while (eSPI_STATE_IDLE != sSpiInformation.ulSpiState)
		{
		}
	}
	return (0);
}

/**
 * @brief  This function processes received SPI Header and in accordance with it
 - continues reading the packet

 * @param  None
 * @retval None
 */
long SpiReadDataCont(void)
{
	long data_to_recv;

	unsigned char *evnt_buff, type;

	//
	//determine what type of packet we have
	//
	evnt_buff = sSpiInformation.pRxPacket;
	data_to_recv = 0;
	STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_PACKET_TYPE_OFFSET, type);

	switch (type)
	{
		case HCI_TYPE_DATA:
		{
			STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_DATA_LENGTH_OFFSET, data_to_recv);

			if (data_to_recv >= SPI_WINDOW_SIZE)
			{
				data_to_recv = eSPI_STATE_READ_FIRST_PORTION;
				SpiReadData(evnt_buff + 10, SPI_WINDOW_SIZE);
				sSpiInformation.ulSpiState = eSPI_STATE_READ_FIRST_PORTION;
			}
			else
			{
				//
				// We need to read the rest of data..
				//
				if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
				{
					data_to_recv++;
				}

				if (data_to_recv)
				{
					SpiReadData(evnt_buff + 10, data_to_recv);
				}

				sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;
			}
			break;
		}
		case HCI_TYPE_EVNT:
		{
			//
			// Calculate the rest length of the data
			//
			STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_EVENT_LENGTH_OFFSET, data_to_recv);
			data_to_recv -= 1;

			//
			// Add padding byte if needed
			//
			if ((HEADERS_SIZE_EVNT + data_to_recv) & 1)
			{

				data_to_recv++;
			}

			if (data_to_recv)
			{
				SpiReadData(evnt_buff + 10, data_to_recv);
			}

			sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;
			break;
		}
	}

	return (data_to_recv);
}

/**
 * @brief  This function enter point for read flow: first we read minimal
 5 SPI header bytes and 5 Event Data bytes

 * @param  None
 * @retval None
 */
void SpiReadHeader(void)
{
	SpiReadWriteStringInt(TRUE, sSpiInformation.pRxPacket, 10);
}

/**
 * @brief  Determine if all data was read if so end the data exchange
 * @param  None
 * @retval None
 */
void SpiContReadOperation(void)
{
	//
	// The header was read - continue with  the payload read
	//
	if (!SpiReadDataCont())
	{
		//
		// All the data was read - finalize handling by switching to teh task
		//	and calling from task Event Handler
		//
		SpiTriggerRxProcessing();
	}
}

/**
 * @brief  The handler for Interrupt that is generated on SPI at the end of DMA
 transfer.
 * @param  None
 * @retval None
 */
void SPI_DMA_IntHandler(void)
{
	unsigned long ucTxFinished, ucRxFinished;
	unsigned short data_to_recv;
	unsigned char *evnt_buff;

	evnt_buff = sSpiInformation.pRxPacket;
	data_to_recv = 0;

	ucTxFinished = DMA_GetFlagStatus(CC3000_SPI_TX_DMA_TCFLAG );
	ucRxFinished = DMA_GetFlagStatus(CC3000_SPI_RX_DMA_TCFLAG );

	if (sSpiInformation.ulSpiState == eSPI_STATE_READ_IRQ)
	{
		//
		// If one of DMA's still did not finished its operation - we need to stay
		// and wait till it will finish
		//
		if (ucTxFinished && ucRxFinished)
		{
			/* Clear SPI_DMA Interrupt Pending Flags */
			DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

			SpiContReadOperation();
		}
	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_READ_FIRST_PORTION)
	{
		if (ucRxFinished)
		{
			/* Clear SPI_DMA Interrupt Pending Flags */
			DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

			STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_DATA_LENGTH_OFFSET, data_to_recv);

			//
			// Read the last portion of data
			//
			//
			// We need to read the rest of data..
			//
			data_to_recv -= SPI_WINDOW_SIZE;

			if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
			{
				data_to_recv++;
			}

			SpiReadData(sSpiInformation.pRxPacket + 10 + SPI_WINDOW_SIZE, data_to_recv);

			sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;
		}
	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_READ_EOT)
	{
		//
		// All the data was read - finalize handling by switching to the task
		// and calling from task Event Handler
		//
		if (ucRxFinished)
		{
			/* Clear SPI_DMA Interrupt Pending Flags */
			DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

			SpiTriggerRxProcessing();
		}
	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_EOT)
	{
		if (ucTxFinished)
		{
			/* Loop until SPI busy */
			while (SPI_I2S_GetFlagStatus(CC3000_SPI, SPI_I2S_FLAG_BSY ) != RESET)
			{
			}

			/* Clear SPI_DMA Interrupt Pending Flags */
			DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

			DEASSERT_CS();

			sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
		}
	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_FIRST_PORTION)
	{
		if (ucTxFinished)
		{
			sSpiInformation.ulSpiState = eSPI_STATE_WRITE_EOT;
			SpiWriteAsync(sSpiInformation.pTxPacket + SPI_WINDOW_SIZE, sSpiInformation.usTxPacketLength - SPI_WINDOW_SIZE);
		}
	}
}

/**
 * @brief  The handler for Interrupt that is generated when CC3000 brings the
 IRQ line low.
 * @param  None
 * @retval None
 */
void SPI_EXTI_IntHandler(void)
{
	//Flag is cleared in first ISR handler
	if (!tSLInformation.ReadWlanInterruptPin())
	{
		if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP)
		{
			/* This means IRQ line was low call a callback of HCI Layer to inform on event */
			sSpiInformation.ulSpiState = eSPI_STATE_INITIALIZED;
		}
		else if (sSpiInformation.ulSpiState == eSPI_STATE_IDLE)
		{
			sSpiInformation.ulSpiState = eSPI_STATE_READ_IRQ;

			/* IRQ line goes down - we are starting reception */

			ASSERT_CS();

			//
			// Wait for TX/RX Complete which will come as DMA interrupt
			//
			SpiReadHeader();
		}
		else if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_IRQ)
		{
			if (sSpiInformation.usTxPacketLength <= SPI_WINDOW_SIZE)
			{
				//
				// Send the data over SPI and wait for complete interrupt
				//
				sSpiInformation.ulSpiState = eSPI_STATE_WRITE_EOT;

				SpiReadWriteStringInt(FALSE, sSpiInformation.pTxPacket, sSpiInformation.usTxPacketLength);

			}
			else
			{
				//
				// Send the data over SPI and wait for complete interrupt to transfer the rest
				//
				sSpiInformation.ulSpiState = eSPI_STATE_WRITE_FIRST_PORTION;

				//
				// Start the DMA and change state
				//
				SpiWriteAsync(sSpiInformation.pTxPacket, SPI_WINDOW_SIZE);
			}
		}
	}
}

