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
#include "eckeygen.h"
#include <cstring>
#include "parse_server_address.h"
#include "spark_macros.h"
#include "bootloader.h"
#include "ota_module.h"
#include "ota_flash_hal_stm32f2xx.h"
#include "spark_protocol_functions.h"
#include "hal_platform.h"
#include "hal_event.h"
#include "service_debug.h"
#include "spark_wiring_random.h"
#include "delay_hal.h"
// For ATOMIC_BLOCK
#include "spark_wiring_interrupts.h"

#include <memory>

#define OTA_CHUNK_SIZE                 (512)
#define BOOTLOADER_RANDOM_BACKOFF_MIN  (200)
#define BOOTLOADER_RANDOM_BACKOFF_MAX  (1000)

static hal_update_complete_t flash_bootloader(hal_module_t* mod, uint32_t moduleLength);

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

bool validate_module_dependencies_full(const module_info_t* module, const module_bounds_t* bounds)
{
    if (module_function(module) != MODULE_FUNCTION_SYSTEM_PART)
        return true;

    bool valid = true;

    // When updating system-parts
    // If MODULE_VALIDATION_DEPENDENCIES_FULL was requested, validate that the dependecies
    // would still be satisfied after the module from the "ota_module" replaces the current one
    hal_system_info_t sysinfo;
    memset(&sysinfo, 0, sizeof(sysinfo));
    sysinfo.size = sizeof(sysinfo);
    HAL_System_Info(&sysinfo, true, nullptr);
    for (unsigned i=0; i<sysinfo.module_count; i++) {
        const hal_module_t& smod = sysinfo.modules[i];
        const module_info_t* info = smod.info;
        if (!info)
            continue;

        // Just in case
        if (!memcmp((const void*)&smod.bounds, (const void*)bounds, sizeof(module_bounds_t))) {
            // Do not validate against self
            continue;
        }

        // Validate only system parts
        if (module_function(info) != MODULE_FUNCTION_SYSTEM_PART)
            continue;

        if (info->module_start_address == module->module_start_address &&
            module_function(module) == module_function(info) &&
            module_index(module) == module_index(info)) {
            // Do not validate replaced module
            continue;
        }

        if (info->dependency.module_function != MODULE_FUNCTION_NONE) {
            if (info->dependency.module_function == module->module_function &&
                info->dependency.module_index == module->module_index) {
                valid = module->module_version >= info->dependency.module_version;
                if (!valid)
                    break;
            }
        }

        if (info->dependency2.module_function != MODULE_FUNCTION_NONE) {
            if (info->dependency2.module_function == module->module_function &&
                info->dependency2.module_index == module->module_index) {
                valid = module->module_version >= info->dependency2.module_version;
                if (!valid)
                    break;
            }
        }
    }
    HAL_System_Info(&sysinfo, false, nullptr);

    return valid;
}

bool validate_module_dependencies(const module_bounds_t* bounds, bool userOptional, bool fullDeps)
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
            if (module->dependency.module_function != MODULE_FUNCTION_NONE) {
                const module_bounds_t* dependency_bounds = find_module_bounds(module->dependency.module_function, module->dependency.module_index);
                const module_info_t* dependency = locate_module(dependency_bounds);
                valid = dependency && (dependency->module_version>=module->dependency.module_version);
            } else {
                valid = true;
            }
            // Validate dependency2
            if (module->dependency2.module_function == MODULE_FUNCTION_NONE ||
                (module->dependency2.module_function == MODULE_FUNCTION_BOOTLOADER && userOptional)) {
                valid = valid && true;
            } else {
                const module_bounds_t* dependency_bounds = find_module_bounds(module->dependency2.module_function, module->dependency2.module_index);
                const module_info_t* dependency = locate_module(dependency_bounds);
                valid = valid && dependency && (dependency->module_version>=module->dependency2.module_version);
            }
        }

        if (fullDeps && valid) {
            valid = valid && validate_module_dependencies_full(module, bounds);
        }
    }
    return valid;
}


bool HAL_Verify_User_Dependencies()
{
    const module_bounds_t* bounds = find_module_bounds(MODULE_FUNCTION_USER_PART, 1);
    return validate_module_dependencies(bounds, false, false);
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

static hal_update_complete_t flash_bootloader(hal_module_t* mod, uint32_t moduleLength)
{
    hal_update_complete_t result = HAL_UPDATE_ERROR;
    uint32_t attempt = 0;
    do {
        int fres = FLASH_ACCESS_RESULT_ERROR;
        if (attempt++ > 0) {
            ATOMIC_BLOCK() {
                // If it's not the first flashing attempt, try with interrupts disabled
                fres = bootloader_update((const void*)mod->bounds.start_address, moduleLength + 4);
            }
        } else {
            fres = bootloader_update((const void*)mod->bounds.start_address, moduleLength + 4);
        }
        if (fres == FLASH_ACCESS_RESULT_OK) {
            // Validate bootloader
            hal_module_t module;
            bool module_fetched = fetch_module(&module, &module_bootloader, true, MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL);
            if (module_fetched && (module.validity_checked == module.validity_result)) {
                result = HAL_UPDATE_APPLIED;
                break;
            }
        }

        if (fres == FLASH_ACCESS_RESULT_BADARG) {
            // The bootloader is still intact, for some reason the module being copied has failed some checks.
            result = HAL_UPDATE_ERROR;
            break;
        }

        // Random backoff
        system_tick_t period = random(BOOTLOADER_RANDOM_BACKOFF_MIN, BOOTLOADER_RANDOM_BACKOFF_MAX);
        LOG_DEBUG(WARN, "Failed to flash bootloader. Retrying in %lu ms", period);
        HAL_Delay_Milliseconds(period);
    } while ((result == HAL_UPDATE_ERROR));
    return result;
}

int HAL_FLASH_OTA_Validate(hal_module_t* mod, bool userDepsOptional, module_validation_flags_t flags, void* reserved) {
    hal_module_t module;

    bool module_fetched = fetch_module(&module, &module_ota, userDepsOptional, flags);

    if (mod) {
        memcpy(mod, &module, sizeof(hal_module_t));
    }

    return (int)!module_fetched;
}

hal_update_complete_t HAL_FLASH_End(hal_module_t* mod)
{
    hal_module_t module;
    hal_update_complete_t result = HAL_UPDATE_ERROR;

    bool module_fetched = !HAL_FLASH_OTA_Validate(&module, true, (module_validation_flags_t)(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL), NULL);
	DEBUG("module fetched %d, checks=%d, result=%d", module_fetched, module.validity_checked, module.validity_result);
    if (module_fetched && (module.validity_checked==module.validity_result))
    {
        uint32_t moduleLength = module_length(module.info);
        module_function_t function = module_function(module.info);

        // bootloader is copied directly
        if (function==MODULE_FUNCTION_BOOTLOADER) {
            result = flash_bootloader(&module, moduleLength);
        }
        else
        {
            if (FLASH_AddToNextAvailableModulesSlot(FLASH_INTERNAL, module_ota.start_address,
                FLASH_INTERNAL, uint32_t(module.info->module_start_address),
                (moduleLength + 4),//+4 to copy the CRC too
                function,
                MODULE_VERIFY_CRC|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_FUNCTION)) { //true to verify the CRC during copy also
                    result = HAL_UPDATE_APPLIED_PENDING_RESTART;
                    DEBUG("OTA module applied - device will restart");
            }
        }

        FLASH_End();
    }
    else
    {
    		WARN("OTA module not applied");
    }
    if (mod)
    {
        memcpy(mod, &module, sizeof(hal_module_t));
    }
    return result;
}

// Todo this code and much of the code here is duplicated between Gen2 and Gen3
// This should be factored out into directory shared by both platforms.
hal_update_complete_t HAL_FLASH_ApplyPendingUpdate(hal_module_t* module, bool dryRun, void* reserved)
{
    uint8_t otaUpdateFlag = DCT_OTA_UPDATE_FLAG_CLEAR;
    STATIC_ASSERT(sizeof(otaUpdateFlag)==DCT_OTA_UPDATE_FLAG_SIZE, "expected ota update flag size to be 1");
    dct_read_app_data_copy(DCT_OTA_UPDATE_FLAG_OFFSET, &otaUpdateFlag, DCT_OTA_UPDATE_FLAG_SIZE);
    hal_update_complete_t result = HAL_UPDATE_ERROR;
    if (otaUpdateFlag==DCT_OTA_UPDATE_FLAG_SET) {
        if (dryRun) {
            // todo - we should probably check the module integrity too
            // ideally this parameter would be passed to HAL_FLASH_End to avoid duplication of logic here.
            result = HAL_UPDATE_APPLIED;
        }
        else {
            // clear the flag
            otaUpdateFlag = DCT_OTA_UPDATE_FLAG_CLEAR;
            dct_write_app_data(&otaUpdateFlag, DCT_OTA_UPDATE_FLAG_OFFSET, DCT_OTA_UPDATE_FLAG_SIZE);
            result = HAL_FLASH_End(module);
        }
    }
    return result;
}


void copy_dct(void* target, uint16_t offset, uint16_t length) {
    dct_read_app_data_copy(offset, target, length);
}


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
	bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
    const void* data = dct_read_app_data_lock(udp ? DCT_ALT_SERVER_ADDRESS_OFFSET : DCT_SERVER_ADDRESS_OFFSET);
    parseServerAddressData(server_addr, (const uint8_t*)data, DCT_SERVER_ADDRESS_SIZE);
    dct_read_app_data_unlock(udp ? DCT_ALT_SERVER_ADDRESS_OFFSET : DCT_SERVER_ADDRESS_OFFSET);
}

void HAL_FLASH_Write_ServerAddress(const uint8_t *buf, bool udp)
{
    if (udp) {
        dct_write_app_data(buf, DCT_ALT_SERVER_ADDRESS_OFFSET, DCT_ALT_SERVER_ADDRESS_SIZE );
    } else {
        dct_write_app_data(buf, DCT_SERVER_ADDRESS_OFFSET, DCT_SERVER_ADDRESS_SIZE);
    }
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
    fetch_device_public_key(1);
    fetch_device_public_key(0);
	bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
	if (udp)
	    copy_dct(keyBuffer, DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
	else
		copy_dct(keyBuffer, DCT_SERVER_PUBLIC_KEY_OFFSET, EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
}

void HAL_FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer, bool udp)
{
    if (udp) {
        dct_write_app_data(keyBuffer, DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
    } else {
        dct_write_app_data(keyBuffer, DCT_SERVER_PUBLIC_KEY_OFFSET, EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
    }
}

int32_t key_gen_random(void* p)
{
    return (int32_t)HAL_RNG_GetRandomNumber();
}

int key_gen_random_block(void* handle, uint8_t* data, size_t len)
{
	while (len>=4)
	{
		*((uint32_t*)data) = HAL_RNG_GetRandomNumber();
		data += 4;
		len -= 4;
	}
	while (len-->0)
	{
		*data++ = HAL_RNG_GetRandomNumber();
	}
	return 0;
}



/**
 * Reads and generates the device's private key.
 * @param keyBuffer
 * @return
 */
int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* genspec)
{
    bool generated = false;
    bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
    if (udp)
    		copy_dct(keyBuffer, DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, DCT_ALT_DEVICE_PRIVATE_KEY_SIZE);
    else
    		copy_dct(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
    genspec->had_key = (*keyBuffer!=0xFF); // uninitialized
    if (genspec->gen==PRIVATE_KEY_GENERATE_ALWAYS || (!genspec->had_key && genspec->gen!=PRIVATE_KEY_GENERATE_NEVER)) {
        hal_notify_event(HAL_EVENT_GENERATE_DEVICE_KEY, HAL_EVENT_FLAG_START, nullptr);
        int error  = 1;
#if HAL_PLATFORM_CLOUD_UDP
        if (udp)
        		error = gen_ec_key(keyBuffer, DCT_ALT_DEVICE_PRIVATE_KEY_SIZE, key_gen_random_block, NULL);
#endif
#if HAL_PLATFORM_CLOUD_TCP
		if (!udp)
			error = gen_rsa_key(keyBuffer, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH, key_gen_random, NULL);
#endif
        if (!error) {
        		if (udp)
        			dct_write_app_data(keyBuffer, DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, DCT_ALT_DEVICE_PRIVATE_KEY_SIZE);
        		else
        			dct_write_app_data(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
			// refetch and rewrite public key to ensure it is valid
			fetch_device_public_key(1);
            fetch_device_public_key(0);
            generated = true;
        }
        hal_notify_event(HAL_EVENT_GENERATE_DEVICE_KEY, HAL_EVENT_FLAG_STOP, nullptr);
    }
    genspec->generated_key = generated;
    return 0;
}

STATIC_ASSERT(Internet_Address_is_2_bytes_c1, sizeof(Internet_Address_TypeDef)==1);
STATIC_ASSERT(ServerAddress_packed_c1, offsetof(ServerAddress, ip)==2);

uint16_t HAL_Set_Claim_Code(const char* code)
{
    if (code)
        return dct_write_app_data(code, DCT_CLAIM_CODE_OFFSET, DCT_CLAIM_CODE_SIZE);
    else // clear code
    {
        char c = '\0';
        dct_write_app_data(&c, DCT_CLAIM_CODE_OFFSET, 1);
        // now flag as claimed
        uint8_t claimed = 0;
        dct_read_app_data_copy(DCT_DEVICE_CLAIMED_OFFSET, &claimed, sizeof(claimed));
        c = '1';
        if (claimed!=uint8_t(c))
        {
            dct_write_app_data(&c, DCT_DEVICE_CLAIMED_OFFSET, 1);
        }
    }
    return 0;
}

uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len)
{
    uint16_t result = 0;
    if (len>DCT_CLAIM_CODE_SIZE) {
        dct_read_app_data_copy(DCT_CLAIM_CODE_OFFSET, buffer, DCT_CLAIM_CODE_SIZE);
        buffer[DCT_CLAIM_CODE_SIZE] = 0;
    }
    else {
        result = -1;
    }
    return result;
}

bool HAL_IsDeviceClaimed(void* reserved)
{
    uint8_t claimed = 0;
    dct_read_app_data_copy(DCT_DEVICE_CLAIMED_OFFSET, &claimed, sizeof(claimed));
    return (claimed)=='1';
}


const uint8_t* fetch_server_public_key(uint8_t lock)
{
    if (lock) {
        return (const uint8_t*)dct_read_app_data_lock(HAL_Feature_Get(FEATURE_CLOUD_UDP) ? DCT_ALT_SERVER_PUBLIC_KEY_OFFSET : DCT_SERVER_PUBLIC_KEY_OFFSET);
    } else {
        dct_read_app_data_unlock(0);
        return NULL;
    }
}

const uint8_t* fetch_device_private_key(uint8_t lock)
{
    if (lock) {
        return (const uint8_t*)dct_read_app_data_lock(HAL_Feature_Get(FEATURE_CLOUD_UDP) ? DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET : DCT_DEVICE_PRIVATE_KEY_OFFSET);
    } else {
        dct_read_app_data_unlock(0);
        return NULL;
    }
}

const uint8_t* fetch_device_public_key(uint8_t lock)
{
    if (!lock) {
        dct_read_app_data_unlock(0);
        return NULL;
    }

    uint8_t pubkey[DCT_DEVICE_PUBLIC_KEY_SIZE];
    memset(pubkey, 0, sizeof(pubkey));
    bool udp = false;
#if HAL_PLATFORM_CLOUD_UDP
    udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#endif
    const uint8_t* priv = fetch_device_private_key(1);
    int extracted_length = 0;
#if HAL_PLATFORM_CLOUD_UDP
    if (udp) {
        extracted_length = extract_public_ec_key(pubkey, sizeof(pubkey), priv);
    }
#endif
#if HAL_PLATFORM_CLOUD_TCP
    if (!udp) {
        extract_public_rsa_key(pubkey, priv);
        extracted_length = DCT_DEVICE_PUBLIC_KEY_SIZE;
    }
#endif
    fetch_device_private_key(0);

    int offset = udp ? DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET : DCT_DEVICE_PUBLIC_KEY_OFFSET;
    size_t key_size = udp ? DCT_ALT_DEVICE_PUBLIC_KEY_SIZE : DCT_DEVICE_PUBLIC_KEY_SIZE;
    const uint8_t* flash_pub_key = (const uint8_t*)dct_read_app_data_lock(offset);
    if ((extracted_length > 0) && memcmp(pubkey, flash_pub_key, sizeof(pubkey))) {
        dct_read_app_data_unlock(offset);
        dct_write_app_data(pubkey, offset, key_size);
        flash_pub_key = (const uint8_t*)dct_read_app_data_lock(offset);
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
        offset = DCT_DEVICE_CODE_OFFSET;
        // copy the string with a zero terminator if less than the maximum width
        data_length = std::min(DCT_DEVICE_CODE_SIZE, data_length+1);
        length = data_length;
        break;
    case SYSTEM_CONFIG_SOFTAP_HOSTNAMES:
        break;
    }

    if (length>=0)
        dct_write_app_data(data, offset, length>data_length ? data_length : length);

    return length;
}

namespace {

bool dctAddressForKeyType(security_key_type type, size_t* offset, size_t* size) {
    size_t offs = 0;
    size_t n = 0;
    switch (type) {
#if HAL_PLATFORM_CLOUD_TCP
    case TCP_DEVICE_PRIVATE_KEY:
        offs = DCT_DEVICE_PRIVATE_KEY_OFFSET;
        n = DCT_DEVICE_PRIVATE_KEY_SIZE;
        break;
    case TCP_DEVICE_PUBLIC_KEY:
        offs = DCT_DEVICE_PUBLIC_KEY_OFFSET;
        n = DCT_DEVICE_PUBLIC_KEY_SIZE;
        break;
    case TCP_SERVER_PUBLIC_KEY:
        offs = DCT_SERVER_PUBLIC_KEY_OFFSET;
        n = DCT_SERVER_PUBLIC_KEY_SIZE;
        break;
#endif // HAL_PLATFORM_CLOUD_TCP
#if HAL_PLATFORM_CLOUD_UDP
    case UDP_DEVICE_PRIVATE_KEY:
        offs = DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET;
        n = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE;
        break;
    case UDP_DEVICE_PUBLIC_KEY:
        offs = DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET;
        n = DCT_ALT_DEVICE_PUBLIC_KEY_SIZE;
        break;
    case UDP_SERVER_PUBLIC_KEY:
        offs = DCT_ALT_SERVER_PUBLIC_KEY_OFFSET;
        n = DCT_ALT_SERVER_PUBLIC_KEY_SIZE;
        break;
#endif // HAL_PLATFORM_CLOUD_UDP
    default:
        return false;
    }
    if (offset) {
        *offset = offs;
    }
    if (size) {
        *size = n;
    }
    return true;
}

bool dctAddressForProtocolType(server_protocol_type type, size_t* offset, size_t* size) {
    size_t offs = 0;
    size_t n = 0;
    switch (type) {
#if HAL_PLATFORM_CLOUD_TCP
    case TCP_SERVER_PROTOCOL:
        offs = DCT_SERVER_ADDRESS_OFFSET;
        n = DCT_SERVER_ADDRESS_SIZE;
        break;
#endif // HAL_PLATFORM_CLOUD_TCP
#if HAL_PLATFORM_CLOUD_UDP
    case UDP_SERVER_PROTOCOL:
        offs = DCT_ALT_SERVER_ADDRESS_OFFSET;
        n = DCT_ALT_SERVER_ADDRESS_SIZE;
        break;
#endif // HAL_PLATFORM_CLOUD_UDP
    default:
        return false;
    }
    if (offset) {
        *offset = offs;
    }
    if (size) {
        *size = n;
    }
    return true;
}

} // namespace

int lock_security_key_data(security_key_type type, const char** data, size_t* size) {
    // TODO: Ensure missing public keys are extracted from private keys (fetch_device_public_key() is
    // currently broken on Electron)
    size_t offs = 0;
    size_t n = 0;
    if (!dctAddressForKeyType(type, &offs, &n)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    const char* d = (const char*)dct_read_app_data_lock(offs);
    if (!d) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    if (data) {
        *data = d;
    }
    if (size) {
        *size = n; // TODO: Return actual size of the key data?
    }
    return 0;
}

void unlock_security_key_data(security_key_type type) {
    size_t offs = 0;
    if (dctAddressForKeyType(type, &offs, nullptr)) {
        dct_read_app_data_unlock(offs);
    }
}

int store_security_key_data(security_key_type type, const char* data, size_t size) {
    size_t offs = 0;
    size_t maxSize = 0;
    if (!dctAddressForKeyType(type, &offs, &maxSize)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (size > maxSize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    const std::unique_ptr<char[]> buf(new(std::nothrow) char[maxSize]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    memcpy(buf.get(), data, size);
    memset(buf.get() + size, 0xff, maxSize - size);
    return dct_write_app_data(buf.get(), offs, maxSize);
}

int load_server_address(server_protocol_type type, ServerAddress* addr) {
    size_t offs = 0;
    size_t size = 0;
    if (!dctAddressForProtocolType(type, &offs, &size)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    const auto data = (const uint8_t*)dct_read_app_data_lock(offs);
    if (!data) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    parseServerAddressData(addr, data, size);
    dct_read_app_data_unlock(offs);
    if (addr->addr_type == INVALID_INTERNET_ADDRESS) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    return 0;
}

int store_server_address(server_protocol_type type, const ServerAddress* addr) {
    size_t offs = 0;
    size_t maxSize = 0;
    if (!dctAddressForProtocolType(type, &offs, &maxSize)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    const std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[maxSize]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    memset(buf.get(), 0xff, maxSize);
    const int ret = encodeServerAddressData(addr, buf.get(), maxSize);
    if (ret < 0) {
        return ret;
    }
    return dct_write_app_data(buf.get(), offs, maxSize);
}
