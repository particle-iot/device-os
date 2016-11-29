/**
 ******************************************************************************
 * @file    i2c_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "i2c_hal.h"
#include "gpio_hal.h"
#define NAMESPACE_WPI_PINMODE
#include "wiringPi.h"
#include "wiringPiI2C.h"
#include <unistd.h>
#include <map>
#include <deque>
#include <memory>
#include <algorithm>

/* Private define ------------------------------------------------------------*/
#define TOTAL_I2C   1

/* Private variables ---------------------------------------------------------*/
using AddressFileMap = std::map<uint8_t, int>;
using ByteBuffer = std::deque<uint8_t>;

typedef struct RPi_I2C_Info {
    bool I2C_Enabled;
    int transactionAddress;
    AddressFileMap addressFileMap;
    ByteBuffer rxBuffer;
    ByteBuffer txBuffer;
} RPi_I2C_Info;

RPi_I2C_Info I2C_MAP[TOTAL_I2C] =
{
};

static RPi_I2C_Info *i2cMap[TOTAL_I2C]; // pointer to I2C_MAP[] containing I2C peripheral info

void HAL_I2C_Init(HAL_I2C_Interface i2c, void* reserved)
{
  if (i2c == HAL_I2C_INTERFACE1)
  {
    i2cMap[i2c] = &I2C_MAP[0];
  }

  i2cMap[i2c]->I2C_Enabled = false;
  i2cMap[i2c]->transactionAddress = -1;
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
  // unimplemented
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{
  // unimplemented
}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
  i2cMap[i2c]->I2C_Enabled = true;
  i2cMap[i2c]->transactionAddress = -1;
  i2cMap[i2c]->rxBuffer.clear();
  i2cMap[i2c]->txBuffer.clear();
}

void HAL_I2C_End(HAL_I2C_Interface i2c, void* reserved)
{
  i2cMap[i2c]->I2C_Enabled = false;
}

int HAL_I2C_File_From_Address(HAL_I2C_Interface i2c, uint8_t address) {
  auto &devices = i2cMap[i2c]->addressFileMap;

  int file = devices[address];
  if (file <= 0) {
    file = wiringPiI2CSetup(address);
    devices[address] = file;
  }

  return file;
}

uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved)
{
  int file = HAL_I2C_File_From_Address(i2c, address);
  if (file <= 0) {
    return 0;
  }

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[quantity]);

  size_t count = read(file, &buffer[0], quantity);

  std::copy(&buffer[0], &buffer[count], std::back_inserter(i2cMap[i2c]->rxBuffer));

  return count;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address, void* reserved)
{
  i2cMap[i2c]->transactionAddress = address;
  i2cMap[i2c]->txBuffer.clear();
}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop, void* reserved)
{
  int address = i2cMap[i2c]->transactionAddress;
  if (address < 0) {
    return 0;
  }

  int file = HAL_I2C_File_From_Address(i2c, (uint8_t) address);
  if (file <= 0) {
    return 0;
  }

  auto &txBuffer = i2cMap[i2c]->txBuffer;
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[txBuffer.size()]);

  std::copy(txBuffer.begin(), txBuffer.end(), &buffer[0]);

  size_t count = write(file, &buffer[0], txBuffer.size());

  i2cMap[i2c]->transactionAddress = -1;
  return count;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data, void* reserved)
{
  i2cMap[i2c]->txBuffer.push_back(data);
  return 0;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c, void* reserved)
{
  return i2cMap[i2c]->rxBuffer.size();
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c, void* reserved)
{
  auto &rxBuffer = i2cMap[i2c]->rxBuffer;
  if (rxBuffer.empty()) {
    return -1;
  }

  auto value = rxBuffer.front();
  rxBuffer.pop_front();
  return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c, void* reserved)
{
  auto &rxBuffer = i2cMap[i2c]->rxBuffer;
  if (rxBuffer.empty()) {
    return -1;
  }

  auto value = rxBuffer.front();
  return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c, void* reserved)
{
  // unimplemented
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c, void* reserved)
{
  return i2cMap[i2c]->I2C_Enabled;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int), void* reserved)
{
  // unimplemented - only for slave
}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void), void* reserved)
{
  // unimplemented - only for slave
}

void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable, void* reserved)
{
  // unimplemented
}
