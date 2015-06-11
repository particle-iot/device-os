/**
 ******************************************************************************
 * @file    ota_flash_hal.cpp
 * @author  Matthew McGowan, Satish Nair
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

#include "core_hal.h"
#include "ota_flash_hal.h"
#include "rng_hal.h"
#include "hw_config.h"
#include "flash_mal.h"
#include "dct_hal.h"
#include "dsakeygen.h"
#include "softap.h"
#include <cstring>
#include "ledcontrol.h"
#include "parse_server_address.h"
#include "spark_macros.h"
#include "bootloader.h"

#define OTA_CHUNK_SIZE          512

/**
 * Checks if the minimum required dependencies for the given module are satisfied.
 * @param bounds    The bounds of the module to check.
 * @return {@code true} if the dependencies are satisfied, {@code false} otherwise.
 */
bool validate_module_dependencies(const module_bounds_t* bounds, bool userPartOptional);
const module_bounds_t* find_module_bounds(uint8_t module_function, uint8_t module_index);

/**
 * Find the module_info at a given address. No validation is done so the data
 * pointed to should not be trusted.
 * @param bounds
 * @return 
 */
inline const module_info_t* locate_module(const module_bounds_t* bounds) {
    return FLASH_ModuleInfo(FLASH_INTERNAL, bounds->start_address);
}

/**
 * Determines if a given address is in range. 
 * @param test      The address to test
 * @param start     The start range (inclusive)
 * @param end       The end range (inclusive)
 * @return {@code true} if the address is within range.
 */
inline bool in_range(uint32_t test, uint32_t start, uint32_t end)
{
    return test>=start && test<=end;
}

/**
 * Fetches and validates the module info found at a given location.
 * @param target        Receives the module into
 * @param bounds        The location where to retrieve the module from.
 * @param userDepsOptional
 * @return {@code true} if the module info can be read via the info, crc, suffix pointers.
 */
bool fetch_module(hal_module_t* target, const module_bounds_t* bounds, bool userDepsOptional, uint16_t check_flags=0)
{
    memset(target, 0, sizeof(*target));

    target->bounds = *bounds;
    if (NULL!=(target->info = locate_module(bounds)))
    {
        target->validity_checked = MODULE_VALIDATION_RANGE | MODULE_VALIDATION_DEPENDENCIES | MODULE_VALIDATION_PLATFORM | check_flags;
        target->validity_result = 0;
        const uint8_t* module_end = (const uint8_t*)target->info->module_end_address;
        const module_bounds_t* expected_bounds = find_module_bounds(module_function(target->info), module_index(target->info));        
        if (expected_bounds && in_range(uint32_t(module_end), expected_bounds->start_address, expected_bounds->end_address)) {
            target->validity_result |= MODULE_VALIDATION_RANGE;
            target->validity_result |= (PRODUCT_ID==module_platform_id(target->info)) ? MODULE_VALIDATION_PLATFORM : 0;
            // the suffix ends at module_end, and the crc starts after module end
            target->crc = (module_info_crc_t*)module_end;
            target->suffix = (module_info_suffix_t*)(module_end-sizeof(module_info_suffix_t));            
            if (validate_module_dependencies(bounds, userDepsOptional))
                target->validity_result |= MODULE_VALIDATION_DEPENDENCIES;
            if ((target->validity_checked & MODULE_VALIDATION_INTEGRITY) && FLASH_VerifyCRC32(FLASH_INTERNAL, bounds->start_address, module_length(target->info)))
                target->validity_result |= MODULE_VALIDATION_INTEGRITY;
        }
        else
            target->info = NULL;
    }           
    return target->info!=NULL;
}

#if MODULAR_FIRMWARE
const module_bounds_t module_bootloader = { 0x4000, 0x8000000, 0x8004000, MODULE_FUNCTION_BOOTLOADER, 0, MODULE_STORE_MAIN };
const module_bounds_t module_system_part1 = { 0x40000, 0x8020000, 0x8060000, MODULE_FUNCTION_SYSTEM_PART, 1, MODULE_STORE_MAIN };
const module_bounds_t module_system_part2 = { 0x40000, 0x8060000, 0x80A0000, MODULE_FUNCTION_SYSTEM_PART, 2, MODULE_STORE_MAIN};
const module_bounds_t module_user = { 0x20000, 0x80A0000, 0x80C0000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_MAIN};
const module_bounds_t module_factory = { 0x20000, 0x80E0000, 0x8100000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_FACTORY};
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_system_part1, &module_system_part2, &module_user, &module_factory };

const module_bounds_t module_ota = { 0x40000, 0x80C0000, 0x8100000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD};
#else
const module_bounds_t module_bootloader = { 0x4000, 0x8000000, 0x8004000, MODULE_FUNCTION_BOOTLOADER, 0, MODULE_STORE_MAIN};
const module_bounds_t module_user = { 0x60000, 0x8020000, 0x8080000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_MAIN};
const module_bounds_t module_factory = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_FACTORY};
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_user, &module_factory };

const module_bounds_t module_ota = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD};
#endif

/**
 * Finds the location where a given module is stored. The module is identified
 * by it's funciton and index.
 * @param module_function   The function of the module to find.
 * @param module_index      The function index of the module to find.
 * @return the module_bounds corresponding to the module, NULL when not found.
 */
const module_bounds_t* find_module_bounds(uint8_t module_function, uint8_t module_index)
{
    for (unsigned i=0; i<arraySize(module_bounds); i++) {
        if (module_bounds[i]->module_function==module_function && module_bounds[i]->module_index==module_index)
            return module_bounds[i];
    }
    return NULL;
}

void HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved)
{
    if (construct) {
        info->platform_id = PLATFORM_ID;
        // bootloader, system 1, system 2, optional user code and factory restore    
        uint8_t count = arraySize(module_bounds);
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

