#include <string.h>
#include "hw_config.h"
#include "cc3000_common.h"
#include "hci.h"
#include "wlan.h"

#define FALSE					0x00
#define TRUE					!FALSE

#define READ					0x03
#define WRITE					0x01

#define HI(value)				(((value) & 0xFF00) >> 8)
#define LO(value)				((value) & 0x00FF)

#define ASSERT_CS()				CC3000_CS_LOW()
#define DEASSERT_CS()			CC3000_CS_HIGH()

/*It takes roughly 4 instruction to perform a delay*/
#define FIFTY_US_DELAY			(((SystemCoreClock/1000000)*50)/4) // (24*50)/4 for 24 MHz clk
#define DMA_WINDOW_SIZE         1024
#define SPI_WINDOW_SIZE         DMA_WINDOW_SIZE

#define HEADERS_SIZE_EVNT       (SPI_HEADER_SIZE + 5)

typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

/* CC3000 SPI Protocol API */
extern void SpiOpen(gcSpiHandleRx pfRxHandler);
extern void SpiClose(void);
extern long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);
extern void SpiResumeSpi(void);
extern void SPI_DMA_IntHandler(void);
extern void SPI_EXTI_IntHandler(void);
