/**
 ******************************************************************************
 * @file    spark_wiring_i2c.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_i2c.c module
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

#ifndef __SPARK_WIRING_I2C_H
#define __SPARK_WIRING_I2C_H

#include "spark_wiring_stream.h"
#include "spark_wiring_platform.h"
#include "i2c_hal.h"
#include <chrono>

class WireTransmission {
public:
  WireTransmission(uint8_t address)
      : address_{address},
        size_{0},
        stop_{true},
        timeout_{HAL_I2C_DEFAULT_TIMEOUT_MS} {
  }

  WireTransmission() = delete;

  WireTransmission& quantity(size_t size) {
    size_ = size;
    return *this;
  }

  WireTransmission& timeout(system_tick_t ms) {
    timeout_ = ms;
    return *this;
  }

  WireTransmission& timeout(std::chrono::milliseconds ms) {
    return timeout((system_tick_t)ms.count());
  }

  WireTransmission& stop(bool stop) {
    stop_ = stop;
    return *this;
  }

  hal_i2c_transmission_config_t halConfig() const {
    hal_i2c_transmission_config_t conf = {
      .size = sizeof(hal_i2c_transmission_config_t),
      .version = 0,
      .address = address_,
      .reserved = {0},
      .quantity = (uint32_t)size_,
      .timeout_ms = timeout_,
      .flags = (uint32_t)(stop_ ? HAL_I2C_TRANSMISSION_FLAG_STOP : 0)
    };
    return conf;
  }

private:
  uint8_t address_;
  size_t size_;
  bool stop_;
  system_tick_t timeout_;
};

class TwoWire : public Stream
{
private:
  hal_i2c_interface_t _i2c;

public:
  TwoWire(hal_i2c_interface_t i2c, const hal_i2c_config_t& config);
  virtual ~TwoWire() {};
  inline void setClock(uint32_t speed) {
	  setSpeed(speed);
  }
  void setSpeed(uint32_t);
  void enableDMAMode(bool);
  void stretchClock(bool);
  void begin();
  void begin(uint8_t);
  void begin(int);
  void beginTransmission(uint8_t);
  void beginTransmission(int);
  void beginTransmission(const WireTransmission& transfer);
  void end();
  uint8_t endTransmission(void);
  uint8_t endTransmission(uint8_t);
  size_t requestFrom(uint8_t, size_t);
  size_t requestFrom(uint8_t, size_t, uint8_t);
  size_t requestFrom(const WireTransmission& transfer);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *, size_t);
  virtual int available(void);
  virtual int read(void);
  virtual int peek(void);
  virtual void flush(void);
  void onReceive(void (*)(int));
  void onRequest(void (*)(void));

  bool lock();
  bool unlock();

  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }
  using Print::write;

  bool isEnabled(void);

  /**
   * Attempts to reset this I2C bus.
   */
  void reset();
};

/**
 * This global instance cannot be gc-ed by the linker because of the virtual functions.
 * So we provide a conditional compile to exclude it.
 */
#ifndef SPARK_WIRING_NO_I2C

hal_i2c_config_t __attribute__((weak)) acquireWireBuffer();

#define Wire __fetch_global_Wire()
TwoWire& __fetch_global_Wire();

#if Wiring_Wire1
#ifdef Wire1
#undef Wire1
#endif  // Wire1

#define Wire1 __fetch_global_Wire1()
TwoWire& __fetch_global_Wire1();
hal_i2c_config_t __attribute__((weak)) acquireWire1Buffer();
#endif  // Wiring_Wire1

/* System PMIC and Fuel Guage I2C3 */
#if Wiring_Wire3
#ifdef Wire3
#undef Wire3
#endif  // Wire3

#define Wire3 __fetch_global_Wire3()
TwoWire& __fetch_global_Wire3();
hal_i2c_config_t __attribute__((weak)) acquireWire3Buffer();
#endif  // Wiring_Wire3

#endif

#endif

