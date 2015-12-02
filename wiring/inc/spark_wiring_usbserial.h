/**
 ******************************************************************************
 * @file    spark_wiring_usbserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usbserial.c module
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

#ifndef __SPARK_WIRING_USBSERIAL_H
#define __SPARK_WIRING_USBSERIAL_H

#include "spark_wiring_stream.h"
#include "usb_hal.h"

class USBSerial : public Stream
{
public:
	// public methods
	USBSerial();

        unsigned int baud() { return USB_USART_Baud_Rate(); }

        operator bool() { return baud()!=0; }

	void begin(long speed);
	void end();
	int peek();

	virtual size_t write(uint8_t byte);
	virtual int read();
	virtual int available();
	virtual void flush();

	using Print::write;
};

extern USBSerial Serial;

#endif
