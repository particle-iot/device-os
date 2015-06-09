/**
 ******************************************************************************
 * @file    spark_wiring_i2c.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
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

#include "spark_wiring_i2c.h"
#include "i2c_hal.h"

// Constructors ////////////////////////////////////////////////////////////////

TwoWire::TwoWire()
{
}

// Public Methods //////////////////////////////////////////////////////////////

//setSpeed() should be called before begin() else default to 100KHz
void TwoWire::setSpeed(uint32_t clockSpeed)
{
  HAL_I2C_Set_Speed(clockSpeed);
}

//enableDMAMode(true) should be called before begin() else default polling mode used
void TwoWire::enableDMAMode(bool enableDMAMode)
{
  HAL_I2C_Enable_DMA_Mode(enableDMAMode);
}

void TwoWire::stretchClock(bool stretch)
{
  HAL_I2C_Stretch_Clock(stretch);
}

void TwoWire::begin(void)
{
  HAL_I2C_Begin(I2C_MODE_MASTER, 0x00);
}

void TwoWire::begin(uint8_t address)
{
  HAL_I2C_Begin(I2C_MODE_SLAVE, address);
}

void TwoWire::begin(int address)
{
  begin((uint8_t)address);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
  return HAL_I2C_Request_Data(address, quantity, sendStop);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

void TwoWire::beginTransmission(uint8_t address)
{
  HAL_I2C_Begin_Transmission(address);
}

void TwoWire::beginTransmission(int address)
{
  beginTransmission((uint8_t)address);
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
  return HAL_I2C_End_Transmission(sendStop);
}

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void)
{
  return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data)
{
  return HAL_I2C_Write_Data(data);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
  // in master/slave transmitter mode
  for(size_t i = 0; i < quantity; ++i)
  {
    write(data[i]);
  }

  return quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::available(void)
{
  return HAL_I2C_Available_Data();
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void)
{
  return HAL_I2C_Read_Data();
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void)
{
  return HAL_I2C_Peek_Data();
}

void TwoWire::flush(void)
{
  HAL_I2C_Flush_Data();
}

// sets function called on slave write
void TwoWire::onReceive( void (*function)(int) )
{
  HAL_I2C_Set_Callback_On_Receive(function);
}

// sets function called on slave read
void TwoWire::onRequest( void (*function)(void) )
{
  HAL_I2C_Set_Callback_On_Request(function);
}

bool TwoWire::isEnabled()
{
  return HAL_I2C_Is_Enabled();
}

// Preinstantiate Objects //////////////////////////////////////////////////////
#ifndef SPARK_WIRING_NO_I2C
TwoWire Wire = TwoWire();
#endif

