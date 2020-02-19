/**
 ******************************************************************************
 * @file    spark_wiring_spi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_spi.c module
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

#ifndef __SPARK_WIRING_SPI_H
#define __SPARK_WIRING_SPI_H

#include "spark_wiring.h"
#include "spark_wiring_platform.h"
#include "spi_hal.h"

class SPIClass;

typedef void (*wiring_spi_dma_transfercomplete_callback_t)(void);
typedef void (*wiring_spi_select_callback_t)(uint8_t);

enum FrequencyScale
{
    HZ = 1,
    KHZ = HZ*1000,
    MHZ = KHZ*1000,
    SPI_CLK_SYSTEM = 0,         // represents the system clock speed
    SPI_CLK_ARDUINO = 16*MHZ,
    SPI_CLK_PHOTON = 60*MHZ
};

namespace particle {
class __SPISettings : public Printable {
public:
  __SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
    : default_{false},
      clock_{clock},
      bitOrder_{bitOrder},
      dataMode_{dataMode}
  {
  }

  __SPISettings()
  {
  }

  bool operator==(const __SPISettings& other) const
  {
    if (default_ && other.default_)
      return true;

    if (default_ == other.default_ &&
        clock_ == other.clock_ &&
        bitOrder_ == other.bitOrder_ &&
        dataMode_ == other.dataMode_)
    {
      return true;
    }

    return false;
  }

  bool operator>=(const __SPISettings& other) const
  {
    if (default_ && other.default_)
      return true;

    if (default_ == other.default_ &&
        clock_ >= other.clock_ &&
        bitOrder_ == other.bitOrder_ &&
        dataMode_ == other.dataMode_)
    {
      return true;
    }

    return false;
  }

  bool operator<=(const __SPISettings& other) const
  {
    if (default_ && other.default_)
      return true;

    if (default_ == other.default_ &&
        clock_ <= other.clock_ &&
        bitOrder_ == other.bitOrder_ &&
        dataMode_ == other.dataMode_)
    {
      return true;
    }

    return false;
  }

  bool operator!=(const __SPISettings& other) const
  {
    return !(other == *this);
  }

  virtual size_t printTo(Print& p) const
  {
    if (default_ && clock_ == 0)
      return p.print("<SPISettings default>");
    else
      return p.printf("<SPISettings %s%lu %s MODE%d>", default_ ? "default " : "", clock_, bitOrder_ == MSBFIRST ? "MSB" : "LSB", dataMode_);
  }

  uint32_t getClock() const {
    return clock_;
  }

private:
  friend class ::SPIClass;
  bool default_ = true;
  uint32_t clock_ = 0;
  uint8_t bitOrder_ = 0;
  uint8_t dataMode_ = 0;
};
}

class SPIClass {
private:
  HAL_SPI_Interface _spi;

  /**
   * \brief Divider Reference Clock
   *
   * Set the divider reference clock.
   * The default is the system clock.
   */
  unsigned _dividerReference;

#if PLATFORM_THREADING && !HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
  /**
   * \brief Mutex for Gen2 platforms
   *
   * Enables Gen2 platforms to synchronize access to the SPI peripheral
   */
  Mutex _mutex;
#endif

public:
  SPIClass(HAL_SPI_Interface spi);
  virtual ~SPIClass() {};

  void begin();
  void begin(uint16_t);
  void begin(SPI_Mode mode, uint16_t ss_pin = SPI_DEFAULT_SS);
  void end();

  void setBitOrder(uint8_t);
  void setDataMode(uint8_t);

  static void usingInterrupt(uint8_t) {};

  int32_t beginTransaction();
  int32_t beginTransaction(const particle::__SPISettings& settings);
  void endTransaction();

  /**
   * Sets the clock speed that the divider is relative to. This does not change
   * the assigned clock speed until the next call to {@link #setClockDivider}
   * @param value   The clock speed reference value
   * @param scale   The clock speed reference scalar
   *
   * E.g.
   *
   * setClockDividerReference(ARDUINO);
   * setClockDividerReference(16, MHZ);
   *
   * @see #setClockDivider
   */
  void setClockDividerReference(unsigned value, unsigned scale=HZ);

  /**
   * Sets the clock speed as a divider relative to the clock divider reference.
   * @param divider SPI_CLOCK_DIVx where x is a power of 2 from 2 to 256.
   */
  void setClockDivider(uint8_t divider);

  /**
   * Sets the absolute clock speed. This will select the clock divider that is no greater than
   * {@code value*scale}.
   * @param value
   * @param scale
   * @return the actual clock speed set.
   */
  unsigned setClockSpeed(unsigned value, unsigned scale=HZ);


   /*
    * Test method to compute the divider needed to attain a given clock frequency.
    */
  static void computeClockDivider(unsigned reference, unsigned targetSpeed, uint8_t& divider, unsigned& clock);

  byte transfer(byte _data);
  void transfer(void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback);

  void attachInterrupt();
  void detachInterrupt();

  bool isEnabled(void);

  void onSelect(wiring_spi_select_callback_t user_callback);
  void transferCancel();
  int32_t available();

  bool trylock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    HAL_SPI_AcquireConfig conf = {
      .size = sizeof(conf),
      .version = 0,
      .timeout = 0
    };
    return HAL_SPI_Acquire(_spi, &conf) == SYSTEM_ERROR_NONE;
#elif PLATFORM_THREADING
    return _mutex.trylock();
#else
    return true;
#endif
  }

  int lock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    return HAL_SPI_Acquire(_spi, nullptr);
#elif PLATFORM_THREADING
    _mutex.lock();
    return 0;
#else
    return 0;
#endif
  }

  void unlock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    HAL_SPI_Release(_spi, nullptr);
#elif PLATFORM_THREADING
    _mutex.unlock();
#endif
  }
};

#ifndef SPARK_WIRING_NO_SPI

namespace particle {
namespace globals {

SPIClass& instanceSpi();
#define SPI ::particle::globals::instanceSpi()

#if Wiring_SPI1
#ifdef SPI1
#undef SPI1
#endif  // SPI1

SPIClass& instanceSpi1();
#define SPI1 ::particle::globals::instanceSpi1()

#endif // Wiring_SPI1

#if Wiring_SPI2
#ifdef SPI2
#undef SPI2
#endif  // SPI2

SPIClass& instanceSpi2();
#define SPI2 ::particle::globals::instanceSpi2()

#endif // Wiring_SPI2

} } // particle::globals

#endif  // SPARK_WIRING_NO_SPI

#endif
