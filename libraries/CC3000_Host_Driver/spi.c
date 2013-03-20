#include "spi.h"

unsigned char wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

#define 	eSPI_STATE_POWERUP 				 (0)
#define 	eSPI_STATE_INITIALIZED  		 (1)
#define 	eSPI_STATE_IDLE					 (2)
#define 	eSPI_STATE_WRITE_IRQ	   		 (3)
#define 	eSPI_STATE_WRITE_FIRST_PORTION   (4)
#define 	eSPI_STATE_WRITE_EOT			 (5)
#define 	eSPI_STATE_READ_IRQ				 (6)
#define 	eSPI_STATE_READ_FIRST_PORTION	 (7)
#define 	eSPI_STATE_READ_EOT				 (8)

typedef struct {
	gcSpiHandleRx SPIRxHandler;
	unsigned short usTxPacketLength;
	unsigned short usRxPacketLength;
	volatile unsigned long ulSpiState;
	unsigned char *pTxPacket;
	unsigned char *pRxPacket;
} tSpiInformation;

tSpiInformation sSpiInformation;

// The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
// for the purpose of detection of the overrun. The location of the memory where the magic number
// resides shall never be written. In case it is written - the overrun occured and either recevie function
// or send function will stuck forever.
#define CC3000_BUFFER_MAGIC_NUMBER (0xDE)

// Static buffer for 5 bytes of SPI HEADER
unsigned char tSpiReadHeader[] = { READ, 0, 0, 0, 0 };

/****************************************************************************
 CC3000 SPI Protocol API
 ****************************************************************************/
void SpiOpen(gcSpiHandleRx pfRxHandler) {
	sSpiInformation.ulSpiState = eSPI_STATE_POWERUP;

	sSpiInformation.SPIRxHandler = pfRxHandler;
	sSpiInformation.pRxPacket = wlan_rx_buffer;
	sSpiInformation.usRxPacketLength = 0;
	sSpiInformation.pTxPacket = NULL;
	sSpiInformation.usTxPacketLength = 0;

	/* Enable Interrupt */
	tSLInformation.WlanInterruptEnable();
}

void SpiClose(void) {
	if (sSpiInformation.pRxPacket) {
		sSpiInformation.pRxPacket = 0;
	}

	/* Disable Interrupt */
	tSLInformation.WlanInterruptDisable();
}

/**
 * @brief  Writes data over SPI  transport link to CC3000
 * @param  pUserBuffer: pointer to data
 * @param usLength: length of data that will be sent to CC3000
 * @retval None
 */
long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength) {
	unsigned char ucPad = 0;

	//
	// Figure out the total length of the packet in order to figure out if there is padding or not
	//
	if (!(usLength & 0x0001)) {
		ucPad++;
	}

	pUserBuffer[0] = WRITE;
	pUserBuffer[1] = HI(usLength + ucPad);
	pUserBuffer[2] = LO(usLength + ucPad);
	pUserBuffer[3] = 0;
	pUserBuffer[4] = 0;

	usLength += (SPI_HEADER_SIZE + ucPad);

	// The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
	// for the purpose of detection of the overrun. If the magic number is overriten - buffer overrun
	// occurred - and we will stuck here forever!
	if (wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] != CC3000_BUFFER_MAGIC_NUMBER) {
		while (1)
			;
	}

	if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP) {
		while (sSpiInformation.ulSpiState != eSPI_STATE_INITIALIZED)
			;
	}

	if (sSpiInformation.ulSpiState == eSPI_STATE_INITIALIZED) {
		//
		// This is time for first TX/RX transactions over SPI: the IRQ is down - so need to send read buffer size command
		//
		ASSERT_CS();

		//Insert Delay
		//Delay(1);	//1ms delay

		//SPI writes first 4 bytes of data using DMA
		CC3000_DMA_Config(CC3000_DMA_TX, pUserBuffer, 4);
		/* Enable TX DMA channel */
		DMA_Cmd(CC3000_SPI_TX_DMA_CHANNEL, ENABLE);
		/* Wait for DMA Transfer complete */
		while (!DMA_GetFlagStatus(CC3000_SPI_TX_DMA_TCFLAG ))
			;

		//SPI writes remaining bytes of data using DMA
		CC3000_DMA_Config(CC3000_DMA_TX, pUserBuffer + 4, usLength - 4);
		/* Enable TX DMA channel */
		DMA_Cmd(CC3000_SPI_TX_DMA_CHANNEL, ENABLE);
		/* Wait for DMA Transfer complete */
		while (!DMA_GetFlagStatus(CC3000_SPI_TX_DMA_TCFLAG ))
			;

		// From this point on - operate in a regular way
		sSpiInformation.ulSpiState = eSPI_STATE_IDLE;

		DEASSERT_CS();
	} else {
		//
		// We need to prevent here race that can occur in case 2 back to back packets are sent to the
		// device, so the state will move to IDLE and once again to not IDLE due to IRQ
		//

		while (sSpiInformation.ulSpiState != eSPI_STATE_IDLE) {
			;
		}

		sSpiInformation.ulSpiState = eSPI_STATE_WRITE_IRQ;
		sSpiInformation.pTxPacket = pUserBuffer;
		sSpiInformation.usTxPacketLength = usLength;

		//
		// Assert the CS line and wait till SSI IRQ line is active and then initialize write operation
		//
		ASSERT_CS();
	}
	//
	// Due to the fact that we are currently implementing a blocking situation
	// here we will wait till end of transaction
	//
	while (eSPI_STATE_IDLE != sSpiInformation.ulSpiState) {
		;
	}
	return (0);
}

void SpiResumeSpi(void) {
	//To Do
}

/**
 * @brief  The handler for Interrupt that is generated when CC3000 brings the
 IRQ line low.
 * @param  None
 * @retval None
 */
void SPIIntHandler(void) {
	//Interrupt pending bit is cleared in the ISR handler
	if (!tSLInformation.ReadWlanInterruptPin()) {
		if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP) {
			/* This means IRQ line was low call a callback of HCI Layer to inform on event */
			sSpiInformation.ulSpiState = eSPI_STATE_INITIALIZED;
		} else if (sSpiInformation.ulSpiState == eSPI_STATE_IDLE) {
			sSpiInformation.ulSpiState = eSPI_STATE_READ_IRQ;

			/* IRQ line goes down - we start reception */ASSERT_CS();

			//SPI Read Header - To Do
			//Configure DMA buffer to read rest of the data for processing - To Do

			sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;

			DEASSERT_CS();

			sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
			sSpiInformation.SPIRxHandler(
					sSpiInformation.pRxPacket + SPI_HEADER_SIZE);
		} else if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_IRQ) {
			//Send the data over SPI using DMA
			CC3000_DMA_Config(CC3000_DMA_TX, sSpiInformation.pTxPacket,
					sSpiInformation.usTxPacketLength);
			/* Enable TX DMA channel */
			DMA_Cmd(CC3000_SPI_TX_DMA_CHANNEL, ENABLE);
			/* Wait for DMA Transfer complete */
			while (!DMA_GetFlagStatus(CC3000_SPI_TX_DMA_TCFLAG ))
				;

			sSpiInformation.ulSpiState = eSPI_STATE_IDLE;

			DEASSERT_CS();
		}
	}
}
