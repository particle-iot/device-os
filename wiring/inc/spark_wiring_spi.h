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
};

namespace particle {
class SPISettings : public Printable {
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
    : default_{false},
      clock_{clock},
      bitOrder_{bitOrder},
      dataMode_{dataMode}
  {
  }

  SPISettings()
  {
  }

  virtual ~SPISettings() {
  }

  bool operator==(const SPISettings& other) const
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

  bool operator>=(const SPISettings& other) const
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

  bool operator<=(const SPISettings& other) const
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

  bool operator!=(const SPISettings& other) const
  {
    return !(other == *this);
  }

  virtual size_t printTo(Print& p) const
  {
    if (default_ && clock_ == 0)
      return p.print("<SPISettings default>");
    else
      return p.printf("<SPISettings %s%u %s MODE%u>", default_ ? "default " : "", (unsigned int)clock_,
          bitOrder_ == MSBFIRST ? "MSB" : "LSB", (unsigned int)dataMode_);
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

// Compatibility typedef
typedef SPISettings __SPISettings;

}

// NOTE: when modifying this class (method signatures, adding/removing methods)
// make sure to update spark_wiring_spi_proxy.h and wiring/api SPI tests to reflect these changes.
class SPIClass {
private:
  hal_spi_interface_t _spi;

  /**
   * \brief Divider Reference Clock
   *
   * Set the divider reference clock.
   * The default is the system clock.
   */
  unsigned _dividerReference;

public:
  SPIClass(hal_spi_interface_t spi);
  ~SPIClass() = default;

  // Prevent copying
  SPIClass(const SPIClass&) = delete;
  SPIClass& operator=(const SPIClass&) = delete;

  hal_spi_interface_t interface() const {
    return _spi;
  }

  void begin();
  void begin(uint16_t);
  void begin(hal_spi_mode_t mode, uint16_t ss_pin = SPI_DEFAULT_SS);
  void end();

  void setBitOrder(uint8_t);
  void setDataMode(uint8_t);

  static void usingInterrupt(uint8_t) {};

  int32_t beginTransaction();
  int32_t beginTransaction(const particle::SPISettings& settings);
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
  void transfer(const void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback);

  void attachInterrupt();
  void detachInterrupt();

  bool isEnabled(void);

  void onSelect(wiring_spi_select_callback_t user_callback);
  void transferCancel();
  int32_t available();

  bool trylock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    hal_spi_acquire_config_t conf = {
      .size = sizeof(conf),
      .version = 0,
      .timeout = 0
    };
    return hal_spi_acquire(_spi, &conf) == SYSTEM_ERROR_NONE;
#elif !PLATFORM_THREADING
    return true;
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
  }

  int lock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    return hal_spi_acquire(_spi, nullptr);
#elif !PLATFORM_THREADING
    return 0;
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
  }

  void unlock()
  {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
    hal_spi_release(_spi, nullptr);
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
  }
};

#ifndef SPARK_WIRING_NO_SPI

#include "spark_wiring_spi_proxy.h"

namespace particle {
namespace globals {

extern ::particle::SpiProxy<HAL_SPI_INTERFACE1> SPI;

#if Wiring_SPI1
#ifdef SPI1
#undef SPI1
#endif // SPI1

extern ::particle::SpiProxy<HAL_SPI_INTERFACE2> SPI1;

#endif // Wiring_SPI1

#if Wiring_SPI2
#ifdef SPI2
#undef SPI2
#endif // SPI2

extern ::particle::SpiProxy<HAL_SPI_INTERFACE3> SPI2;

#endif // Wiring_SPI2

} } // particle::globals

using particle::globals::SPI;

#if Wiring_SPI1
using particle::globals::SPI1;
#endif // Wiring_SPI1

#if Wiring_SPI2
using particle::globals::SPI2;
#endif // Wiring_SPI1

#endif  // SPARK_WIRING_NO_SPI

#endif
