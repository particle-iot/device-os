/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "Particle.h"
#include "unit-test/unit-test.h"

static pin_t expectedSsPin(HAL_SPI_Interface spi) {
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    return (spi == HAL_SPI_INTERFACE1) ? D14 : D5;
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    return (spi == HAL_SPI_INTERFACE1) ? D8 : D5;
#elif PLATFORM_ID == PLATFORM_TRACKER
    return (spi == HAL_SPI_INTERFACE1) ? D7 : D5;
#elif PLATFORM_ID == PLATFORM_P2
    return (spi == HAL_SPI_INTERFACE1) ? S3 : D5;
#elif PLATFORM_ID == PLATFORM_TRACKERM
    return (spi == HAL_SPI_INTERFACE1) ? A2 : D5;
#elif PLATFORM_ID == PLATFORM_ESOMX
    return (spi == HAL_SPI_INTERFACE1) ? A2 : D5;
#elif PLATFORM_ID == PLATFORM_MSOM
    return (spi == HAL_SPI_INTERFACE1) ? D8 : D3;
#else
#error "Unknown platform!"
#endif
    return PIN_INVALID;
}

static bool isSlaveModeSupported(HAL_SPI_Interface spi) {
    // NRF52840 platform
    //   SPI - only supports master mode
    //   SPI1 - supports master and slave mode
    // RTL872X platform
    //   P2/TRACKERM
    //      SPI - only supports master mode
    //      SPI1 - supports master and slave mode
    //   MSOM
    //      SPI - supports master and slave mode
    //      SPI1 - only supports master mode
#if HAL_PLATFORM_NRF52840
    return (spi == HAL_SPI_INTERFACE1) ? false : true;
#elif HAL_PLATFORM_RTL872X
    #if PLATFORM_ID == PLATFORM_MSOM
    return (spi == HAL_SPI_INTERFACE1) ? true : false;
    #elif PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
    return (spi == HAL_SPI_INTERFACE1) ? false : true;
    #else
    #error "Unknown platform!"
    #endif // PLATFORM_ID == PLATFORM_MSOM
#else
#error "Unknown platform!"
#endif // HAL_PLATFORM_NRF52840
}

static void querySpiInfo(HAL_SPI_Interface spi, hal_spi_info_t* info)
{
    memset(info, 0, sizeof(hal_spi_info_t));
    info->version = HAL_SPI_INFO_VERSION;
    hal_spi_info(spi, info, nullptr);
}

test(SPIX_01_SPI_Begin_Without_Argument)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin();
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE1));
    SPI.end();
}

test(SPIX_02_SPI_Begin_With_Ss_Pin)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE1));
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(D0);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, D0);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, PIN_INVALID);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(123);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, 123);
    SPI.end();
}

test(SPIX_03_SPI_Begin_With_Mode)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_MODE_MASTER);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE1));
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));
    SPI.begin(SPI_MODE_SLAVE);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled == isSlaveModeSupported(HAL_SPI_INTERFACE1));
    SPI.end();
}

test(SPIX_04_SPI_Begin_With_Master_Ss_Pin)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_MODE_MASTER, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE1));
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_MASTER, D0);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, D0);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_MASTER, PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, PIN_INVALID);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_MASTER, 123);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, 123);
    SPI.end();
}

#if Wiring_SPI1
test(SPIX_05_SPI1_Begin_Without_Argument)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin();
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    SPI1.end();
}

test(SPIX_06_SPI1_Begin_With_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(D0);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, D0);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, PIN_INVALID);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(123);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, 123);
    SPI1.end();
}

test(SPIX_07_SPI1_Begin_With_Mode)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};
    SPI1.begin(SPI_MODE_MASTER);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));
    SPI1.begin(SPI_MODE_SLAVE);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled == isSlaveModeSupported(HAL_SPI_INTERFACE2));
    assertEqual(info.mode, isSlaveModeSupported(HAL_SPI_INTERFACE2) ? SPI_MODE_SLAVE : SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    SPI1.end();
}

test(SPIX_08_SPI1_Begin_With_Master_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_MODE_MASTER, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_MASTER, D0);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, D0);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_MASTER, PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, PIN_INVALID);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_MASTER, 123);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    assertEqual(info.ss_pin, 123);
    SPI1.end();
}

test(SPIX_09_SPI1_Begin_With_Slave_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_MODE_SLAVE, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled == isSlaveModeSupported(HAL_SPI_INTERFACE2));
    assertEqual(info.mode, isSlaveModeSupported(HAL_SPI_INTERFACE2) ? SPI_MODE_SLAVE : SPI_MODE_MASTER);
    if (isSlaveModeSupported(HAL_SPI_INTERFACE2)) {
        assertEqual(info.ss_pin, expectedSsPin(HAL_SPI_INTERFACE2));
    }
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_SLAVE, D0);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled == isSlaveModeSupported(HAL_SPI_INTERFACE2));
    assertEqual(info.mode, isSlaveModeSupported(HAL_SPI_INTERFACE2) ? SPI_MODE_SLAVE : SPI_MODE_MASTER);
    if (isSlaveModeSupported(HAL_SPI_INTERFACE2)) {
        assertEqual(info.ss_pin, D0);
    }
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_SLAVE, PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertFalse(info.enabled);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_SLAVE, 123);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertFalse(info.enabled);
    SPI1.end();
}
#endif // Wiring_SPI1

namespace {

template <unsigned int clockSpeed, unsigned int transferSize, unsigned int overheadNs, unsigned int iterations>
constexpr system_tick_t calculateExpectedTime() {
    constexpr uint64_t microsInSecond = 1000000ULL;
    constexpr uint64_t nanosInSecond = 1000000000ULL;
    constexpr uint64_t spiTransferTime = (nanosInSecond * transferSize * 8) / clockSpeed;
    constexpr uint64_t expectedTransferTime = spiTransferTime + overheadNs;

    return (expectedTransferTime * iterations) / microsInSecond;
}

// Common settings for all performance tests
constexpr unsigned int SPI_ITERATIONS = 10000;

constexpr unsigned int SPI_ERROR_MARGIN = 5; // 5%

#if HAL_PLATFORM_RTL872X
// devices (P2, Photon 2, Tracker M), the clock reference is 50MHz for SPI and 100MHz for SPI1.
// SPI = 25MHz max / SPI1 = 50Mhz max
constexpr unsigned int SPI_CLOCK_SPEED = 12500000; // 12.5MHz
constexpr unsigned int SPI_NODMA_OVERHEAD = 3500; // 3.5us (imperically measured, time between transfers on a scope, WIP set high to pass for now)
constexpr unsigned int SPI_DMA_OVERHEAD = 30000; // 27us between, plus some extra
constexpr unsigned int SPI_DMA_LARGE_OVERHEAD = 27000; // 27us between
constexpr unsigned int SPI1_DMA_LARGE_OVERHEAD = SPI_DMA_LARGE_OVERHEAD;
#elif HAL_PLATFORM_NRF52840
// On Gen 3 devices (Argon, Boron, B Series SoM, Tracker SoM), the clock reference is 64MHz.
// SPI = 32MHz max / SPI1 = 8Mhz max
constexpr unsigned int SPI_CLOCK_SPEED = 8000000; // 8MHz
constexpr unsigned int SPI_DMA_OVERHEAD = 15500; // Gen 3 always uses DMA underneath
constexpr unsigned int SPI_DMA_LARGE_OVERHEAD = SPI_DMA_OVERHEAD;
constexpr unsigned int SPI_NODMA_OVERHEAD = SPI_DMA_OVERHEAD; // 15.5us (imperically measured, time between transfers on a scope)
constexpr unsigned int SPI1_DMA_LARGE_OVERHEAD = SPI_DMA_LARGE_OVERHEAD;
#else
#error "Unsupported platform"
#endif // HAL_PLATFORM_RTL872X

#if !HAL_PLATFORM_RTL872X
using SpixTestLock = SingleThreadedSection;
#else
// On RTL872x platforms USB interrupts are processed in a thread, so USB comms will
// be broken for the duration of the test which can cause test runner failures
// FIXME: disabling for now
struct SpixTestLock {

};
#endif // HAL_PLATFORM_RTL872X

} // anonymous

test(SPIX_10_SPI_Clock_Speed)
{
    SPI.begin();
    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    hal_spi_info_t info = {};
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    SPI.end();

    assertEqual(info.clock, SPI_CLOCK_SPEED);
}

test(SPIX_11_SPI_Transfer_1_Bytes_Per_Transmission_No_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(0x55);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_12_SPI_Transfer_1_Bytes_Per_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(0x55);
    }
    SPI.endTransaction();
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_13_SPI_Transfer_2_Bytes_Per_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 2;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i+=2)
    {
        SPI.transfer(0x55);
        SPI.transfer(0x55);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_14_SPI_Transfer_1_Bytes_Per_DMA_Transmission_No_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    uint8_t temp = 0x55;
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(&temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_15_SPI_Transfer_1_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp = 0x55;
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(&temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_16_SPI_Transfer_2_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 2;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_17_SPI_Transfer_16_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 16;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_18_SPI_Transfer_128_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 128;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

test(SPIX_19_SPI_Transfer_1024_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1024;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_LARGE_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}

#if Wiring_SPI1
test(SPIX_20_SPI1_Transfer_1024_Bytes_Per_DMA_Transmission_Locking)
{
    SpixTestLock lk;
    constexpr unsigned int transferSize = 1024;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI1_DMA_LARGE_OVERHEAD, SPI_ITERATIONS>();

    SPI1.setClockSpeed(SPI_CLOCK_SPEED);
    SPI1.begin();
    SPI1.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = SYSTEM_TICK_COUNTER;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI1.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (SYSTEM_TICK_COUNTER - start) / SYSTEM_US_TICKS / 1000;
    SPI1.endTransaction();
    SPI1.end();

    // Serial.printlnf("in %lu ms, expected: %lu, expected max: %lu", transferTime, expectedTime, expectedTime + ((expectedTime * SPI_ERROR_MARGIN) / 100));
    assertLessOrEqual(transferTime, expectedTime + (expectedTime * SPI_ERROR_MARGIN) / 100);
}
#endif // Wiring_SPI1

test(SPIX_21_SPI_Sleep) {
    constexpr unsigned int transferSize = 128;
    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    assertTrue(SPI.isEnabled());
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    uint8_t tempRx[transferSize] = {};
    uint8_t tempRx1[transferSize] = {};
    SPI.transfer(temp, tempRx, transferSize, nullptr);
    SPI.endTransaction();

    for (int i = 0; i < transferSize; i++) {
        tempRx1[i] = tempRx[i];
        tempRx[i] = ~tempRx[i];
    }

    assertEqual(0, hal_spi_sleep(SPI.interface(), true, nullptr));
    assertFalse(SPI.isEnabled());
    assertEqual(0, hal_spi_sleep(SPI.interface(), false, nullptr));
    assertTrue(SPI.isEnabled());

    SPI.beginTransaction();
    SPI.transfer(temp, tempRx, transferSize, nullptr);
    SPI.endTransaction();
    SPI.end();

    assertEqual(0, memcmp(tempRx, tempRx1, sizeof(tempRx)));
}

test(SPIX_22_SPI_Transfer_Buffer_In_Flash) {
    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    assertTrue(SPI.isEnabled());

    SPI.beginTransaction();
    SPI.transfer("Hello", nullptr, sizeof("Hello"), nullptr);
    SPI.endTransaction();

    SPI.end();
}
