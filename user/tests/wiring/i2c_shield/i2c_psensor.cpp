/**
 ******************************************************************************
 * @file    i2c_psensor.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    10-Feb-2015
 * @brief   I2C test application
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

#include "application.h"
#include "unit-test/unit-test.h"

/*
 * I2C Test requires a custom Honeywell Pressure Sensor Shield with SSCMRNXXXXX IC
 *
 *     +-----------------------------------------------------------------+
 *     |                        Pin assignment                           |
 *     +---------------------------------------+-----------+-------------+
 *     |  Core/Photon Wire/I2C Pins            |  PSENSOR  |   Pin       |
 *     +---------------------------------------+-----------+-------------+
 *     | GND                                   |   GND     |    1 (0V)   |
 *     | 3V3                                   |   VDD     |    2 (3.3V) |
 *     | SDA                                   |   SDA     |    3        |
 *     | SCL                                   |   SCL     |    4        |
 *     | .                                     |   NC      |    5        |
 *     | .                                     |   NC      |    6        |
 *     | .                                     |   NC      |    7        |
 *     | .                                     |   NC      |    8        |
 *     +---------------------------------------+-----------+-------------+
 *
 */

//PSENSOR address
#define PSENSOR_ADDR    0x28

/**
 * @brief  Read pressure data of PSENSOR
 * @retval uint16_t PressureOutput
 */
uint16_t PSENSOR_ReadPressure(void)
{
    uint8_t PSENSOR_BufferRX[2] = {0,0}, index = 0;

    // request 2 bytes from pessure sensor
    Wire.requestFrom(PSENSOR_ADDR, 2);

    // slave may send less than requested
    while(Wire.available())
    {
        // receive a byte
        PSENSOR_BufferRX[index++] = (uint8_t)Wire.read();
        if(index == 2) break;
    }

    //return PSENSOR_I2C received bridge data
    return (uint16_t)(((PSENSOR_BufferRX[0] & 0x3F) << 8) | PSENSOR_BufferRX[1]);
}

test(I2C_ReadPressureSensorReturns_PSENSOR_OK)
{
    Wire.begin();

    //Read Pressure Sensor Data
    uint16_t pSensorOutput = PSENSOR_ReadPressure();

    //pSensorOutput should be a non-zero(~1627) result
    assertNotEqual(pSensorOutput, 0);
}
