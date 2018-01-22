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

FuelGauge::FuelGauge(bool _lock) :
    lock_(_lock)
{
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
#if (PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION) // what PRODUCT_ID will E0 get?
	return this->begin(Wire3);  // with Electron the fuel gauge will be attached to Wire3
#else                                             
   return this->begin(Wire);   // otherwise we assume it to be on default Wire
#endif
}
 
boolean FuelGauge::begin(TwoWire& i2c)
{
	_i2c = &i2c;
	/* as per comment of Mat pointers will always be initialzed 
	//if (_i2c) { 
	*/

    if (!_i2c->isEnabled()) 
		_i2c->begin();
    return _i2c->isEnabled();

	/*
	//}
	//return 0;
	*/
}

namespace detail {
	// Converts VCELL_REGISTER reading to Battery Voltage
	float _getVCell(byte MSB, byte LSB) {
		// VCELL = 12-bit value, 1.25mV (1V/800) per bit
		float value = (float)((MSB << 4) | (LSB >> 4));
		return value / 800.0;
	}

	// Converts SOC_REGISTER reading to state of charge of the cell as a percentage
	float _getSoC(byte MSB, byte LSB) {
		// MSB is the whole number
		// LSB is the decimal, resolution in units 1/256%
		float decimal = LSB / 256.0;
		return MSB + decimal;
	}
} // namespace detail

// Read and return the cell voltage
float FuelGauge::getVCell() {

	byte MSB = 0;
	byte LSB = 0;

	readRegister(VCELL_REGISTER, MSB, LSB);
	return detail::_getVCell(MSB, LSB);
}

// Read and return the state of charge of the cell
float FuelGauge::getSoC() {

	byte MSB = 0;
	byte LSB = 0;

	readRegister(SOC_REGISTER, MSB, LSB);
	return detail::_getSoC(MSB, LSB);
}

float FuelGauge::getNormalizedSoC() {
    std::lock_guard<FuelGauge> l(*this);
    PMIC power(true);

    const float soc = getSoC() / 100.0f;
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
}

// Return the version number of the chip
int FuelGauge::getVersion() {

	byte MSB = 0;
	byte LSB = 0;

	readRegister(VERSION_REGISTER, MSB, LSB);
	return (MSB << 8) | LSB;
}

byte FuelGauge::getCompensateValue() {

	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);
	return MSB;
}

byte FuelGauge::getAlertThreshold() {

	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);
	return 32 - (LSB & 0x1F);
}

void FuelGauge::setAlertThreshold(byte threshold) {

	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);
	if(threshold > 32) threshold = 32;
	threshold = 32 - threshold;

	writeRegister(CONFIG_REGISTER, MSB, (LSB & 0xE0) | threshold);
}

// Check if alert interrupt was generated
boolean FuelGauge::getAlert() {

	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);
	return LSB & 0x20;
}

void FuelGauge::clearAlert() {

	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);

	// Clear ALRT bit
    writeRegister(CONFIG_REGISTER, MSB, LSB & ~(0x20));
}

void FuelGauge::reset() {

	writeRegister(COMMAND_REGISTER, 0x00, 0x54);
}

void FuelGauge::quickStart() {

	writeRegister(MODE_REGISTER, 0x40, 0x00);
}

void FuelGauge::sleep() {

    std::lock_guard<FuelGauge> l(*this);
	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);

	writeRegister(CONFIG_REGISTER, MSB, (LSB | 0b10000000));

}

void FuelGauge::wakeup() {
    std::lock_guard<FuelGauge> l(*this);
	byte MSB = 0;
	byte LSB = 0;

	readConfigRegister(MSB, LSB);

	writeRegister(CONFIG_REGISTER, MSB, (LSB & 0b01111111));

}


void FuelGauge::readConfigRegister(byte &MSB, byte &LSB) {
	readRegister(CONFIG_REGISTER, MSB, LSB);
}


void FuelGauge::readRegister(byte startAddress, byte &MSB, byte &LSB) {
    std::lock_guard<FuelGauge> l(*this);
	
	if (_i2c) { 
		_i2c->beginTransmission(MAX17043_ADDRESS);
		_i2c->write(startAddress);
		_i2c->endTransmission(true);

		_i2c->requestFrom(MAX17043_ADDRESS, 2, true);
		MSB = _i2c->read();
		LSB = _i2c->read();
	}
	else { // e.g. since FuelGauge::begin() wasn't called
		DEBUG("I2C interface not initialized! Has FuelGauge::begin() been called earlier?");
	}
}

void FuelGauge::writeRegister(byte address, byte MSB, byte LSB) {
    std::lock_guard<FuelGauge> l(*this);
	if (_i2c) { 
		_i2c->beginTransmission(MAX17043_ADDRESS);
		_i2c->write(address);
		_i2c->write(MSB);
		_i2c->write(LSB);
		_i2c->endTransmission(true);
	}
	else { // e.g. since FuelGauge::begin() wasn't called
		DEBUG("I2C interface not initialized! Has FuelGauge::begin() been called earlier?");
	}
}

bool FuelGauge::lock() {
	if (_i2c) { 
		return _i2c->lock();
	}
	else { // e.g. since FuelGauge::begin() wasn't called
		DEBUG("I2C interface not initialized! Has FuelGauge::begin() been called earlier?");
		return false;
	}
}

bool FuelGauge::unlock() {
	if (_i2c) { 
		return _i2c->unlock();
	}
	else { // e.g. since FuelGauge::begin() wasn't called
		DEBUG("I2C interface not initialized! Has FuelGauge::begin() been called earlier?");
		return false;
	}
}
