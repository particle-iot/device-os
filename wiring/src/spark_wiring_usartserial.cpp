/**
 ******************************************************************************
 * @file    spark_wiring_usartserial.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring hardware serial module
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

#include "spark_wiring_usartserial.h"
#include "spark_wiring_constants.h"

// Constructors ////////////////////////////////////////////////////////////////

USARTSerial::USARTSerial(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
  _serial = serial;
  // Default is blocking mode
  _blocking = true;
  HAL_USART_Init(serial, rx_buffer, tx_buffer);
}
// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
  begin(baud, SERIAL_8N1);
}

void USARTSerial::begin(unsigned long baud, uint32_t config)
{
  HAL_USART_BeginConfig(_serial, baud, config, 0);
}

void USARTSerial::end()
{
  HAL_USART_End(_serial);
}

void USARTSerial::halfduplex(bool Enable)
{
    HAL_USART_Half_Duplex(_serial, Enable);
}

void USARTSerial::blockOnOverrun(bool block)
{
  _blocking = block;
}


int USARTSerial::availableForWrite(void)
{
  return HAL_USART_Available_Data_For_Write(_serial);
}

int USARTSerial::available(void)
{
  return HAL_USART_Available_Data(_serial);
}

int USARTSerial::peek(void)
{
  return HAL_USART_Peek_Data(_serial);
}

int USARTSerial::read(void)
{
  return HAL_USART_Read_Data(_serial);
}

void USARTSerial::flush()
{
  HAL_USART_Flush_Data(_serial);
}

size_t USARTSerial::write(uint8_t c)
{
  // attempt a write if blocking, or for non-blocking if there is room.
  if (_blocking || HAL_USART_Available_Data_For_Write(_serial) > 0) {
    // the HAL always blocks.
	  return HAL_USART_Write_Data(_serial, c);
  }
  return 0;
}

size_t USARTSerial::write(uint16_t c)
{
  return HAL_USART_Write_NineBitData(_serial, c);
}

USARTSerial::operator bool() {
  return true;
}

bool USARTSerial::isEnabled() {
  return HAL_USART_Is_Enabled(_serial);
}

void USARTSerial::breakTx() {
  HAL_USART_Send_Break(_serial, NULL);
}

bool USARTSerial::breakRx() {
  return (bool)HAL_USART_Break_Detected(_serial);
}

#ifndef SPARK_WIRING_NO_USART_SERIAL
// Preinstantiate Objects //////////////////////////////////////////////////////
static Ring_Buffer serial1_rx_buffer;
static Ring_Buffer serial1_tx_buffer;

USARTSerial& __fetch_global_Serial1()
{
	static USARTSerial serial1(HAL_USART_SERIAL1, &serial1_rx_buffer, &serial1_tx_buffer);
	return serial1;
}

// optional Serial2 is instantiated from libraries/Serial2/Serial2.h
#endif
