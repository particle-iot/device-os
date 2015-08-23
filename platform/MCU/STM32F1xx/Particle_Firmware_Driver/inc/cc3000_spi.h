/**
  ******************************************************************************
  * @file    cc3000_spi.h
  * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
  * @version V1.0.0
  * @date    29-March-2013
  * @brief   This file contains all the functions prototypes for the
  *          CC3000 SPI firmware driver.
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CC3000_SPI_H
#define __CC3000_SPI_H

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "hw_config.h"
#include "cc3000_common.h"
#include "hci.h"
#include "wlan.h"

#ifndef FALSE
#define FALSE					0x00
#endif
#ifndef TRUE
#define TRUE					!FALSE
#endif

#define READ					0x03
#define WRITE					0x01

#define HI(value)				(((value) & 0xFF00) >> 8)
#define LO(value)				((value) & 0x00FF)

#define ASSERT_CS()				CC3000_CS_LOW()
#define DEASSERT_CS()			        CC3000_CS_HIGH()


#define HEADERS_SIZE_EVNT       (SPI_HEADER_SIZE + 5)
#define MAX_PACKET_PAYLOAD_SIZE 1014
#define RX_SPI_BUFFER_SIZE      (MAX_PACKET_PAYLOAD_SIZE+HEADERS_SIZE_EVNT)
#define TX_SPI_BUFFER_SIZE      (MAX_PACKET_PAYLOAD_SIZE+HEADERS_SIZE_EVNT)

typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);

extern unsigned char wlan_rx_buffer[RX_SPI_BUFFER_SIZE];
extern unsigned char wlan_tx_buffer[TX_SPI_BUFFER_SIZE];

/* CC3000 SPI Protocol API */
extern void SpiOpen(gcSpiHandleRx pfRxHandler);
extern void SpiClose(void);
extern long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);
extern void SpiResumeSpi(void);
extern void SPI_DMA_IntHandler(void);
extern void SPI_EXTI_IntHandler(void);

extern void handle_spi_request();

#endif /* __CC3000_SPI_H */
