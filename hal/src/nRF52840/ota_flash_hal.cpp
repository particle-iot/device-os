/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "ota_flash_hal.h"
#include "ota_flash_hal_impl.h"
#include "dct_hal.h"
#include "dct.h"
#include "handshake.h"
#include "core_hal.h"
#include "flash_mal.h"
#include "dct_hal.h"
#include "dsakeygen.h"
#include "eckeygen.h"
#include <cstring>
#include "parse_server_address.h"
#include "spark_macros.h"
#include "bootloader.h"
#include "ota_module.h"
#include "spark_protocol_functions.h"
#include "hal_platform.h"
#include "hal_event.h"
#include "rng_hal.h"
#include "service_debug.h"
#include "spark_wiring_random.h"
#include "delay_hal.h"
// For ATOMIC_BLOCK
#include "spark_wiring_interrupts.h"
#include "hal_platform.h"
#include "platform_ncp.h"
#include "deviceid_hal.h"
#include <memory>

#define OTA_CHUNK_SIZE                 (512)
#define BOOTLOADER_RANDOM_BACKOFF_MIN  (200)
#define BOOTLOADER_RANDOM_BACKOFF_MAX  (1000)

static hal_update_complete_t flash_bootloader(hal_module_t* mod, uint32_t moduleLength);

inline bool matches_mcu(uint8_t bounds_mcu, uint8_t actual_mcu) {
	return bounds_mcu==HAL_PLATFORM_MCU_ANY || actual_mcu==HAL_PLATFORM_MCU_ANY || (bounds_mcu==actual_mcu);
}

/**
 * Finds the location where a given module is stored. The module is identified
 * by its function and index.
 * @param module_function   The function of the module to find.
 * @param module_index      The function index of the module to find.
 * @param mcu_identifier	The mcu identifier that is the target for the module.
 * @return the module_bounds corresponding to the module, NULL when not found.
 */
const module_bounds_t* find_module_bounds(uint8_t module_function, uint8_t module_index, uint8_t mcu_identifier)
{
    for (unsigned i=0; i<module_bounds_length; i++) {
        if (module_bounds[i]->module_function==module_function && module_bounds[i]->module_index==module_index && matches_mcu(module_bounds[i]->mcu_identifier, mcu_identifier))
            return module_bounds[i];
    }
    return NULL;
}

void set_key_value(key_value* kv, const char* key, const char* value)
{
    kv->key = key;
    strncpy(kv->value, value, sizeof(kv->value)-1);
}

void copy_dct(void* target, uint16_t offset, uint16_t length) 
{
    dct_read_app_data_copy(offset, target, length);
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

int fetch_device_private_key_ex(uint8_t *key_buf, uint16_t length)
{
    return dct_read_app_data_copy(HAL_Feature_Get(FEATURE_CLOUD_UDP) ? DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET : DCT_DEVICE_PRIVATE_KEY_OFFSET,
                                    key_buf, length); 
}

int fetch_device_public_key_ex(void)
{
    uint8_t pubkey[DCT_DEVICE_PUBLIC_KEY_SIZE];
    memset(pubkey, 0, sizeof(pubkey));
    bool udp = false;
#if HAL_PLATFORM_CLOUD_UDP
    udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#endif
    uint8_t privkey[DCT_DEVICE_PRIVATE_KEY_SIZE];
    memset(privkey, 0, sizeof(privkey));
    if (fetch_device_private_key_ex(privkey, sizeof(privkey)))
    {
        return -1;
    }
    int extracted_length = 0;
#if HAL_PLATFORM_CLOUD_UDP
    if (udp) {
        extracted_length = extract_public_ec_key(pubkey, sizeof(pubkey), privkey);
    }
#endif
#if HAL_PLATFORM_CLOUD_TCP
    if (!udp) {
        extract_public_rsa_key(pubkey, privkey);
        extracted_length = DCT_DEVICE_PUBLIC_KEY_SIZE;
    }
#endif
    int offset = udp ? DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET : DCT_DEVICE_PUBLIC_KEY_OFFSET;
    size_t key_size = udp ? DCT_ALT_DEVICE_PUBLIC_KEY_SIZE : DCT_DEVICE_PUBLIC_KEY_SIZE;
    uint8_t flash_pub_key[DCT_DEVICE_PUBLIC_KEY_SIZE];
    memset(flash_pub_key, 0, sizeof(flash_pub_key));
    if (dct_read_app_data_copy(offset, flash_pub_key, DCT_DEVICE_PUBLIC_KEY_SIZE))
    {
        return -2;
    }

    if ((extracted_length > 0) && memcmp(pubkey, flash_pub_key, sizeof(pubkey))) {
        if (dct_write_app_data(pubkey, offset, key_size))
        {
            return -3;
        }
    }
    return 0; // flash_pub_key
}

void HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved)
{
    if (construct) {
        info->platform_id = PLATFORM_ID;
        uint8_t count = module_bounds_length;
        info->modules = new hal_module_t[count];
        if (info->modules) {
            info->module_count = count;
#if defined(HYBRID_BUILD)
            bool hybrid_module_found = false;
#endif
            for (unsigned i=0; i<count; i++) {
                fetch_module(info->modules+i, module_bounds[i], false, MODULE_VALIDATION_INTEGRITY);
#if defined(HYBRID_BUILD)
#ifndef MODULAR_FIRMWARE
#error HYBRID_BUILD must be modular
#endif
                static module_info_t hybrid_info;
                // change monolithic firmware to modular in the hybrid build.
                if (!hybrid_module_found && info->modules[i].info->module_function == MODULE_FUNCTION_MONO_FIRMWARE) {
                    memcpy(&hybrid_info, info->modules[i].info, sizeof(hybrid_info));
                    info->modules[i].info = &hybrid_info;
                    hybrid_info.module_function = MODULE_FUNCTION_SYSTEM_PART;
                    hybrid_info.module_index = 1; 
                    hybrid_module_found = true;
                }
#endif // HYBRID_BUILD
            }
        }
        HAL_OTA_Add_System_Info(info, construct, reserved);
    }
    else
    {
        HAL_OTA_Add_System_Info(info, construct, reserved);
        delete info->modules;
        info->modules = NULL;
    }
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
                const module_bounds_t* dependency_bounds = find_module_bounds(module->dependency.module_function, module->dependency.module_index, module_mcu_target(module));
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
                const module_bounds_t* dependency_bounds = find_module_bounds(module->dependency2.module_function, module->dependency2.module_index, module_mcu_target(module));
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
    const module_bounds_t* bounds = find_module_bounds(MODULE_FUNCTION_USER_PART, 1, HAL_PLATFORM_MCU_DEFAULT);
    return validate_module_dependencies(bounds, false, false);
}

bool HAL_OTA_CheckValidAddressRange(uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length;

#ifdef USE_SERIAL_FLASH
    if (startAddress == EXTERNAL_FLASH_OTA_ADDRESS && endAddress <= 0x400000)
    {
        return true;
    }
#else
    if (startAddress == INTERNAL_FLASH_OTA_ADDRESS && endAddress < 0x0100000)
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

int HAL_FLASH_OTA_Validate(hal_module_t* mod, bool userDepsOptional, module_validation_flags_t flags, void* reserved)
{
    hal_module_t module;

    bool module_fetched = fetch_module(&module, &module_ota, userDepsOptional, flags);

    if (mod) 
    {
        memcpy(mod, &module, sizeof(hal_module_t));
    }

    return (int)!module_fetched;
}

hal_update_complete_t HAL_FLASH_End(hal_module_t* mod)
{
    hal_module_t module;
    hal_update_complete_t result = HAL_UPDATE_ERROR;

    bool module_fetched = !HAL_FLASH_OTA_Validate(&module, true, (module_validation_flags_t)(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL), NULL);
	LOG(INFO, "module fetched %d, checks=%x, result=%x", module_fetched, module.validity_checked, module.validity_result);
    if (module_fetched && (module.validity_checked==module.validity_result))
    {
    	uint8_t mcu_identifier = module_mcu_target(module.info);
    	uint8_t current_mcu_identifier = platform_current_ncp_identifier();
    	if (mcu_identifier==HAL_PLATFORM_MCU_DEFAULT) {
            uint32_t moduleLength = module_length(module.info);
            module_function_t function = module_function(module.info);
            // bootloader is copied directly

			if (function==MODULE_FUNCTION_BOOTLOADER) {
				result = flash_bootloader(&module, moduleLength);
			}
			else
			{
				if (FLASH_AddToNextAvailableModulesSlot(FLASH_SERIAL, EXTERNAL_FLASH_OTA_ADDRESS,
					FLASH_INTERNAL, uint32_t(module.info->module_start_address),
					(moduleLength + 4),//+4 to copy the CRC too
					function,
					MODULE_VERIFY_CRC|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_FUNCTION)) { //true to verify the CRC during copy also
						result = HAL_UPDATE_APPLIED_PENDING_RESTART;
						DEBUG("OTA module applied - device will restart");
						FLASH_End();
				}
			}
    	}

    	else if (mcu_identifier==current_mcu_identifier) {
#if HAL_PLATFORM_NCP_UPDATABLE
    		result = platform_ncp_update_module(&module);
#else
    		LOG(ERROR, "NCP module is not updatable on this platform");
#endif
		}
    	else {
    		LOG(ERROR, "NCP module is not for this platform. module NCP: %x, current NCP: %x", mcu_identifier, current_mcu_identifier);
    	}
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


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
	bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
    uint8_t data[DCT_SERVER_ADDRESS_SIZE];
    memset(data, 0, DCT_SERVER_ADDRESS_SIZE);
    dct_read_app_data_copy(udp ? DCT_ALT_SERVER_ADDRESS_OFFSET : DCT_SERVER_ADDRESS_OFFSET,
                            data, DCT_SERVER_ADDRESS_SIZE);
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
    fetch_device_public_key_ex();
	bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
	if (udp)
	    copy_dct(keyBuffer, DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
	else
		copy_dct(keyBuffer, DCT_SERVER_PUBLIC_KEY_OFFSET, EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
}

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
        if (!error) 
        {
            if (udp)
                dct_write_app_data(keyBuffer, DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, DCT_ALT_DEVICE_PRIVATE_KEY_SIZE);
            else
                dct_write_app_data(keyBuffer, DCT_DEVICE_PRIVATE_KEY_OFFSET, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
			// refetch and rewrite public key to ensure it is valid
			fetch_device_public_key_ex();
            generated = true;
        }
        hal_notify_event(HAL_EVENT_GENERATE_DEVICE_KEY, HAL_EVENT_FLAG_STOP, nullptr);
    }
    genspec->generated_key = generated;
    return 0;
}

uint16_t HAL_Set_Claim_Code(const char* code)
{
    if (code)
    {
        return dct_write_app_data(code, DCT_CLAIM_CODE_OFFSET, DCT_CLAIM_CODE_SIZE);
    }
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
    if (len>DCT_CLAIM_CODE_SIZE) 
    {
        dct_read_app_data_copy(DCT_CLAIM_CODE_OFFSET, buffer, DCT_CLAIM_CODE_SIZE);
        buffer[DCT_CLAIM_CODE_SIZE] = 0;
    }
    else 
    {
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

void HAL_FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer, bool udp)
{
    if (udp) 
    {
        dct_write_app_data(keyBuffer, DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
    } else 
    {
        dct_write_app_data(keyBuffer, DCT_SERVER_PUBLIC_KEY_OFFSET, EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
    }
}

void HAL_FLASH_Write_ServerAddress(const uint8_t *buf, bool udp)
{
    if (udp) 
    {
        dct_write_app_data(buf, DCT_ALT_SERVER_ADDRESS_OFFSET, DCT_ALT_SERVER_ADDRESS_SIZE);
    } 
    else 
    {
        dct_write_app_data(buf, DCT_SERVER_ADDRESS_OFFSET, DCT_SERVER_ADDRESS_SIZE);
    }
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

int fetch_system_properties(key_value* storage, int keyCount, uint16_t flags) {
	int keys = 0;

	if (storage) {
		if (!(flags & HAL_SYSTEM_INFO_FLAGS_CLOUD) && keyCount && 0<hal_get_device_secret(storage[keys].value, sizeof(storage[0].value), nullptr)) {
			storage[keys].key = "ms";
			keyCount--;
			keys++;
		}
		if (!(flags & HAL_SYSTEM_INFO_FLAGS_CLOUD) && keyCount && 0<hal_get_device_serial_number(storage[keys].value, sizeof(storage[0].value), nullptr)) {
			storage[keys].key = "sn";
			keyCount--;
			keys++;
		}
		return keys;
	}
	else {
		return 2;
	}
}

int add_system_properties(hal_system_info_t* info, bool create, size_t additional) {
	uint16_t flags = 0;
	if (info->size >= sizeof(hal_system_info_t::flags) + offsetof(hal_system_info_t, flags)) {
		flags = info->flags;
	}

	if (create) {
		int keyCount = fetch_system_properties(nullptr, 0, flags);
		info->key_values = new key_value[keyCount+additional];
		info->key_value_count = fetch_system_properties(info->key_values, keyCount, flags);
		return info->key_value_count;
	} else {
		delete[] info->key_values;
		info->key_values = nullptr;
		return 0;
	}
}
