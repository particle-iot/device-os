/**
 ******************************************************************************
 * @file    spark_wiring_hciserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_hciserial.c module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

#ifndef __SPARK_WIRING_HCISERIAL_H
#define __SPARK_WIRING_HCISERIAL_H

#if PLATFORM_ID == 88 // Duo

#include "spark_wiring_stream.h"
#include "hci_usart_hal.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_usartserial.h"


class HCIUsart
{
private:
    HAL_HCI_USART_Serial _serial;
public:
    HCIUsart(HAL_HCI_USART_Serial serial, HCI_USART_Ring_Buffer *rx_buffer, HCI_USART_Ring_Buffer *tx_buffer);

    void registerReceiveHandler(ReceiveHandler_t handler);
    int  downloadFirmware(void);

    void begin(unsigned long baudrate);
    void end(void);

    int available(void);
    int read(void);
    size_t write(uint8_t dat);
    size_t write(const uint8_t *buf, uint16_t len);

    void restartTransmit(void);
    void setRTS(uint8_t value);
    uint8_t checkCTS(void);
};

#if Wiring_Serial6
extern HCIUsart BTUsart;
#endif

#endif

#endif
