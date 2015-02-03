/**
 ******************************************************************************
 * @file    flashupdate.cpp
 * @authors Satish Nair
 * @date    29 January 2015
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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
#include "ota_flash_hal.h"
#include "dct.h"
#include "unit-test/unit-test.h"

test(FLASH_UPDATE_MODULES_Test_Passed)
{
    bool compareResult = false;

    //Fill up application_dct=>flash_modules instances
    platform_flash_modules_t flash_modules[5];
    flash_modules[0].magicNumber = 0xABCD;
    flash_modules[0].sourceDeviceID = FLASH_INTERNAL;
    flash_modules[0].destinationDeviceID = FLASH_INTERNAL;
    flash_modules[0].sourceAddress = 0x08020000;
    flash_modules[0].destinationAddress = 0x080C0000;
    flash_modules[0].length = 0x20000;
    dct_write_app_data(flash_modules, DCT_FLASH_MODULES_OFFSET, sizeof(flash_modules));

    //Call HAL_FLASH_UpdateModules() to start the memory copy process
    HAL_FLASH_UpdateModules();

    //Compare internal flash memory data
    compareResult = HAL_FLASH_CompareMemory(flash_modules[0].sourceDeviceID,
                                            flash_modules[0].sourceAddress,
                                            flash_modules[0].destinationDeviceID,
                                            flash_modules[0].destinationAddress,
                                            flash_modules[0].length);

    assertEqual(compareResult, true);
}
