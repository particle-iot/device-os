/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#define PARTICLE_USE_UNSTABLE_API
#include "storage_hal.h"

#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#if PLATFORM_ID == PLATFORM_MSOM

const uintptr_t OTP_TEST_ADDRESS = 0x100;
const uint8_t TEST_BYTE = 0xA5;

const unsigned LONG_TEST_LENGTH = 65;
uint8_t LONG_TEST_PATTERN[LONG_TEST_LENGTH] = {};

static uint8_t otpBuffer[1024] = {};
int read_and_log_otp_bytes(uintptr_t addr, int length) {
    // For debugging
    memset(otpBuffer, 0x00, sizeof(otpBuffer));

    if (length > (int)sizeof(otpBuffer)) {
        return -1;
    }

    int r = hal_storage_read(HAL_STORAGE_ID_OTP, addr, otpBuffer, length);
    Log.info("Read from addr 0x%02X, length %d result = %d", addr, length, r);
    LOG_DUMP(TRACE, otpBuffer, length);
    LOG_PRINT(TRACE, "\r\n");
    return 0;
}

test(FLASH_OTP_00_SETUP)
{
    // Generate basic test pattern
    for (unsigned i = 0; i < sizeof(LONG_TEST_PATTERN); i++) {
        LONG_TEST_PATTERN[i] = i;
    }

    // Ensure all security registers are erased for subsequent tests
    system_internal(4, (void*)(1024 * 0));
    system_internal(4, (void*)(1024 * 1));
    system_internal(4, (void*)(1024 * 2));
}

test(FLASH_OTP_01_SHORT_WRITE)
{
    int length = sizeof(TEST_BYTE);
    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS, &TEST_BYTE, length), length);
}

test(FLASH_OTP_02_LONG_WRITE) {
    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS+1, LONG_TEST_PATTERN, LONG_TEST_LENGTH), LONG_TEST_LENGTH);   
}

test(FLASH_OTP_03_SHORT_READ)
{
    int length = sizeof(TEST_BYTE);
    uint8_t readValue = 0;

    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS, &readValue, length), length);
    assertEqual(TEST_BYTE, readValue);
    read_and_log_otp_bytes(OTP_TEST_ADDRESS, length);
}

test(FLASH_OTP_04_LONG_READ)
{
    read_and_log_otp_bytes(OTP_TEST_ADDRESS+1, LONG_TEST_LENGTH);

    uint8_t readTestValues[LONG_TEST_LENGTH] = {};

    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS+1, readTestValues, LONG_TEST_LENGTH), LONG_TEST_LENGTH);
    for (unsigned i = 0; i < sizeof(readTestValues); i++) {
        assertEqual(LONG_TEST_PATTERN[i], readTestValues[i]);
    }
}

test(FLASH_OTP_05_WRITE_ACROSS_REGISTER_BOUNDARY) {
    uintptr_t boundaryAddress = 1024 - LONG_TEST_LENGTH;
    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, boundaryAddress, LONG_TEST_PATTERN, LONG_TEST_LENGTH), LONG_TEST_LENGTH);
}

test(FLASH_OTP_06_READ_ACROSS_REGISTER_BOUNDARY) {
    uintptr_t boundaryAddress = 1024 - LONG_TEST_LENGTH;
    uint8_t readTestValues[LONG_TEST_LENGTH] = {};

    read_and_log_otp_bytes(boundaryAddress, LONG_TEST_LENGTH);

    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, boundaryAddress, readTestValues, LONG_TEST_LENGTH), LONG_TEST_LENGTH);

    for (unsigned i = 0; i < sizeof(readTestValues); i++) {
        assertEqual(LONG_TEST_PATTERN[i], readTestValues[i]);
    }
}

test(FLASH_OTP_07_OVERWRITE_DATA)
{
    uint8_t testByteFlipped = 0xA0;
    int length = sizeof(testByteFlipped);

    uint8_t readBackByte = 0xFF;

    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS, &testByteFlipped, length), length);
    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, OTP_TEST_ADDRESS, &readBackByte, length), length);
    assertEqual(readBackByte, 0xA0);
}

test(FLASH_OTP_08_SECTOR_ERASE)
{   
    // Write two bytes across register boundaries
    uint8_t sectorTwoTestByte = 0xAA;
    uint8_t sectorThreeTestByte = 0xBB;
    uint8_t readBackByte = 0xFF;

    int length = sizeof(sectorTwoTestByte);

    uintptr_t boundaryAddress = (2 * 1024) - 1;

    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, boundaryAddress, &sectorTwoTestByte, length), length);
    assertEqual(hal_storage_write(HAL_STORAGE_ID_OTP, boundaryAddress+1, &sectorThreeTestByte, length), length);

    // Confirm writes
    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, boundaryAddress, &readBackByte, length), length);
    assertEqual(readBackByte, 0xAA);
    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, boundaryAddress+1, &readBackByte, length), length);
    assertEqual(readBackByte, 0xBB);

    // Erase sector 2
    system_internal(4, (void*)(boundaryAddress-2));

    // Byte in sector 2 should be erased
    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, boundaryAddress, &readBackByte, length), length);
    assertEqual(readBackByte, 0xFF);

    // Byte in Sector 3 byte should NOT be erased
    assertEqual(hal_storage_read(HAL_STORAGE_ID_OTP, boundaryAddress+1, &readBackByte, length), length);
    assertEqual(readBackByte, 0xBB);
}


#endif
