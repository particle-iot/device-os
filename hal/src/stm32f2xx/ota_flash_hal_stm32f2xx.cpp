/**
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


#include "core_hal.h"
#include "ota_flash_hal.h"
#include "rng_hal.h"
#include "hw_config.h"
#include "flash_mal.h"
#include "dct_hal.h"
#include "dsakeygen.h"
#include <cstring>
#include "ledcontrol.h"
#include "parse_server_address.h"
#include "spark_macros.h"
#include "bootloader.h"
#include "ota_module.h"
#include "ota_flash_hal_stm32f2xx.h"
#include "spark_protocol_functions.h"

#define OTA_CHUNK_SIZE          512

/**
 * Finds the location where a given module is stored. The module is identified
 * by it's funciton and index.
 * @param module_function   The function of the module to find.
 * @param module_index      The function index of the module to find.
 * @return the module_bounds corresponding to the module, NULL when not found.
 */
const module_bounds_t* find_module_bounds(uint8_t module_function, uint8_t module_index)
{
    for (unsigned i=0; i<module_bounds_length; i++) {
        if (module_bounds[i]->module_function==module_function && module_bounds[i]->module_index==module_index)
            return module_bounds[i];
    }
    return NULL;
}


void set_key_value(key_value* kv, const char* key, const char* value)
{
    kv->key = key;
    strncpy(kv->value, value, sizeof(kv->value)-1);
}

void HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved)
{
    if (construct) {
        info->platform_id = PLATFORM_ID;
        // bootloader, system 1, system 2, optional user code and factory restore
        uint8_t count = module_bounds_length;
        info->modules = new hal_module_t[count];
        if (info->modules) {
            info->module_count = count;
            for (unsigned i=0; i<count; i++) {
                fetch_module(info->modules+i, module_bounds[i], false, MODULE_VALIDATION_INTEGRITY);
            }
        }
    }
    else
    {
        delete info->modules;
        info->modules = NULL;
    }
    HAL_OTA_Add_System_Info(info, construct, reserved);
}

bool validate_module_dependencies(const module_bounds_t* bounds, bool userOptional)
{
    bool valid = false;
    const module_info_t* module = locate_module(bounds);
    if (module)
    {
        if (module->dependency.module_function == MODULE_FUNCTION_NONE || (userOptional && module_function(module)==MODULE_FUNCTION_USER_PART)) {
            valid = true;
        }
        else {
            // deliberately not transitive, so we only check the first dependency
            // so only user->system_part_2 is checked
            const module_bounds_t* dependency_bounds = find_module_bounds(module->dependency.module_function, module->dependency.module_index);
            const module_info_t* dependency = locate_module(dependency_bounds);
            valid = dependency && (dependency->module_version>=module->dependency.module_version);
        }
    }
    return valid;
}


bool HAL_Verify_User_Dependencies()
{
    const module_bounds_t* bounds = find_module_bounds(MODULE_FUNCTION_USER_PART, 1);
    return validate_module_dependencies(bounds, false);
}

bool HAL_OTA_CheckValidAddressRange(uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length;

#ifdef USE_SERIAL_FLASH
    if (startAddress == EXTERNAL_FLASH_OTA_ADDRESS && endAddress <= 0x100000)
    {
        return true;
    }
#else
    if (startAddress == INTERNAL_FLASH_OTA_ADDRESS && endAddress < 0x8100000)
    {
        return true;
    }
#endif

    return false;
}

uint32_t HAL_OTA_FlashAddress()
{
#ifdef USE_SERIAL_FLASH
    return EXTERNAL_FLASH_OTA_ADDRESS;
#else
    return module_ota.start_address;
#endif
}

STATIC_ASSERT(ota_length_for_pid_6_is_less_than_512k, PLATFORM_ID!=5 || FIRMWARE_IMAGE_SIZE<512*1024);

uint32_t HAL_OTA_FlashLength()
{
    return module_ota.maximum_size;
}

uint16_t HAL_OTA_ChunkSize()
{
    return OTA_CHUNK_SIZE;
}

bool HAL_FLASH_Begin(uint32_t address, uint32_t length, void* reserved)
{
    FLASH_Begin(address, length);
    return true;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
    return FLASH_Update(pBuffer, address, length);
}

hal_update_complete_t HAL_FLASH_End(void* reserved)
{
    hal_module_t module;
    hal_update_complete_t result = HAL_UPDATE_ERROR;
    if (fetch_module(&module, &module_ota, true, MODULE_VALIDATION_INTEGRITY) && (module.validity_checked==module.validity_result))
    {
        uint32_t moduleLength = module_length(module.info);
        module_function_t function = module_function(module.info);

        // bootloader is copied directly
        if (function==MODULE_FUNCTION_BOOTLOADER) {
            if (bootloader_update((const void*)module_ota.start_address, moduleLength+4))
                result = HAL_UPDATE_APPLIED;
        }
        else
        {
            if (FLASH_AddToNextAvailableModulesSlot(FLASH_INTERNAL, module_ota.start_address,
                FLASH_INTERNAL, uint32_t(module.info->module_start_address),
                (moduleLength + 4),//+4 to copy the CRC too
                function,
                MODULE_VERIFY_CRC|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_FUNCTION))//true to verify the CRC during copy also
                    result = HAL_UPDATE_APPLIED_PENDING_RESTART;

        }

        FLASH_End();
    }
    return result;
}

void copy_dct(void* target, uint16_t offset, uint16_t length) {
    const void* data = dct_read_app_data(offset);
    memcpy(target, data, length);
}


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
    const void* data = dct_read_app_data(DCT_SERVER_ADDRESS_OFFSET);
    parseServerAddressData(server_addr, (const uint8_t*)data, DCT_SERVER_ADDRESS_SIZE);
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

/**
 * Reads and generates the device's private key.
 * @param keyBuffer
 * @return
 */
int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* genspec)
{
    bool generated = false;
    copy_dct(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
    genspec->had_key = (*keyBuffer!=0xFF); // uninitialized
    if (genspec->gen==PRIVATE_KEY_GENERATE_ALWAYS || (!genspec->had_key && genspec->gen!=PRIVATE_KEY_GENERATE_NEVER)) {
        // todo - this couples the HAL with the system. Use events instead.
        SPARK_LED_FADE = false;
        if (!gen_rsa_key(keyBuffer, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH, rsa_random, NULL)) {
            dct_write_app_data(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
            // refetch and rewrite public key to ensure it is valid
            fetch_device_public_key();
            generated = true;
        }
    }
    genspec->generated_key = generated;
    return 0;
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

const uint8_t* fetch_server_public_key()
{
    return (const uint8_t*)dct_read_app_data(DCT_SERVER_PUBLIC_KEY_OFFSET);
}

const uint8_t* fetch_device_private_key()
{
    return (const uint8_t*)dct_read_app_data(DCT_DEVICE_PRIVATE_KEY_OFFSET);
}

const uint8_t* fetch_device_public_key()
{
    uint8_t pubkey[DCT_DEVICE_PUBLIC_KEY_SIZE];
    memset(pubkey, 0, sizeof(pubkey));
    parse_device_pubkey_from_privkey(pubkey, fetch_device_private_key());

    const uint8_t* flash_pub_key = (const uint8_t*)dct_read_app_data(DCT_DEVICE_PUBLIC_KEY_OFFSET);
    if (memcmp(pubkey, flash_pub_key, sizeof(pubkey))) {
        dct_write_app_data(pubkey, DCT_DEVICE_PUBLIC_KEY_OFFSET, DCT_DEVICE_PUBLIC_KEY_SIZE);
        flash_pub_key = (const uint8_t*)dct_read_app_data(DCT_DEVICE_PUBLIC_KEY_OFFSET);
    }
    return flash_pub_key;
}


int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned data_length)
{
    unsigned offset = 0;
    unsigned length = -1;

    switch (config_item)
    {
    case SYSTEM_CONFIG_DEVICE_KEY:
        offset = DCT_DEVICE_PRIVATE_KEY_OFFSET;
        length = DCT_DEVICE_PRIVATE_KEY_SIZE;
        break;
    case SYSTEM_CONFIG_SERVER_KEY:
        offset = DCT_SERVER_PUBLIC_KEY_OFFSET;
        length = DCT_SERVER_PUBLIC_KEY_SIZE;
        break;
    case SYSTEM_CONFIG_SOFTAP_PREFIX:
        offset = DCT_SSID_PREFIX_OFFSET;
        length = DCT_SSID_PREFIX_SIZE-1;
        if (data_length>length)
            data_length = length;
        dct_write_app_data(&data_length, offset++, 1);
        break;
    case SYSTEM_CONFIG_SOFTAP_SUFFIX:
        offset = DCT_DEVICE_ID_OFFSET;
        length = DCT_DEVICE_ID_SIZE;
        break;
    case SYSTEM_CONFIG_SOFTAP_HOSTNAMES:
        break;
    }

    if (length>=0)
        dct_write_app_data(data, offset, length>data_length ? data_length : length);

    return length;
}