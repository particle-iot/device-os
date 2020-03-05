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

test(SPI_1_SPI_Begin_Without_Argument)
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

test(SPI_2_SPI_Begin_With_Ss_Pin)
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

test(SPI_3_SPI_Begin_With_Mode)
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

test(SPI_4_SPI_Begin_With_Master_Ss_Pin)
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
#elif PLATFORM_ID == PLATFORM_BSOM
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
test(SPI_5_SPI_Begin_With_Slave_Ss_Pin)
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
test(SPI_6_SPI1_Begin_Without_Argument)
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

test(SPI_7_SPI1_Begin_With_Ss_Pin)
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

test(SPI_8_SPI1_Begin_With_Mode)
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

test(SPI_9_SPI1_Begin_With_Master_Ss_Pin)
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

