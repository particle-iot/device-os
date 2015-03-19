/**
 ******************************************************************************
 * @file    spark_wiring_usartserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usartserial.c module
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

#ifndef __SPARK_WIRING_USARTSERIAL_H
#define __SPARK_WIRING_USARTSERIAL_H

#include "spark_wiring_stream.h"
#include "usart_hal.h"

/* serial configurations (some HW implementations may not support specific formats)
 * packet is configured as a collection of 2-bit flags for simple translation
 * bits 1(LSB)-2: stop bits [0b00=1 stop bit, 0b01=2 stop bits, 0b10=0.5 stop bits, 0b11 = 1.5 stop bits]
 * bits 3-4: parity [0b00=none, 0b01=even, 0b10=odd]
 * bits 5-6: data bits [0b00=5, 0b01=6, 0b10=7, 0b11=8]
 * bits 7-8(MSB): hardware flow control [0b00=none, 0b01=RTS, 0b10=CTS, 0b11=RTS+CTS]
 * Hardware implementation should include a macro to check for 
 */
#define SERIAL_5N1 0b00000000
#define SERIAL_6N1 0b00010000
#define SERIAL_7N1 0b00100000
#define SERIAL_8N1 0b00110000
#define SERIAL_5N2 0b00000001
#define SERIAL_6N2 0b00010001
#define SERIAL_7N2 0b00100001
#define SERIAL_8N2 0b00110001
#define SERIAL_5E1 0b00000100
#define SERIAL_6E1 0b00010100
#define SERIAL_7E1 0b00100100
#define SERIAL_8E1 0b00110100
#define SERIAL_5E2 0b00000101
#define SERIAL_6E2 0b00010101
#define SERIAL_7E2 0b00100101
#define SERIAL_8E2 0b00110101
#define SERIAL_5O1 0b00001000
#define SERIAL_6O1 0b00011000
#define SERIAL_7O1 0b00101000
#define SERIAL_8O1 0b00111000
#define SERIAL_5O2 0b00001001
#define SERIAL_6O2 0b00011001
#define SERIAL_7O2 0b00101001
#define SERIAL_8O2 0b00111001

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

extern USARTSerial Serial1; // USART2 on PA2/3 (Spark TX, RX)
extern USARTSerial Serial2; // USART1 on alternate PB6/7 (Spark D1, D0)

#endif
