/**
 ******************************************************************************
 * @file    hw_modules.cpp
 * @authors Matthew McGowan, Satish Nair
 * @date    27 January 2015
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

#include "hw_config.h"
#include "dct.h"


//This function will be called in bootloader to perform the memory update process
void FLASH_Update_Modules(void)
{
    platform_flash_modules_t flash_modules[FLASH_MODULES_MAX];
    uint8_t flash_module_count = 0;
    uint8_t updateStatusFlags = 0;

    //Fill the Flash modules info data read from the dct area
    const void* dct_app_data = dct_read_app_data(DCT_FLASH_MODULES_OFFSET);
    memcpy(flash_modules, dct_app_data, DCT_FLASH_MODULES_SIZE);

    for (flash_module_count = 0; flash_module_count < FLASH_MODULES_MAX; flash_module_count++)
    {
        if(flash_modules[flash_module_count].statusFlag == 0x1)
        {
            //To Do : Copy flash data from source to destination based on MAL ID

            updateStatusFlags |= flash_modules[flash_module_count].statusFlag;
            flash_modules[flash_module_count].statusFlag = 0; //Reset statusFlag
        }
    }

    if(updateStatusFlags)
    {
        //Only update DCT if required
        dct_write_app_data(flash_modules, DCT_FLASH_MODULES_OFFSET, DCT_FLASH_MODULES_SIZE);
    }
}

