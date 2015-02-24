/**
 ******************************************************************************
 * @file    ota_flash_hal.cpp
 * @author  Matthew McGowan, Satish Nair
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

#include "core_hal.h"
#include "ota_flash_hal.h"
#include "rng_hal.h"
#include "hw_config.h"
#include "dct_hal.h"
#include "dsakeygen.h"
#include "softap.h"
#include <cstring>

#define OTA_CHUNK_SIZE          512

uint32_t HAL_OTA_FlashAddress()
{
#ifdef USE_SERIAL_FLASH
    return EXTERNAL_FLASH_OTA_ADDRESS;
#else
    return INTERNAL_FLASH_OTA_ADDRESS;
#endif
}

STATIC_ASSERT(ota_length_for_pid_6_is_less_than_512k, PLATFORM_ID!=5 || FIRMWARE_IMAGE_SIZE<512*1024);

uint32_t HAL_OTA_FlashLength()
{
    return FIRMWARE_IMAGE_SIZE;
}

uint16_t HAL_OTA_ChunkSize()
{
    return OTA_CHUNK_SIZE;
}

bool HAL_FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                          flash_device_t destinationDeviceID, uint32_t destinationAddress,
                          uint32_t length, uint8_t function, uint8_t flags)
{
    return FLASH_CopyMemory(sourceDeviceID, sourceAddress,
                            destinationDeviceID, destinationAddress,
                            length, function, flags);
}

bool HAL_FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                             flash_device_t destinationDeviceID, uint32_t destinationAddress,
                             uint32_t length)
{
    return FLASH_CompareMemory(sourceDeviceID, sourceAddress,
                               destinationDeviceID, destinationAddress,
                               length);
}

bool HAL_FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                             flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                             uint32_t length, uint8_t function, uint8_t flags)
{
    return FLASH_AddToNextAvailableModulesSlot(sourceDeviceID, sourceAddress,
                                               destinationDeviceID, destinationAddress,
                                               length, function, flags);
}

bool HAL_FLASH_AddToFactoryResetModuleSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                           flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                           uint32_t length, uint8_t function, uint8_t flags)
{
    return FLASH_AddToFactoryResetModuleSlot(sourceDeviceID, sourceAddress,
                                             destinationDeviceID, destinationAddress,
                                             length, function, flags);
}

bool HAL_FLASH_ClearFactoryResetModuleSlot(void)
{
    return FLASH_ClearFactoryResetModuleSlot();
}

bool HAL_FLASH_RestoreFromFactoryResetModuleSlot(void)
{
    return FLASH_RestoreFromFactoryResetModuleSlot();
}

void HAL_FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating))
{
    FLASH_UpdateModules(flashModulesCallback);
}

void HAL_FLASH_WriteProtectionEnable(uint32_t FLASH_Sectors)
{
    FLASH_WriteProtection_Enable(FLASH_Sectors);
}

void HAL_FLASH_WriteProtectionDisable(uint32_t FLASH_Sectors)
{
    FLASH_WriteProtection_Disable(FLASH_Sectors);
}

void HAL_FLASH_Begin(uint32_t address, uint32_t length)
{
    FLASH_Begin(address, length);
}

uint16_t HAL_FLASH_Update(uint8_t *pBuffer, uint32_t length)
{
    return FLASH_Update(pBuffer, length);
}

void HAL_FLASH_End(void)
{
    FLASH_End();
}

uint32_t HAL_FLASH_ModuleAddress(uint32_t address)
{
#ifdef USE_SERIAL_FLASH
    return 0;
#else
    return FLASH_ModuleAddress(FLASH_INTERNAL, address);
#endif
}

uint32_t HAL_FLASH_ModuleLength(uint32_t address)
{
#ifdef USE_SERIAL_FLASH
    return 0;
#else
    return FLASH_ModuleLength(FLASH_INTERNAL, address);
#endif
}

bool HAL_FLASH_VerifyCRC32(uint32_t address, uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    return false;
#else
    return FLASH_VerifyCRC32(FLASH_INTERNAL, address, length);
#endif
}

void copy_dct(void* target, uint16_t offset, uint16_t length) {
    const void* data = dct_read_app_data(offset);
    memcpy(target, data, length);
}

void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
    copy_dct(server_addr, DCT_SERVER_ADDRESS_OFFSET, DCT_SERVER_ADDRESS_SIZE);
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
    fetch_device_public_key();
    copy_dct(keyBuffer, DCT_SERVER_PUBLIC_KEY_OFFSET, EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
}

int rsa_random(void* p) 
{
    return (int)HAL_RNG_GetRandomNumber();
}

void HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{         
    copy_dct(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
    if (*keyBuffer==0xFF) {         // uninitialized
        if (!gen_rsa_key(keyBuffer, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH, rsa_random, NULL)) {
            dct_write_app_data(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
            
            // refetch and rewrite public key to ensure it is valid
            fetch_device_public_key();
        }        
    }    
}

STATIC_ASSERT(Internet_Address_is_2_bytes_c1, sizeof(Internet_Address_TypeDef)==1);
STATIC_ASSERT(ServerAddress_packed_c1, offsetof(ServerAddress, ip)==2);    



void check() {
    // todo - why is this static assert giving a different result?
    STATIC_ASSERT_EXPR(Internet_Address_is_2_bytes_c, sizeof(Internet_Address_TypeDef)==2);
    STATIC_ASSERT_EXPR(ServerAddress_packed_c, offsetof(ServerAddress, ip)==4);    
}


uint16_t HAL_Set_Claim_Code(const char* code) 
{
    if (code) 
        return dct_write_app_data(code, DCT_CLAIM_CODE_OFFSET, DCT_CLAIM_CODE_SIZE);
    else // clear code
    {
        char c = '\0';
        dct_write_app_data(&c, DCT_CLAIM_CODE_OFFSET, 1);
        // now flag as claimed
        const uint8_t* claimed = (const uint8_t*)dct_read_app_data(DCT_DEVICE_CLAIMED_OFFSET);
        c = '1';
        if (*claimed!=uint8_t(c))
        {            
            dct_write_app_data(&c, DCT_DEVICE_CLAIMED_OFFSET, 1);
        }
    }
    return 0;    
}

uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len) 
{
    const uint8_t* data = (const uint8_t*)dct_read_app_data(DCT_CLAIM_CODE_OFFSET);
    uint16_t result = 0;
    if (len>DCT_CLAIM_CODE_SIZE) {
        memcpy(buffer, data, DCT_CLAIM_CODE_SIZE);
        buffer[DCT_CLAIM_CODE_SIZE] = 0;
    }
    else {
        result = -1;
    }
    return result;
}

