#include <string.h>
#include "cc3000_common.h"
#include "hci.h"
#include "wlan.h"

#define OPCODE_WRITE          0x01
#define OPCODE_READ           0x03
#define HI_BYTE(value)        (((value) & 0xFF00) >> 8)
#define LO_BYTE(value)        ((value) & 0x00FF) 
#define PROTOCOL_HEADER_SIZE  5
#define ASSERT_CS()           //To Do
#define DEASSERT_CS()         //To Do
typedef void (*gcSpiHandleRx)(void *p);

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

/* CC3000 SPI Protocol API */
extern void SpiOpen(gcSpiHandleRx rx_handler);
extern void SpiClose(void);
extern long SpiWrite(unsigned char *user_buffer, unsigned short length);
extern void SpiResumeSpi(void);
