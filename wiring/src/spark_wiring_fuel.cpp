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
      lock_(_lock),
      config_() {

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

namespace particle { namespace detail {
    // Converts VCELL_REGISTER reading to Battery Voltage
    float _getVCell(byte MSB, byte LSB) {
        // VCELL = 12-bit value, 1.25mV (1V/800) per bit
        float value = (float)((MSB << 4) | (LSB >> 4));
        return value / 800.0;
    }

    // Converts SOC_REGISTER reading to state of charge of the cell as a percentage
    //TODO this needs to change to use the configured number of bytes
    float _getSoC(byte MSB, byte LSB, byte bits_resolution) {
        float soc_percent = 0;
        if (bits_resolution == 18)  { 
            soc_percent = (((uint32_t)MSB << 8) + LSB) / 256.0; 
        } 
        else if (bits_resolution == 19) { 
            soc_percent = (((uint32_t)MSB << 8) + LSB) / 512.0; 
        }
        else {
            // MSB is the whole number
            // LSB is the decimal, resolution in units 1/256%
            soc_percent = MSB + (LSB / 256.0);
        }
        return soc_percent;

    }
}} // namespace particle detail

// // Read and return the cell voltage
// float FuelGauge::getVCell() {
//     byte MSB = 0;
//     byte LSB = 0;

//     if (readRegister(VCELL_REGISTER, MSB, LSB) != SYSTEM_ERROR_NONE) {
//         return -1.0f;
//     }
//     // VCELL = 12-bit value, 1.25mV (1V/800) per bit
//     float value = (float)((MSB << 4) | (LSB >> 4));
//     return (value / 800.0);
// }

// Read and return the state of charge of the cell
float FuelGauge::getSoC() {

    byte MSB = 0;
    byte LSB = 0;
    byte bits_precision = 0; 
    if (config_.valid_config) {
        bits_precision = config_.bits;
    }
    if(readRegister(SOC_REGISTER, MSB, LSB) != SYSTEM_ERROR_NONE) {
        return -1.0f;
    }

    return particle::detail::_getSoC(MSB, LSB, bits_precision);
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
    // TODO should normalized SoC use the raw SoC when there is no PMIC ? 
    // It's weird that this driver has a dependency on another chip/driver that may not be present
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

int FuelGauge::writeBlock(byte address, const byte* block, size_t size) {
    std::lock_guard<FuelGauge> l(*this);
    WireTransmission config(MAX17043_ADDRESS);
    config.timeout(FUELGAUGE_DEFAULT_TIMEOUT);
    i2c_.beginTransmission(config);
    i2c_.write(address);
    size_t nwritten = i2c_.write((const uint8_t*)block, size);
    CHECK_TRUE(i2c_.endTransmission(true) == 0, SYSTEM_ERROR_TIMEOUT);
    if (nwritten != size) {
        //TODO return an error?
    }
    return SYSTEM_ERROR_NONE;
}

bool FuelGauge::lock() {
    return i2c_.lock();
}

bool FuelGauge::unlock() {
    return i2c_.unlock();
}

void FuelGauge::setValidationInterval(uint32_t seconds) {
    
}

int FuelGauge::setConfig(const PlatformFuelGaugeConfig_t &config) {
    config_ = config;
    config_.valid_config = true;
    SPARK_ASSERT(config_.OCVTest == config.OCVTest);
    SPARK_ASSERT(config_.model_data[BATTERY_MODEL_DATA_LENGTH-1] == config.model_data[BATTERY_MODEL_DATA_LENGTH-1])
    return reloadModel();
}


/**
 * @brief Load the battery model into the fuel gauge
 * @details The most recently known model is stored in config_ and can be
 * loaded onto the fuel gauge device using this method.
 * @return int Error code or success
 */
int FuelGauge::reloadModel() {
    int result_code = SYSTEM_ERROR_NONE;

    byte orig_OCV1, orig_OCV2; 
    while(1) {
        // To unlock the TABLE registers, write 0x57 to address 0x3F, and 0x4A to address 0x3E. 
        // While TABLE is unlocked, no ModelGauge registers are updated, so relock as soon as 
        // possible by writing 0x00 to address 0x3F, and 0x00 to address 0x3E.

        // unlock model access -- this isn't allowed to fail
        result_code = writeRegister(TABLE_LOCK_REGISTER, 0x4A, 0x57);
        if (result_code != SYSTEM_ERROR_NONE) {
            break;
        }
        // read OCV 
        readRegister(OCV_REGISTER, orig_OCV1, orig_OCV2);
        // Log.info("read original OCV: %d, %d", orig_OCV1, orig_OCV2);

        // verify model access unlocked 
        if((orig_OCV1 == 0xFF) && (orig_OCV2 == 0xFF))   { 
            // failed and retry 
            // Log.info("verify model access unlocked: failed");
            delay(100);
            // TODO this could end up in an infinite loop if the fuel gauge is nonresponsive
        }
        else   {
            // Log.info("verify model access unlocked: success");
            break;
        }
    }

    //TODO lots of blind writes / reads remaining here
    if (SYSTEM_ERROR_NONE == result_code) {
        // write RCOMP to its Maximum value (MAX17040/1/3/4 only) 
        writeRegister(CONFIG_REGISTER, 0xFF, 0x00);

        // write the model 
        writeBlock(MODEL_DATA_BLOCK,config_.model_data, BATTERY_MODEL_DATA_LENGTH);
        // {
        //     std::lock_guard<FuelGauge> l(*this);
        //     for(int i = 0; i < 64; i += 16)   { 
        //         i2c_.beginTransmission(MAX17043_ADDRESS);
        //         i2c_.write(0x40+i);
        //         for(int k = 0; k < 16; k++) {
        //             i2c_.write(_config.model_data[i+k]);
        //         }
        //         i2c_.endTransmission(true);
        //     }
        // }

        // delay at least 150ms (MAX17040/1/3/4 only) 
        delay(150);

        // write OCV 
        writeRegister(OCV_REGISTER, (config_.OCVTest >> 8) & 0xFF, config_.OCVTest & 0xFF);

        // delay between 150ms and 600ms 
        delay(150);

        // read SOC register and compare to expected result 
        byte soc1, soc2; 
        readRegister(SOC_REGISTER, soc1, soc2); 
        if(soc1 >= config_.SOCCheckA && soc1 <= config_.SOCCheckB)  { 
            // Log.info("load model successfully");
            result_code = -666;
        } 

        // restore CONFIG 
        //TODO does this trash CONFIG_REGISTER LSB??
        writeRegister(CONFIG_REGISTER, config_.RCOMP0, 0x00); 

        //restore OCV
        writeRegister(OCV_REGISTER, orig_OCV1, orig_OCV2);

    }

    // lock model access 
    writeRegister(TABLE_LOCK_REGISTER, 0x00, 0x00);

    // delay at least 150ms 
    delay(150);

    return result_code;
}


/**
 * @brief Verify the custom model stored in the fuel gauge RAM. If it's invalid, reload it.
 * @details Maxim ModelGauge devices store the custom model parameters in RAM. The RAM data can be 
 *  corrupted in the event of a power loss, brown out or ESD event. It is good practice to 
 *  occasionally verify the model and reload if necessary. Maxim recommends doing this once per 
 *  hour while the application is active.
 * @return int Error if the model in RAM is not valid (compared to that stored in _config)
 */
void FuelGauge::verifyModel() {
    byte orig_OCV1, orig_OCV2; 
    byte orig_RCOMP1, orig_RCOMP2; 
    byte soc1, soc2; 

    if (!config_.valid_config) {
        // no custom battery model
        return;
    }

    //unlock the TABLE registers
    writeRegister(TABLE_LOCK_REGISTER, 0x4A, 0x57); 

    // cache original RCOMP values
    readConfigRegister(orig_RCOMP1, orig_RCOMP2);
    // cache the original OCV values for later restoration
    readRegister(OCV_REGISTER, orig_OCV1, orig_OCV2);

    // write OCVTest value
    writeRegister(OCV_REGISTER, (config_.OCVTest >> 8) & 0xFF, config_.OCVTest & 0xFF);  
    // rewrite original RCOMP value
    writeRegister(CONFIG_REGISTER, orig_RCOMP1, orig_RCOMP2); 
    //This delay must be between 150mS and 600mS. Delaying beyond 600mS could cause the verification to fail.
    delay(150);

    //Read SOC Register and Compare to expected result
    readRegister(SOC_REGISTER, soc1, soc2);
    if(soc1 >= config_.SOCCheckA && soc1 <= config_.SOCCheckB) { 
        // Log.info("model verify success");
        // restore settings
        writeRegister(CONFIG_REGISTER, orig_RCOMP1, orig_RCOMP2); 
        writeRegister(OCV_REGISTER, orig_OCV1, orig_OCV2);   
    } 
    else  { 
        // Log.info("model verify failed, reload it");
        reloadModel();
    }
    
    //re-lock the TABLE registers
    writeRegister(TABLE_LOCK_REGISTER, 0x00, 0x00);
}




