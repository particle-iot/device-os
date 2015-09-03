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
#include "cellular_hal.h"
#include <string.h>

void set_key_value(key_value* kv, const char* key, const char* value)
{
    kv->key = key;
    strncpy(kv->value, value, sizeof(kv->value-1));
}

void HAL_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    info->platform_id = PLATFORM_ID;
    info->module_count = 0;
    info->modules = NULL;
    // TODO: add module info

    info->key_value_count = 2;
    info->key_values = new key_value[info->key_value_count];

    CellularDevice device;
    memset(&device, 0, sizeof(device));
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);

    set_key_value(info->key_values, "imei", device.imei);
    set_key_value(info->key_values+1, "iccid", device.iccid);
}


uint32_t HAL_OTA_FlashAddress()
{
    return 0;
}

uint32_t HAL_OTA_FlashLength()
{
    return 0;
}

uint16_t HAL_OTA_ChunkSize()
{
    return 0;
}

bool HAL_FLASH_Begin(uint32_t address, uint32_t length, void* reserved)
{
    return false;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
    return 0;
}

hal_update_complete_t HAL_FLASH_End(void* reserved)
{
    return HAL_UPDATE_ERROR;
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
}

int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* genspec)
{
    bool generated = false;
    // copy_dct(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
    // genspec->had_key = (*keyBuffer!=0xFF); // uninitialized
    // if (genspec->gen==PRIVATE_KEY_GENERATE_ALWAYS || (!genspec->had_key && genspec->gen!=PRIVATE_KEY_GENERATE_NEVER)) {
    //     // todo - this couples the HAL with the system. Use events instead.
    //     SPARK_LED_FADE = false;
    //     if (!gen_rsa_key(keyBuffer, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH, rsa_random, NULL)) {
    //         dct_write_app_data(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
    //         // refetch and rewrite public key to ensure it is valid
    //         fetch_device_public_key();
    //         generated = true;
    //     }
    // }
    genspec->generated_key = generated;
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


