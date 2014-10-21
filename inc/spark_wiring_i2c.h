/**
 ******************************************************************************
 * @file    spark_wiring_i2c.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_i2c.c module
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

#ifndef __SPARK_WIRING_I2C_H
#define __SPARK_WIRING_I2C_H

#include "spark_wiring_stream.h"

#define BUFFER_LENGTH 32
#define EVENT_TIMEOUT 100

#define CLOCK_SPEED_100KHZ (uint32_t)100000
#define CLOCK_SPEED_400KHZ (uint32_t)400000

class TwoWire : public Stream
{
private:
  static I2C_InitTypeDef I2C_InitStructure;

  static uint32_t I2C_ClockSpeed;
  static bool I2C_EnableDMAMode;
  static bool I2C_SetAsSlave;
  static bool I2C_Enabled;

  static uint8_t rxBuffer[];
  static uint8_t rxBufferIndex;
  static uint8_t rxBufferLength;

  static uint8_t txAddress;
  static uint8_t txBuffer[];
  static uint8_t txBufferIndex;
  static uint8_t txBufferLength;

  static uint8_t transmitting;
  static void (*user_onRequest)(void);
  static void (*user_onReceive)(int);
public:
  TwoWire();
  void setSpeed(uint32_t);
  void enableDMAMode(bool);
  void stretchClock(bool);
  void begin();
  void begin(uint8_t);
  void begin(int);
  void beginTransmission(uint8_t);
  void beginTransmission(int);
  uint8_t endTransmission(void);
  uint8_t endTransmission(uint8_t);
  uint8_t requestFrom(uint8_t, uint8_t);
  uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
  uint8_t requestFrom(int, int);
  uint8_t requestFrom(int, int, int);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *, size_t);
  virtual int available(void);
  virtual int read(void);
  virtual int peek(void);
  virtual void flush(void);
  void onReceive( void (*)(int) );
  void onRequest( void (*)(void) );

  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }
  using Print::write;

  static void slaveEventHandler(void);
  static bool isEnabled(void);
};

extern TwoWire Wire;

#endif

