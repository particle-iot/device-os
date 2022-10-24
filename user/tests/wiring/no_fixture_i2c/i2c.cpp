/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"

// SerialLogHandler g_logSerial(LOG_LEVEL_ALL);

static const int WIRE_ACQUIRE_BUFFER_SIZE = HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE1) + 1;
static const int WIRE1_ACQUIRE_BUFFER_SIZE = HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE2) + 1;
static const int TRACKER_MINIMUM_BUFFER_SIZE = 512;

static hal_i2c_config_t allocateWireConfig(uint32_t bufferSize) {
    hal_i2c_config_t config = {
        .size = sizeof(hal_i2c_config_t),
        .version = HAL_I2C_CONFIG_VERSION_2,
        .rx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .rx_buffer_size = bufferSize,
        .tx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .tx_buffer_size = bufferSize,
        .flags = HAL_I2C_CONFIG_FLAG_FREEABLE
    };
    return config;
}

static void freeWireBuffers(hal_i2c_config_t * config){
    free(config->tx_buffer);
    free(config->rx_buffer);
}

hal_i2c_config_t acquireWireBuffer()
{
    return allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE);
}

#if Wiring_Wire1
hal_i2c_config_t acquireWire1Buffer()
{
    return allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE);
}
#endif

test(I2C_00_hal_init_default_buffer)
{
    hal_i2c_lock(HAL_I2C_INTERFACE1, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(HAL_I2C_INTERFACE1, nullptr);
    });
    // Initializing the HAL with a null config is allowed and should allocate system defaults
    if (!hal_i2c_is_enabled(HAL_I2C_INTERFACE1, nullptr)) {
        assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, nullptr), (int)SYSTEM_ERROR_NONE);    
    } else {
        assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, nullptr), (int)SYSTEM_ERROR_INVALID_ARGUMENT);    
    }

#if (PLATFORM_ID == PLATFORM_TRACKER) || (PLATFORM_ID == PLATFORM_TRACKERM)
    // Tracker platforms should have buffers at least 512 bytes large, allocated by the system on startup
    // IE trying to re-initialize the HAL with same size should fail
    hal_i2c_config_t smallBuffers = allocateWireConfig(TRACKER_MINIMUM_BUFFER_SIZE);
    hal_i2c_interface_t largeBufferInterface = (PLATFORM_ID == PLATFORM_TRACKER) ? HAL_I2C_INTERFACE2 : HAL_I2C_INTERFACE1;
    hal_i2c_lock(largeBufferInterface, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(largeBufferInterface, nullptr);
    });

    assertEqual(hal_i2c_init(largeBufferInterface, &smallBuffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
    freeWireBuffers(&smallBuffers);
#endif

}

test(I2C_01_test_acquire_wire_buffer) {
    // Construct TwoWire object with user acquireWireBuffer() configuration structures, which should re-initialize the I2C HAL
    Wire.begin();
    hal_i2c_lock(HAL_I2C_INTERFACE1, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(HAL_I2C_INTERFACE1, nullptr);
    });

    // Attempt to re-initialize the HAL with different sized buffers to implicitly determine if the user buffers were used by the HAL
    // System Defaults < User values from acquireWireBuffer < Values used here to re-initialize the HAL

    // Equal size buffers should fail
    hal_i2c_config_t wireSmallerBuffers = allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, &wireSmallerBuffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
    freeWireBuffers(&wireSmallerBuffers);

    // Larger sized buffers should succeed
    hal_i2c_config_t wireLargerBuffers = allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE + 1);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, &wireLargerBuffers), (int)SYSTEM_ERROR_NONE);
    freeWireBuffers(&wireLargerBuffers);

    // Reinitializing with a NULL config is not allowed
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, nullptr), (int)SYSTEM_ERROR_INVALID_ARGUMENT);
}

#if Wiring_Wire1
test(I2C_02_test_acquire_wire1_buffer) {
    Wire1.begin();
    hal_i2c_lock(HAL_I2C_INTERFACE2, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(HAL_I2C_INTERFACE2, nullptr);
    });
    hal_i2c_config_t wire1SmallerBuffers = allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, &wire1SmallerBuffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
    freeWireBuffers(&wire1SmallerBuffers);

    hal_i2c_config_t wire1LargerBuffers = allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE + 1);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, &wire1LargerBuffers), (int)SYSTEM_ERROR_NONE);
    freeWireBuffers(&wire1LargerBuffers);

    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, nullptr), (int)SYSTEM_ERROR_INVALID_ARGUMENT);
}
#endif

#if HAL_PLATFORM_EXTERNAL_RTC
test(I2C_03_long_read_rtc)
{
    // Test > 32 byte reads by reading from AM18x5 RTC peripheral
    // Read beyond full register range
    const uint8_t REGISTER_RANGE = 64;
    uint8_t longReadBuffer[REGISTER_RANGE] = {};
    uint8_t readLength = REGISTER_RANGE; 

    // Check read values against ID registers
    const uint8_t ID0_REGISTER = 0x28;
    const uint8_t ID0_REGISTER_VALUE = 0x18;
    const uint8_t ID1_REGISTER = 0x29;
    const uint8_t ID1_REGISTER_VALUE = 0x05;

    hal_i2c_interface_t wireInterface = HAL_PLATFORM_EXTERNAL_RTC_I2C;
    uint8_t rtcAddress = HAL_PLATFORM_EXTERNAL_RTC_I2C_ADDR;
    uint8_t rtcRegister = 0x00;

    hal_i2c_begin(wireInterface, I2C_MODE_MASTER, 0x00, NULL);

    hal_i2c_lock(wireInterface, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(wireInterface, nullptr);
    });

    hal_i2c_begin_transmission(wireInterface, rtcAddress, NULL);
    hal_i2c_write(wireInterface, rtcRegister, NULL);
    hal_i2c_end_transmission(wireInterface, true, NULL);
    auto config = WireTransmission(rtcAddress).quantity(readLength).stop(true).halConfig();
    hal_i2c_request_ex(wireInterface, &config, nullptr);

    int32_t bytesAvailable = hal_i2c_available(wireInterface, NULL);
    assertEqual(bytesAvailable, readLength);

    for (int32_t i = 0; i < bytesAvailable; i++) {
        longReadBuffer[i] = (uint8_t)hal_i2c_read(wireInterface, NULL);
    }

    // Log.info("I2C_03_long_read_rtc result %d\n", bytesAvailable);
    // Log.dump(long_read_buffer, bytesAvailable);
    // Log.info(" \n");

    assertEqual(longReadBuffer[ID0_REGISTER], ID0_REGISTER_VALUE);
    assertEqual(longReadBuffer[ID1_REGISTER], ID1_REGISTER_VALUE);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC

#if (HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL == 0) && (HAL_PLATFORM_PMIC_BQ24195)

test(I2C_04_long_read_pmic)
{
    // Test > 32 byte reads by reading from BQ24195 PMIC peripheral. 
    // The device only has 10 registers to read, but should not NAK reads for more data

    // Read beyond full register range
    const uint8_t REGISTER_RANGE = 34;
    uint8_t longReadBuffer[REGISTER_RANGE] = {};
    uint8_t readLength = REGISTER_RANGE; 

    // Check read values against ID register
    const uint8_t ID_REGISTER = 0x0A;
    const uint8_t ID_REGISTER_VALUE = 0x23;

    hal_i2c_interface_t wireInterface = HAL_PLATFORM_PMIC_BQ24195_I2C;
    uint8_t pmicAddress = PMIC_ADDRESS;
    uint8_t pmicRegister = 0x00;

    hal_i2c_lock(wireInterface, nullptr);
    SCOPE_GUARD({
        hal_i2c_unlock(wireInterface, nullptr);
    });

    hal_i2c_begin(wireInterface, I2C_MODE_MASTER, 0x00, NULL);
    assertTrue(hal_i2c_is_enabled(wireInterface, nullptr));

    hal_i2c_begin_transmission(wireInterface, pmicAddress, NULL);
    hal_i2c_write(wireInterface, pmicRegister, NULL);
    hal_i2c_end_transmission(wireInterface, true, NULL);
    auto config = WireTransmission(pmicAddress).quantity(readLength).stop(true).halConfig();
    hal_i2c_request_ex(wireInterface, &config, nullptr);

    int32_t bytesAvailable = hal_i2c_available(wireInterface, NULL);
    assertEqual(bytesAvailable, readLength);

    for (int32_t i = 0; i < bytesAvailable; i++) {
        longReadBuffer[i] = (uint8_t)hal_i2c_read(wireInterface, NULL);
    }

    // Log.info("I2C_04_long_read_pmic result %ld\n", bytesAvailable);
    // Log.dump(longReadBuffer, bytesAvailable);
    // Log.info(" \n");

    assertEqual(longReadBuffer[ID_REGISTER], ID_REGISTER_VALUE);
}

#endif // HAL_PLATFORM_PMIC_BQ24195
