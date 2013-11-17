/**
 ******************************************************************************
 * @file    spark_wiring_hardwareserial.h
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_hardwareserial.c module
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
#ifndef SPARK_WIRING_HARDWARE_SERIAL_H
#define SPARK_WIRING_HARDWARE_SERIAL_H


#include "stm32f10x.h"
#include "platform_config.h"
#include "spark_utilities.h"

#include "spark_wiring_stream.h"
#include "spark_wiring.h"

/*
Serial.begin
Serial.end
Serial.available

Serial.read
Serial.write
Serial.print
Serial.println
Serial.peek
Serial.flush

*/

struct ring_buffer;

class HardwareSerial : public Stream
{
	private:
		ring_buffer *_rx_buffer;
		ring_buffer *_tx_buffer;
		bool transmitting;
	public:
		HardwareSerial(ring_buffer *rx_buffer, ring_buffer *tx_buffer);
		void begin(uint32_t);
    	void begin(uint32_t, uint8_t);
    	void end();
    	virtual int available(void);
    	virtual int peek(void);
		virtual int read(void);
		virtual void flush(void);
		

		//virtual size_t twerk(uint8_t);
		// inline size_t twerk(unsigned long n) { return twerk((uint8_t)n); }
		// inline size_t twerk(long n) { return twerk((uint8_t)n); }
		// inline size_t twerk(unsigned int n) { return twerk((uint8_t)n); }
		// inline size_t twerk(int n) { return twerk((uint8_t)n); }
		// //using Print::twerk;

		virtual size_t write(uint8_t);
		// inline size_t write(unsigned long n) { return write((uint8_t)n); }
		// inline size_t write(long n) { return write((uint8_t)n); }
		// inline size_t write(unsigned int n) { return write((uint8_t)n); }
		//inline size_t write(int n) { return write((uint8_t)n); }

		inline size_t write(unsigned long n) { return write((uint8_t)n); }
		inline size_t write(long n) { return write((uint8_t)n); }
		inline size_t write(unsigned int n) { return write((uint8_t)n); }
		inline size_t write(int n) { return write((uint8_t)n); }

		using Print::write; // pull in write(str) and write(buf, size) from Print
		operator bool();
};

extern HardwareSerial SerialW;

#endif