/**
 ******************************************************************************
 * @file    spark_wiring_fuel.cpp
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    11-August-2015
 * @brief   driver for the fuel gauge IC  MAX17043
            MAX17043 datasheet: http://datasheets.maximintegrated.com/en/ds/MAX17043-MAX17044.pdf
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

#include "spark_wiring_fuel.h"

#include <mutex>
#include "spark_wiring_power.h"
#include "check.h"

namespace {

TwoWire* fuelWireInstance() {
#if HAL_PLATFORM_FUELGAUGE_MAX17043
    switch (HAL_PLATFORM_FUELGAUGE_MAX17043_I2C) {
        case HAL_I2C_INTERFACE1:
        default: {
            return &Wire;
        }
#if Wiring_Wire1
        case HAL_I2C_INTERFACE2: {
            return &Wire1;
        }
#endif // Wiring_Wire1
#if Wiring_Wire3
        case HAL_I2C_INTERFACE3: {
            return &Wire3;
        }
#endif // Wiring_Wire3
    }
#endif // HAL_PLATFORM_FUELGAUGE_MAX17043

    return &Wire;
}

} // anonymous

FuelGauge::FuelGauge(bool _lock)
        : FuelGauge(*fuelWireInstance(), _lock)
{
}

FuelGauge::FuelGauge(TwoWire& i2c, bool _lock)
    : i2c_(i2c),
      lock_(_lock) {

    if (lock_) {
        lock();
    }
}

FuelGauge::~FuelGauge()
{
    if (lock_) {
        unlock();
    }
}

boolean FuelGauge::begin()
{
    if (!i2c_.isEnabled()) {
        i2c_.begin();
    }
    return i2c_.isEnabled();
}

namespace particle {
namespace detail {
    // Converts VCELL_REGISTER reading to Battery Voltage
    float _getVCell(byte MSB, byte LSB) {
        // VCELL = 12-bit value, 1.25mV (1V/800) per bit
        float value = (float)((MSB << 4) | (LSB >> 4));
        return value / 800.0;
    }

    // Converts SOC_REGISTER reading to state of charge of the cell as a percentage
    float _getSoC(byte MSB, byte LSB, byte soc_bits_precision) {
        float soc_percent = 0;

        // Maxim ModelGauge doc only mentions 18 and 19 bit
        // MSB is the whole number
        // LSB is the decimal, resolution in units 1/256%
        if (soc_bits_precision == particle::power::SOC_19_BIT_PRECISION) {
            soc_percent = (((uint32_t)MSB << 8) + LSB) / 512.0f; // per datasheet
        } else { // default to 18-bit calculation
            soc_percent = (((uint32_t)MSB << 8) + LSB) / 256.0f; // per datasheet
        }
        return soc_percent;

    }
} // namespace detail
} // namespace particle

// Read and return the cell voltage
float FuelGauge::getVCell() {
    byte MSB = 0;
    byte LSB = 0;

    if (readRegister(VCELL_REGISTER, MSB, LSB) != SYSTEM_ERROR_NONE) {
        return -1.0f;
    }
    return particle::detail::_getVCell(MSB, LSB);
}

// Read and return the state of charge of the cell
float FuelGauge::getSoC() {
    byte MSB = 0;
    byte LSB = 0;

    if (readRegister(SOC_REGISTER, MSB, LSB) != SYSTEM_ERROR_NONE) {
        return -1.0f;
    }

    // Load SoC Bit Precision from SystemPowerConfiguration
    int soc_bits = particle::power::DEFAULT_SOC_18_BIT_PRECISION;
#if HAL_PLATFORM_POWER_MANAGEMENT
    hal_power_config config = {};
    config.size = sizeof(config);
    if (system_power_management_get_config(&config, nullptr) == SYSTEM_ERROR_NONE) {
        soc_bits = config.soc_bits;
    };
#endif

    return particle::detail::_getSoC(MSB, LSB, soc_bits);
}

float FuelGauge::getNormalizedSoC() {
#if HAL_PLATFORM_PMIC_BQ24195
    std::lock_guard<FuelGauge> l(*this);
    PMIC power(true);

    const float soc = getSoC() / 100.0f;
    if (soc < 0) {
        return -1.0f;
    }
    const float termV = ((float)power.getChargeVoltageValue()) / 1000.0f;
    const float magicVoltageDiff = 0.1f;
    const float reference100PercentV = 4.2f;
    const float referenceMaxV = std::max(reference100PercentV, termV) - magicVoltageDiff;

    const float magicError = 0.05f;
    const float maxCharge = (1.0f - (reference100PercentV - referenceMaxV)) - magicError;
    const float minCharge = 0.0f; // 0%

    float normalized = (soc - minCharge) * (1.0f / (maxCharge - minCharge)) + 0.0f;
    // Clamp at [0.0, 1.0]
    if (normalized < 0.0f) {
        normalized = 0.0f;
    } else if (normalized > 1.0f) {
        normalized = 1.0f;
    }

    return normalized * 100.0f;
#else
    // TODO: should normalized SoC use the raw SoC when there is no PMIC ?
    return 0.0f;
#endif // HAL_PLATFORM_PMIC_BQ24195
}

// Return the version number of the chip
int FuelGauge::getVersion() {

    byte MSB = 0;
    byte LSB = 0;

    CHECK(readRegister(VERSION_REGISTER, MSB, LSB));
    return (MSB << 8) | LSB;
}

byte FuelGauge::getCompensateValue() {
    byte MSB = 0;
    byte LSB = 0;

    if (readConfigRegister(MSB, LSB) != SYSTEM_ERROR_NONE) {
        return 0xFF;
    }
    return MSB;
}

byte FuelGauge::getAlertThreshold() {
    byte MSB = 0;
    byte LSB = 0;

    if (readConfigRegister(MSB, LSB) != SYSTEM_ERROR_NONE) {
        return 0xFF;
    }
    return 32 - (LSB & 0x1F);
}

int FuelGauge::setAlertThreshold(byte threshold) {
    byte MSB = 0;
    byte LSB = 0;

    CHECK(readConfigRegister(MSB, LSB));
    if(threshold > 32) {
        threshold = 32;
    }
    threshold = 32 - threshold;

    CHECK(writeRegister(CONFIG_REGISTER, MSB, (LSB & 0xE0) | threshold));
    return SYSTEM_ERROR_NONE;
}

// Check if alert interrupt was generated
boolean FuelGauge::getAlert() {
    byte MSB = 0;
    byte LSB = 0;

    if (readConfigRegister(MSB, LSB) != SYSTEM_ERROR_NONE) {
        return false;
    }
    return LSB & 0x20;
}

int FuelGauge::clearAlert() {
    byte MSB = 0;
    byte LSB = 0;

    CHECK(readConfigRegister(MSB, LSB));
    // Clear ALRT bit
    CHECK(writeRegister(CONFIG_REGISTER, MSB, LSB & ~(0x20)));
    return SYSTEM_ERROR_NONE;
}

int FuelGauge::reset() {
    CHECK(writeRegister(COMMAND_REGISTER, 0x00, 0x54));
    return SYSTEM_ERROR_NONE;
}

int FuelGauge::quickStart() {
    CHECK(writeRegister(MODE_REGISTER, 0x40, 0x00));
    return SYSTEM_ERROR_NONE;
}

int FuelGauge::sleep() {
    std::lock_guard<FuelGauge> l(*this);
    byte MSB = 0;
    byte LSB = 0;

    CHECK(readConfigRegister(MSB, LSB));
    CHECK(writeRegister(CONFIG_REGISTER, MSB, (LSB | 0b10000000)));
    return SYSTEM_ERROR_NONE;
}

int FuelGauge::wakeup() {
    std::lock_guard<FuelGauge> l(*this);
    byte MSB = 0;
    byte LSB = 0;

    CHECK(readConfigRegister(MSB, LSB));
    CHECK(writeRegister(CONFIG_REGISTER, MSB, (LSB & 0b01111111)));
    return SYSTEM_ERROR_NONE;
}


int FuelGauge::readConfigRegister(byte &MSB, byte &LSB) {
    return readRegister(CONFIG_REGISTER, MSB, LSB);
}


int FuelGauge::readRegister(byte startAddress, byte &MSB, byte &LSB) {
    std::lock_guard<FuelGauge> l(*this);
    WireTransmission config(MAX17043_ADDRESS);
    config.timeout(FUELGAUGE_DEFAULT_TIMEOUT);
    i2c_.beginTransmission(config);
    i2c_.write(startAddress);
    CHECK_TRUE(i2c_.endTransmission(true) == 0, SYSTEM_ERROR_TIMEOUT);

    config.quantity(2);
    CHECK_TRUE(i2c_.requestFrom(config) == 2, SYSTEM_ERROR_TIMEOUT);
    MSB = i2c_.read();
    LSB = i2c_.read();

    return SYSTEM_ERROR_NONE;
}

int FuelGauge::writeRegister(byte address, byte MSB, byte LSB) {
    std::lock_guard<FuelGauge> l(*this);
    WireTransmission config(MAX17043_ADDRESS);
    config.timeout(FUELGAUGE_DEFAULT_TIMEOUT);
    i2c_.beginTransmission(config);
    i2c_.write(address);
    i2c_.write(MSB);
    i2c_.write(LSB);
    CHECK_TRUE(i2c_.endTransmission(true) == 0, SYSTEM_ERROR_TIMEOUT);
    return SYSTEM_ERROR_NONE;
}

bool FuelGauge::lock() {
    return i2c_.lock();
}

bool FuelGauge::unlock() {
    return i2c_.unlock();
}
