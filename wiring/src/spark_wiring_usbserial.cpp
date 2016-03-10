/**
 ******************************************************************************
 * @file    spark_wiring_usbserial.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring usb serial module
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

#include "spark_wiring_usbserial.h"

#ifdef SPARK_USB_SERIAL

static uint8_t serial_rx_buffer[USB_RX_BUFFER_SIZE];
static uint8_t serial_tx_buffer[USB_TX_BUFFER_SIZE];

static uint8_t usbserial1_rx_buffer[USB_RX_BUFFER_SIZE];
static uint8_t usbserial1_tx_buffer[USB_TX_BUFFER_SIZE];

#endif
//
// Constructor
//
USBSerial::USBSerial()
{
  _serial = HAL_USB_USART_SERIAL;
  _blocking = true;

  HAL_USB_USART_Config conf;
  conf.rx_buffer = serial_rx_buffer;
  conf.tx_buffer = serial_tx_buffer;
  conf.rx_buffer_size = USB_RX_BUFFER_SIZE;
  conf.tx_buffer_size = USB_TX_BUFFER_SIZE;
  HAL_USB_USART_Init(_serial, &conf);
}

USBSerial::USBSerial(HAL_USB_USART_Serial serial, uint8_t* rx_buffer, uint8_t* tx_buffer)
{
  _serial = serial;
  _blocking = true;

  HAL_USB_USART_Config conf;
  conf.rx_buffer = rx_buffer;
  conf.tx_buffer = tx_buffer;
  conf.rx_buffer_size = USB_RX_BUFFER_SIZE;
  conf.tx_buffer_size = USB_TX_BUFFER_SIZE;
  HAL_USB_USART_Init(_serial, &conf);
}

//
// Public methods
//

void USBSerial::begin(long speed)
{
    HAL_USB_USART_Begin(_serial, speed, NULL);
}

void USBSerial::end()
{
    HAL_USB_USART_End(_serial);
}


// Read data from buffer
int USBSerial::read()
{
	return HAL_USB_USART_Receive_Data(_serial, false);
}

int USBSerial::availableForWrite()
{
  return HAL_USB_USART_Available_Data_For_Write(_serial);
}

int USBSerial::available()
{
	return HAL_USB_USART_Available_Data(_serial);
}

size_t USBSerial::write(uint8_t byte)
{
  if (HAL_USB_USART_Available_Data_For_Write(_serial) > 0 || _blocking) {
    HAL_USB_USART_Send_Data(_serial, byte);
    return 1;
  }
  return 0;
}

void USBSerial::flush()
{
  HAL_USB_USART_Flush_Data(_serial);
}

void USBSerial::blockOnOverrun(bool block)
{
  _blocking = block;
}

int USBSerial::peek()
{
	return HAL_USB_USART_Receive_Data(_serial, true);
}

USBSerial::operator bool() {
  return isEnabled();
}

bool USBSerial::isEnabled() {
  return HAL_USB_USART_Is_Enabled(_serial);
}

unsigned int USBSerial::baud() {
  return HAL_USB_USART_Baud_Rate(_serial);
}

// Preinstantiate Objects //////////////////////////////////////////////////////
#ifdef SPARK_USB_SERIAL

USBSerial& _fetch_usbserial()
{
	static USBSerial _usbserial(HAL_USB_USART_SERIAL, serial_rx_buffer, serial_tx_buffer);
	return _usbserial;
}

USBSerial& _fetch_usbserial1()
{
  static USBSerial _usbserial1(HAL_USB_USART_SERIAL10, usbserial1_rx_buffer, usbserial1_tx_buffer);
  return _usbserial1;
}


#endif
