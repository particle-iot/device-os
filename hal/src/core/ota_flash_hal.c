/**
 ******************************************************************************
 * @file    ota_flash_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "ota_flash_hal.h"
#include "hw_config.h"
#include <string.h>
#include "parse_server_address.h"

void HAL_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    info->platform_id = PLATFORM_ID;
    info->module_count = 0;
    info->modules = NULL;
}

uint32_t HAL_OTA_FlashAddress()
{
    return EXTERNAL_FLASH_OTA_ADDRESS;
}

#define FLASH_MAX_SIZE          (int32_t)(INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS)
#define OTA_CHUNK_SIZE          512

uint32_t HAL_OTA_FlashLength()
{
    return FLASH_MAX_SIZE;
}

uint16_t HAL_OTA_ChunkSize()
{
    return OTA_CHUNK_SIZE;
}

bool HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize, void* reserved)
{
    FLASH_Begin(sFLASH_Address, fileSize);
    return true;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize,  void* reserved)
{
    return FLASH_Update(pBuffer, address, bufferSize);
}

int HAL_FLASH_OTA_Validate(hal_module_t* mod, bool userDepsOptional, module_validation_flags_t flags, void* reserved)
{
  return 0;
}

hal_update_complete_t HAL_FLASH_End(hal_module_t* reserved)
{
    FLASH_End();
    return HAL_UPDATE_APPLIED_PENDING_RESTART;
}

hal_update_complete_t HAL_FLASH_ApplyPendingUpdate(hal_module_t* module, bool dryRun, void* reserved)
{
    // Not implemented for Core
    return HAL_UPDATE_ERROR;
}

void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
    uint8_t buf[EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH];
    FLASH_Read_ServerAddress_Data(buf);
    parseServerAddressData(server_addr, buf, EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH);
}

void HAL_FLASH_Write_ServerAddress(const uint8_t *buf, bool udp)
{
    FLASH_Write_ServerAddress_Data(buf);
}

uint32_t HAL_OTA_Flashed_Length(void)
{
    return 0;
}

bool HAL_OTA_Flashed_GetStatus(void)
{
    return OTA_Flashed_GetStatus();
}

void HAL_OTA_Flashed_ResetStatus(void)
{
    OTA_Flashed_ResetStatus();
}

void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    FLASH_Read_ServerPublicKey(keyBuffer);
}


void HAL_FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer, bool udp)
{
    FLASH_Write_ServerPublicKey(keyBuffer);
}

int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* genspec)
{
    FLASH_Read_CorePrivateKey(keyBuffer);
    genspec->generated_key = false;
    genspec->had_key = *keyBuffer!=0xFF;
    return 0;
}

uint16_t HAL_Set_Claim_Code(const char* code)
{
    return -1;
}

uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len)
{
    if (len)
        buffer[0] = 0;
    return 0;
}

bool HAL_IsDeviceClaimed(void* reserved)
{
	return false;
}

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
}
