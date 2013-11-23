/**
 ******************************************************************************
 * @file    spark_wiring_usbserial.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring usb serial module
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

#include "spark_wiring_usbserial.h"

//
// Constructor
//
USBSerial::USBSerial()
{
}

//
// Public methods
//

void USBSerial::begin(long speed)
{
	USB_USART_Init(speed);
}

void USBSerial::end()
{
	//To Do
}


// Read data from buffer
int USBSerial::read()
{
	return USB_USART_Receive_Data();
}

int USBSerial::available()
{
	return USB_USART_Available_Data();
}

size_t USBSerial::write(uint8_t byte)
{
	USB_USART_Send_Data(byte);

	return 1;
}

void USBSerial::flush()
{
	//To Do
}

int USBSerial::peek()
{
	return -1;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

USBSerial Serial;
