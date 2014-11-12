/**
 ******************************************************************************
 * @file    ota_flash_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#if USE_WICED_SDK==1
#include "ota_flash_hal.h"

uint32_t HAL_OTA_FlashAddress()
{
    return 0;
}

uint32_t HAL_OTA_FlashLength()
{
    return 0;
}
    

void HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize) 
{
}

uint16_t HAL_FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize) 
{
    return 0;
}

void HAL_FLASH_End(void) 
{    
}

void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
}


bool HAL_OTA_Flashed_GetStatus(void) 
{
    return false;
}

void HAL_OTA_Flashed_ResetStatus(void)
{    
}

void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    // fetch this from dct data
}

void HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{ 
    // fetch from dct data
}
#else

#include "../template/ota_flash_hal.c"

#endif


