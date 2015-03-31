/**
 ******************************************************************************
 * @file    ota_flash_hal.c
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

#include "ota_flash_hal.h"
#include "hw_config.h"
#include <string.h>

bool HAL_OTA_CheckValidAddressRange(uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (startAddress == EXTERNAL_FLASH_OTA_ADDRESS && endAddress < EXTERNAL_FLASH_USER_ADDRESS)
    {
        return true;
    }
    else if (startAddress >= EXTERNAL_FLASH_USER_ADDRESS && endAddress < 0x200000)
    {
        return true;
    }

    return false;
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

bool HAL_FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                          flash_device_t destinationDeviceID, uint32_t destinationAddress,
                          uint32_t length, uint8_t function, uint8_t flags)
{
    return false;
}

bool HAL_FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                             flash_device_t destinationDeviceID, uint32_t destinationAddress,
                             uint32_t length)
{
    return false;
}

bool HAL_FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                             flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                             uint32_t length, uint8_t function, uint8_t flags)
{
    return false;
}

bool HAL_FLASH_AddToFactoryResetModuleSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                           flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                           uint32_t length, uint8_t function, uint8_t flags)
{
    return false;
}

bool HAL_FLASH_ClearFactoryResetModuleSlot(void)
{
    return false;
}

bool HAL_FLASH_RestoreFromFactoryResetModuleSlot(void)
{
    return false;
}

void HAL_FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating))
{
    //Not Applicable
}

void HAL_FLASH_WriteProtectionEnable(uint32_t FLASH_Sectors)
{
    FLASH_WriteProtection_Enable(FLASH_Sectors);
}

void HAL_FLASH_WriteProtectionDisable(uint32_t FLASH_Sectors)
{
    FLASH_WriteProtection_Disable(FLASH_Sectors);
}

void HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize) 
{
    FLASH_Begin(sFLASH_Address, fileSize);
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize) 
{
    return FLASH_Update(pBuffer, address, bufferSize);
}

void HAL_FLASH_End(void) 
{
    FLASH_End();
}

uint32_t HAL_FLASH_ModuleAddress(uint32_t address)
{
    //Only implemented in photon at present
    return 0;
}

uint32_t HAL_FLASH_ModuleLength(uint32_t address)
{
    //Only implemented in photon at present
    return 0;
}

bool HAL_FLASH_VerifyCRC32(uint32_t address, uint32_t length)
{
    //Only implemented in photon at present
    return false;
}

void parseServerAddressData(ServerAddress* server_addr, uint8_t* buf)
{
  // Internet address stored on external flash may be
  // either a domain name or an IP address.
  // It's stored in a type-length-value encoding.
  // First byte is type, second byte is length, the rest is value.

  switch (buf[0])
  {
    case IP_ADDRESS:
      server_addr->addr_type = IP_ADDRESS;
      server_addr->ip = (buf[2] << 24) | (buf[3] << 16) |
                        (buf[4] << 8)  |  buf[5];
      break;

    case DOMAIN_NAME:
      if (buf[1] <= EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH - 2)
      {
        server_addr->addr_type = DOMAIN_NAME;
        memcpy(server_addr->domain, buf + 2, buf[1]);

        // null terminate string
        char *p = server_addr->domain + buf[1];
        *p = 0;
        break;
      }
      // else fall through to default

    default:
      server_addr->addr_type = INVALID_INTERNET_ADDRESS;
  }

}

void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
    uint8_t buf[EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH];
    FLASH_Read_ServerAddress_Data(buf);
    parseServerAddressData(server_addr, buf);
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

