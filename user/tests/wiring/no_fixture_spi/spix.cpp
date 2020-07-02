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

static void querySpiInfo(HAL_SPI_Interface spi, hal_spi_info_t* info)
{
    memset(info, 0, sizeof(hal_spi_info_t));
    info->version = HAL_SPI_INFO_VERSION;
    HAL_SPI_Info(spi, info, nullptr);
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
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#elif PLATFORM_ID == PLATFORM_TRACKER
    assertEqual(info.ss_pin, D7);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
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
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#elif PLATFORM_ID == PLATFORM_TRACKER
    assertEqual(info.ss_pin, D7);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
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
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin,D8);
#elif PLATFORM_ID == PLATFORM_TRACKER
    assertEqual(info.ss_pin, D7);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    // HAL_SPI_INTERFACE1 does not support slave mode on Gen3 device
#if HAL_PLATFORM_STM32F2XX
    SPI.begin(SPI_MODE_SLAVE);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#elif PLATFORM_ID == PLATFORM_TRACKER
    assertEqual(info.ss_pin, D7);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
    SPI.end();
#endif // HAL_PLATFORM_STM32F2XX
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
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#elif PLATFORM_ID == PLATFORM_TRACKER
    assertEqual(info.ss_pin, D7);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
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

// HAL_SPI_INTERFACE1 does not support slave mode on Gen3 device
#if HAL_PLATFORM_STM32F2XX
test(SPIX_05_SPI_Begin_With_Slave_Ss_Pin)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_MODE_SLAVE, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
    assertEqual(info.ss_pin, A2);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_SLAVE, D0);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
    assertEqual(info.ss_pin, D0);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_SLAVE, PIN_INVALID);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertFalse(info.enabled);
    SPI.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI.begin(SPI_MODE_SLAVE, 123);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertFalse(info.enabled);
    SPI.end();
}
#endif // HAL_PLATFORM_STM32F2XX

#if Wiring_SPI1
test(SPIX_06_SPI1_Begin_Without_Argument)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin();
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
    SPI1.end();
}

test(SPIX_07_SPI1_Begin_With_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
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

test(SPIX_08_SPI1_Begin_With_Mode)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_MODE_MASTER);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_SLAVE);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
    SPI1.end();
}

test(SPIX_09_SPI1_Begin_With_Master_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_MODE_MASTER, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
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

test(SPIX_10_SPI1_Begin_With_Slave_Ss_Pin)
{
    // Just in case
    SPI1.end();

    hal_spi_info_t info = {};

    SPI1.begin(SPI_MODE_SLAVE, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
    // D5 is the default SS pin for all platforms
    assertEqual(info.ss_pin, D5);
    SPI1.end();

    memset(&info, 0x00, sizeof(hal_spi_info_t));

    SPI1.begin(SPI_MODE_SLAVE, D0);
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_SLAVE);
    assertEqual(info.ss_pin, D0);
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
    constexpr uint64_t spiTransferTime = (nanosInSecond * transferSize * 8 + clockSpeed - 1) / clockSpeed;
    constexpr uint64_t expectedTransferTime = spiTransferTime + overheadNs;

    return (expectedTransferTime * iterations) / microsInSecond;
}

// Common settings for all performance tests
constexpr unsigned int SPI_ITERATIONS = 10000;

#if HAL_PLATFORM_NRF52840
constexpr unsigned int SPI_CLOCK_SPEED = 8000000; // 8MHz
constexpr unsigned int SPI_NODMA_OVERHEAD = 12500; // 12us ~= 780 clock cycles @ 64MHz
constexpr unsigned int SPI_DMA_OVERHEAD = SPI_NODMA_OVERHEAD; // Gen 3 always uses DMA underneath
#elif HAL_PLATFORM_STM32F2XX
constexpr unsigned int SPI_CLOCK_SPEED = 7500000; // 7.5MHz
constexpr unsigned int SPI_NODMA_OVERHEAD = 1600; // 1.6us ~= 190 clock cycles @ 120MHz
constexpr unsigned int SPI_DMA_OVERHEAD = 11500; // 11.5us ~= 1380 clock cycles @ 120MHz
#endif // HAL_PLATFORM_NRF52840

} // anonymous

test(SPIX_11_SPI_Clock_Speed)
{
    SPI.begin();
    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    hal_spi_info_t info = {};
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    SPI.end();

    assertEqual(info.clock, SPI_CLOCK_SPEED);
}

test(SPIX_12_SPI_Transfer_1_Bytes_Per_Transmission_No_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(0x55);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_13_SPI_Transfer_1_Bytes_Per_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(0x55);
    }
    SPI.endTransaction();
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_14_SPI_Transfer_2_Bytes_Per_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 2;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_NODMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i+=2)
    {
        SPI.transfer(0x55);
        SPI.transfer(0x55);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_15_SPI_Transfer_1_Bytes_Per_DMA_Transmission_No_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    uint8_t temp = 0x55;
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(&temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_16_SPI_Transfer_1_Bytes_Per_DMA_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 1;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp = 0x55;
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(&temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_17_SPI_Transfer_2_Bytes_Per_DMA_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 2;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_18_SPI_Transfer_16_Bytes_Per_DMA_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 16;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_19_SPI_Transfer_128_Bytes_Per_DMA_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 128;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}

test(SPIX_20_SPI_Transfer_1024_Bytes_Per_DMA_Transmission_Locking)
{
    SINGLE_THREADED_SECTION();
    constexpr unsigned int transferSize = 1024;
    constexpr auto expectedTime = calculateExpectedTime<SPI_CLOCK_SPEED, transferSize, SPI_DMA_OVERHEAD, SPI_ITERATIONS>();

    SPI.setClockSpeed(SPI_CLOCK_SPEED);
    SPI.begin();
    SPI.beginTransaction();
    uint8_t temp[transferSize] = {};
    system_tick_t start = DWT->CYCCNT;
    for(unsigned int i = 0; i < SPI_ITERATIONS; i++)
    {
        SPI.transfer(temp, nullptr, transferSize, nullptr);
    }
    system_tick_t transferTime = (DWT->CYCCNT - start) / SYSTEM_US_TICKS / 1000;
    SPI.endTransaction();
    SPI.end();

    // Serial.printlnf("in %lu ms, expected: %lu", transferTime, expectedTime);
    assertLessOrEqual(transferTime, expectedTime);
}
