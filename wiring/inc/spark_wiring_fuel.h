/**
 ******************************************************************************
 * @file    spark_wiring_fuel.h
 * @author  Mohit Bhoite
            Based on original library published by Luca Dentella (http://www.lucadentella.it)
 * @version V1.0.0
 * @date    11-August-2015
 * @brief   Header for spark_wiring_fuel.cpp module
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

#ifndef __SPARK_WIRING_FUEL_H
#define __SPARK_WIRING_FUEL_H

#include "spark_wiring_platform.h"
#include "spark_wiring.h"
#include "spark_wiring_i2c.h"

//Default MAX17043 I2C address
#define MAX17043_ADDRESS  0x36

// Register definitions
#define VCELL_REGISTER    0x02
#define SOC_REGISTER      0x04
#define MODE_REGISTER     0x06
#define VERSION_REGISTER  0x08
#define CONFIG_REGISTER   0x0C
#define COMMAND_REGISTER  0xFE

/* detail functions defined for unit tests */
namespace detail {
    float _getVCell(byte MSB, byte LSB);
    float _getSoC(byte MSB, byte LSB);
}

class FuelGauge {
public:
    FuelGauge(bool _lock = false);
    FuelGauge(TwoWire& i2c, bool _lock = false);
    ~FuelGauge();

    boolean begin();

    float getVCell();
    float getSoC();
    float getNormalizedSoC();
    int getVersion();
    byte getCompensateValue();
    byte getAlertThreshold();
    void setAlertThreshold(byte threshold);
    boolean getAlert();
    void clearAlert();
    void reset();
    void quickStart();
    void sleep();
    void wakeup();

    bool lock();
    bool unlock();

private:

    void readConfigRegister(byte &MSB, byte &LSB);
    void readRegister(byte startAddress, byte &MSB, byte &LSB);
    void writeRegister(byte address, byte MSB, byte LSB);

    TwoWire& i2c_;
    bool lock_;
};

#endif // __SPARK_WIRING_FUEL_H
