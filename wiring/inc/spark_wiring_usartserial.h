/**
 ******************************************************************************
 * @file    spark_wiring_usartserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usartserial.c module
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

#ifndef __SPARK_WIRING_USARTSERIAL_H
#define __SPARK_WIRING_USARTSERIAL_H

#include "spark_wiring_stream.h"
#include "usart_hal.h"

class USARTSerial : public Stream
{
private:
  HAL_USART_Serial _serial;
public:
  USARTSerial(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer);
  virtual ~USARTSerial() {};
  void begin(unsigned long);
  void begin(unsigned long, uint8_t);
  void end();

  virtual int available(void);
  virtual int peek(void);
  virtual int read(void);
  virtual void flush(void);
  virtual size_t write(uint8_t);

  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }

  using Print::write; // pull in write(str) and write(buf, size) from Print

  operator bool();

  bool isEnabled(void);
};

#ifndef SPARK_WIRING_NO_USART_SERIAL
extern USARTSerial Serial1;
extern USARTSerial Serial2;
#endif

#endif
