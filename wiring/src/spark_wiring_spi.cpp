/**
 ******************************************************************************
 * @file    spark_wiring_spi.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring SPI module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>

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

#include "spark_wiring_spi.h"
#include "core_hal.h"
#include "spark_macros.h"

static void querySpiInfo(HAL_SPI_Interface spi, hal_spi_info_t* info)
{
  memset(info, 0, sizeof(hal_spi_info_t));
  info->version = HAL_SPI_INFO_VERSION_1;
  HAL_SPI_Info(spi, info, nullptr);
}

static particle::__SPISettings spiSettingsFromSpiInfo(hal_spi_info_t* info)
{
  if (!info->enabled || info->default_settings)
    return particle::__SPISettings();
  return particle::__SPISettings(info->clock, info->bit_order, info->data_mode);
}

SPIClass::SPIClass(HAL_SPI_Interface spi)
{
  _spi = spi;
  HAL_SPI_Init(_spi);
  dividerReference = SPI_CLK_SYSTEM;     // 0 indicates the system clock
}

void SPIClass::begin()
{
    // todo - fetch default pin from HAL
  HAL_SPI_Begin(_spi, SPI_DEFAULT_SS);
}

void SPIClass::begin(uint16_t ss_pin)
{
  if (ss_pin >= TOTAL_PINS)
  {
    return;
  }

  HAL_SPI_Begin(_spi, ss_pin);
}

void SPIClass::begin(SPI_Mode mode, uint16_t ss_pin)
{
  if (ss_pin >= TOTAL_PINS)
  {
    return;
  }

  HAL_SPI_Begin_Ext(_spi, mode, ss_pin, NULL);
}

void SPIClass::end()
{
  trylock();
  unlock();
  HAL_SPI_End(_spi);
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
  HAL_SPI_Set_Bit_Order(_spi, bitOrder);
}

void SPIClass::setDataMode(uint8_t mode)
{
  HAL_SPI_Set_Data_Mode(_spi, mode);
}

int32_t SPIClass::beginTransaction()
{
  lock();
  return 0;
}

int32_t SPIClass::beginTransaction(const particle::__SPISettings& settings)
{
  lock();
  // Get Current SPISettings
  hal_spi_info_t info;
  querySpiInfo(_spi, &info);
  particle::__SPISettings current = spiSettingsFromSpiInfo(&info);
  // If they differ, reconfigure SPI peripheral
  if (settings != current)
  {
    if (!settings.default_) {
      uint8_t divisor = 0;
      unsigned int clock;
      computeClockDivider((unsigned int)info.system_clock, settings.clock_, divisor, clock);

      if (!(current <= settings && clock == current.clock_)) {
        HAL_SPI_Set_Settings(_spi, 0, divisor, settings.bitOrder_, settings.dataMode_, nullptr);
      }
    } else {
      HAL_SPI_Set_Settings(_spi, 1, 0, 0, 0, nullptr);
    }
  }

  return 0;
}

void SPIClass::endTransaction()
{
  unlock();
}


void SPIClass::setClockDividerReference(unsigned value, unsigned scale)
{
    dividerReference = value*scale;
    // set the clock to 1/4 of the reference by default.
    // We assume this is called before externally calling setClockDivider.
    setClockDivider(SPI_CLOCK_DIV4);
}

/**
 * The divisors. The index+1 is the power of 2 of the divisor.
 */
static uint8_t clock_divisors[] = {
    SPI_CLOCK_DIV2,
    SPI_CLOCK_DIV4,
    SPI_CLOCK_DIV8,
    SPI_CLOCK_DIV16,
    SPI_CLOCK_DIV32,
    SPI_CLOCK_DIV64,
    SPI_CLOCK_DIV128,
    SPI_CLOCK_DIV256
};

uint8_t divisorShiftScale(uint8_t divider)
{
    unsigned result = 0;
    for (; result<arraySize(clock_divisors); result++)
    {
        if (clock_divisors[result]==divider)
            break;
    }
    return result+1;
}

void SPIClass::setClockDivider(uint8_t rate)
{
    if (dividerReference)
    {
        // determine the clock speed
        uint8_t scale = divisorShiftScale(rate);
        unsigned targetSpeed = dividerReference>>scale;
        setClockSpeed(targetSpeed);
    }
    else
    {
        HAL_SPI_Set_Clock_Divider(_spi, rate);
    }
}

void SPIClass::computeClockDivider(unsigned reference, unsigned targetSpeed, uint8_t& divider, unsigned& clock)
{
    clock = reference;
    uint8_t scale = 0;
    clock >>= 1;        // div2 is the first
    while (clock > targetSpeed && scale<7) {
        clock >>= 1;
        scale++;
    }
    divider = clock_divisors[scale];
}

unsigned SPIClass::setClockSpeed(unsigned value, unsigned value_scale)
{
    // actual speed is the system clock divided by some scalar
    unsigned targetSpeed = value*value_scale;
    hal_spi_info_t info;
    querySpiInfo(_spi, &info);
    uint8_t rate;
    unsigned clock;
    computeClockDivider(info.system_clock, targetSpeed, rate, clock);
    HAL_SPI_Set_Clock_Divider(_spi, rate);
    return clock;
}

byte SPIClass::transfer(byte _data)
{
  return HAL_SPI_Send_Receive_Data(_spi, _data);
}

void SPIClass::transfer(void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback)
{
  HAL_SPI_DMA_Transfer(_spi, tx_buffer, rx_buffer, length, user_callback);
  if (user_callback == NULL) {
    HAL_SPI_TransferStatus st;
    do {
      HAL_SPI_DMA_Transfer_Status(_spi, &st);
    } while(st.transfer_ongoing);
  }
}

void SPIClass::attachInterrupt()
{
  //To Do
}

void SPIClass::detachInterrupt()
{
  //To Do
}

bool SPIClass::isEnabled()
{
  return HAL_SPI_Is_Enabled(_spi);
}

void SPIClass::onSelect(wiring_spi_select_callback_t user_callback)
{
  HAL_SPI_Set_Callback_On_Select(_spi, user_callback, NULL);
}

void SPIClass::transferCancel()
{
  HAL_SPI_DMA_Transfer_Cancel(_spi);
}

int32_t SPIClass::available()
{
  return HAL_SPI_DMA_Transfer_Status(_spi, NULL);
}
