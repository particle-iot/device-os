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

test(SPI_01_SPI_Begin_Without_Argument)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin();
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
    SPI.end();
}

test(SPI_02_SPI_Begin_With_Ss_Pin)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
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

test(SPI_03_SPI_Begin_With_Mode)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_MODE_MASTER);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin,D8);
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
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
#else // Photon, P1 and Electron
    assertEqual(info.ss_pin, A2);
#endif
    SPI.end();
#endif // HAL_PLATFORM_STM32F2XX
}

test(SPI_04_SPI_Begin_With_Master_Ss_Pin)
{
    // Just in case
    SPI.end();

    hal_spi_info_t info = {};

    SPI.begin(SPI_MODE_MASTER, SPI_DEFAULT_SS);
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    assertTrue(info.enabled);
    assertEqual(info.mode, SPI_MODE_MASTER);
#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON
    assertEqual(info.ss_pin, D14);
#elif PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
    assertEqual(info.ss_pin, D8);
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
test(SPI_05_SPI_Begin_With_Slave_Ss_Pin)
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
test(SPI_06_SPI1_Begin_Without_Argument)
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

test(SPI_07_SPI1_Begin_With_Ss_Pin)
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

test(SPI_08_SPI1_Begin_With_Mode)
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

test(SPI_09_SPI1_Begin_With_Master_Ss_Pin)
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

test(SPI_10_SPI1_Begin_With_Slave_Ss_Pin)
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

// Performance tests. All SPI interfaces share the same driver. Testing on SPI only is sufficient.
test(SPI_11_SPI_Transfer_10000_Bytes_No_Locking_Less_Than_135_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    system_tick_t start = millis();
    for(unsigned int i=0; i < 10000; i++)
    {
        SPI.transfer(0x55);
    }
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 135);
    SPI1.end();
}

test(SPI_12_SPI_Transfer_10000_Bytes_Locking_Less_Than_135_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = millis();
    for(unsigned int i=0; i < 10000; i++)
    {
        SPI.transfer(0x55);
    }
    SPI.endTransaction();
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 135);
    SPI1.end();
}

test(SPI_13_SPI_Transfer_2_Miltiply_5000_Bytes_Locking_Less_Than_135_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = millis();
    for(unsigned int i=0; i < 5000; i++)
    {
        SPI.transfer(0x55);
        SPI.transfer(0x55);
    }
    SPI.endTransaction();
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 135);
    SPI1.end();
}

test(SPI_14_SPI_Transfer_10000_Bytes_DMA_No_Locking_Less_Than_140_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    system_tick_t start = millis();
    uint8_t temp = 0x55;
    for(unsigned int i=0; i < 10000; i++)
    {
        SPI.transfer(&temp, nullptr, 1, nullptr); 
    }
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 140);
    SPI1.end();
}

test(SPI_15_SPI_Transfer_10000_Bytes_DMA_Locking_Less_Than_140_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = millis();
    uint8_t temp = 0x55;
    for(unsigned int i=0; i < 10000; i++)
    {
        SPI.transfer(&temp, nullptr, 1, nullptr); 
    }
    SPI.endTransaction();
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 140);
    SPI1.end();
}

test(SPI_16_SPI_Transfer_2_Multiply_5000_Bytes_DMA_Locking_Less_Than_85_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = millis();
    uint8_t temp[2] = {0x55};
    for(unsigned int i=0; i < 5000; i++)
    {
        SPI.transfer(&temp, nullptr, 2, nullptr); 
    }
    SPI.endTransaction();
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 85);
    SPI1.end();
}

test(SPI_17_SPI_Transfer_10_Multiply_1000_Bytes_DMA_Locking_Less_Than_40_Ms)
{
    SPI.setClockSpeed(5, MHZ);
    SPI.begin();
    SPI.beginTransaction();
    system_tick_t start = millis();
    uint8_t temp[10] = {0x55};
    for(unsigned int i=0; i < 1000; i++)
    {
        SPI.transfer(&temp, nullptr, 10, nullptr); 
    }
    SPI.endTransaction();
    Serial.printf("%d ms\r\n", millis() - start);
    assertTrue(millis() - start < 40);
    SPI1.end();
}

