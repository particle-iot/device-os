#include <string.h>
#include "hw_config.h"
#include "cc3000_common.h"
#include "hci.h"
#include "wlan.h"

#define READ					3
#define WRITE					1

#define HI(value)				(((value) & 0xFF00) >> 8)
#define LO(value)				((value) & 0x00FF)

#define ASSERT_CS()				CC3000_CS_LOW()
#define DEASSERT_CS()			CC3000_CS_HIGH()

typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

/* CC3000 SPI Protocol API */
extern void SpiOpen(gcSpiHandleRx pfRxHandler);
extern void SpiClose(void);
extern long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);
extern void SpiResumeSpi(void);
extern void SPIIntHandler(void);
