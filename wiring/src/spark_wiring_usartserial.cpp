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
#include "module_info.h"
#include <algorithm>

// Constructors ////////////////////////////////////////////////////////////////

USARTSerial::USARTSerial(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
  _serial = serial;
  // Default is blocking mode
  _blocking = true;
  hal_usart_init(serial, rx_buffer, tx_buffer);
}
// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
  begin(baud, SERIAL_8N1);
}

void USARTSerial::begin(unsigned long baud, uint32_t config)
{
  hal_usart_begin_config(_serial, baud, config, nullptr);
}

void USARTSerial::end()
{
  hal_usart_end(_serial);
}

void USARTSerial::halfduplex(bool Enable)
{
    hal_usart_half_duplex(_serial, Enable);
}

void USARTSerial::blockOnOverrun(bool block)
{
  _blocking = block;
}


int USARTSerial::availableForWrite(void)
{
  return std::max(0, (int)hal_usart_available_data_for_write(_serial));
}

int USARTSerial::available(void)
{
  return std::max(0, (int)hal_usart_available(_serial));
}

int USARTSerial::peek(void)
{
  return std::max(-1, (int)hal_usart_peek(_serial));
}

int USARTSerial::read(void)
{
  return std::max(-1, (int)hal_usart_read(_serial));
}

void USARTSerial::flush()
{
  hal_usart_flush(_serial);
}

size_t USARTSerial::write(uint8_t c)
{
  // attempt a write if blocking, or for non-blocking if there is room.
  if (_blocking || hal_usart_available_data_for_write(_serial) > 0) {
    // the HAL always blocks.
	  return hal_usart_write(_serial, c);
  }
  return 0;
}

size_t USARTSerial::write(uint16_t c)
{
  return hal_usart_write_nine_bits(_serial, c);
}

USARTSerial::operator bool() {
  return true;
}

bool USARTSerial::isEnabled() {
  return hal_usart_is_enabled(_serial);
}

void USARTSerial::breakTx() {
  hal_usart_send_break(_serial, nullptr);
}

bool USARTSerial::breakRx() {
  return (bool)hal_usart_break_detected(_serial);
}

#ifndef SPARK_WIRING_NO_USART_SERIAL
// Preinstantiate Objects //////////////////////////////////////////////////////
#if ((MODULE_FUNCTION == MOD_FUNC_USER_PART) || (MODULE_FUNCTION == MOD_FUNC_MONO_FIRMWARE))
static Ring_Buffer serial1_rx_buffer;
static Ring_Buffer serial1_tx_buffer;
#else
static Ring_Buffer* serial1_rx_buffer = nullptr;
static Ring_Buffer* serial1_tx_buffer = nullptr;
#endif

USARTSerial& __fetch_global_Serial1()
{
#if ((MODULE_FUNCTION == MOD_FUNC_USER_PART) || (MODULE_FUNCTION == MOD_FUNC_MONO_FIRMWARE))
	static USARTSerial serial1(HAL_USART_SERIAL1, &serial1_rx_buffer, &serial1_tx_buffer);
#else
  if (!serial1_rx_buffer) {
    serial1_rx_buffer = new Ring_Buffer();
  }
  if (!serial1_tx_buffer) {
    serial1_tx_buffer = new Ring_Buffer();
  }
  static USARTSerial serial1(HAL_USART_SERIAL1, serial1_rx_buffer, serial1_tx_buffer);
#endif
	return serial1;
}

// optional Serial2 is instantiated from libraries/Serial2/Serial2.h
#endif
