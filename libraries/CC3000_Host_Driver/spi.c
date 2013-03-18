#include "spi.h"

unsigned char wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

enum SPI_STATE {
	SPI_STATE_POWERUP,
	SPI_STATE_INITIALIZED,
	SPI_STATE_WRITE_IRQ,
	SPI_STATE_WRITE_FIRST_PORTION,
	SPI_STATE_WRITE_EOT,
	SPI_STATE_IDLE,
	SPI_STATE_READ_IRQ,
	SPI_STATE_READ_FIRST_PORTION,
	SPI_STATE_READ_EOT
};

struct SpiInfo {
	enum SPI_STATE state;
	gcSpiHandleRx rx_handler;
	unsigned char *rx_packet;
	unsigned short rx_packet_length;
	unsigned char *tx_packet;
	unsigned short tx_packet_length;
} spi_info;

/****************************************************************************
 CC3000 SPI Protocol API
 ****************************************************************************/

void SpiOpen(gcSpiHandleRx rx_handler) {
	spi_info.state = SPI_STATE_POWERUP;
	spi_info.rx_handler = rx_handler;
	spi_info.rx_packet = wlan_rx_buffer;
	spi_info.rx_packet_length = 0;
	spi_info.tx_packet = NULL;
	spi_info.tx_packet_length = 0;

	tSLInformation.WlanInterruptEnable();
}

void SpiClose(void) {
	if (spi_info.rx_packet) {
		spi_info.rx_packet = 0;
	}

	tSLInformation.WlanInterruptDisable();
}

long SpiWrite(unsigned char *user_buffer, unsigned short length) {
	//To Do
}

void SpiResumeSpi(void) {
	//To Do
}
