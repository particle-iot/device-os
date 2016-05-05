/**
 ******************************************************************************
 * @file    flashupdate.cpp
 * @authors Satish Nair
 * @date    29 January 2015
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
#include "ota_flash_hal.h"
#include "dct.h"
#include "unit-test/unit-test.h"

test(FLASH_UPDATE_MODULES_Test_Passed)
{
    bool result = false;

    result = HAL_FLASH_AddToNextAvailableModulesSlot(FLASH_INTERNAL, 0x08020000,
                                                     FLASH_INTERNAL, 0x080C0000,
                                                     0x20000, 0, 0);

    assertEqual(result, true);

    if(result != false)
    {
        //Call HAL_FLASH_UpdateModules() to start the memory copy process
        HAL_FLASH_UpdateModules(NULL);//No callback needed so NULL

        //Compare internal flash memory data
        result = HAL_FLASH_CompareMemory(FLASH_INTERNAL, 0x08020000,
                                         FLASH_INTERNAL, 0x080C0000,
                                         0x20000);
    }

    assertEqual(result, true);
}
