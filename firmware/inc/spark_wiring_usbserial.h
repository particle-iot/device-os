/**
 ******************************************************************************
 * @file    spark_wiring_usbserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usbserial.c module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
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

#ifndef __SPARK_WIRING_USBSERIAL_H
#define __SPARK_WIRING_USBSERIAL_H

#include "spark_wiring_stream.h"

class USBSerial : public Stream
{
private:

public:
	// public methods
	USBSerial();

	void begin(long speed);
	void end();
	int peek();

	virtual size_t write(uint8_t byte);
	virtual int read();
	virtual int available();
	virtual void flush();

	using Print::write;
};

extern void USB_USART_Init(uint32_t baudRate);
extern uint8_t USB_USART_Available_Data(void);
extern int32_t USB_USART_Receive_Data(void);
extern void USB_USART_Send_Data(uint8_t Data);

extern USBSerial Serial;

#endif
