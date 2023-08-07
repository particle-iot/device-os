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

#include "logging.h"

LOG_SOURCE_CATEGORY("hal.ota")

#include "ota_flash_hal.h"
#include "ota_flash_hal_impl.h"
#include "dct_hal.h"
#include "dct.h"
#include "core_hal.h"
#include "exflash_hal.h"
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
#include "platform_radio_stack.h"
#include "check.h"

#if HAL_PLATFORM_ASSETS
#include "asset_manager.h"
#endif // HAL_PLATFORM_ASSETS

#define OTA_CHUNK_SIZE                 (512)
#define BOOTLOADER_RANDOM_BACKOFF_MIN  (200)
#define BOOTLOADER_RANDOM_BACKOFF_MAX  (1000)

using namespace particle;

namespace {

const uint16_t BOOTLOADER_MBR_UPDATE_MIN_VERSION = 1001; // 2.0.0-rc.1

} // anonymous

static int flash_bootloader(const hal_module_t* mod, uint32_t moduleLength);

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
    bool udp = false;
#if HAL_PLATFORM_CLOUD_UDP
    udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#endif
    size_t key_size = udp ? DCT_ALT_DEVICE_PUBLIC_KEY_SIZE : DCT_DEVICE_PUBLIC_KEY_SIZE;
    uint8_t pubkey[key_size] = {};

    uint8_t privkey[DCT_DEVICE_PRIVATE_KEY_SIZE] = {};
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
    uint8_t flash_pub_key[key_size] = {};
    if (dct_read_app_data_copy(offset, flash_pub_key, key_size))
    {
        return -2;
    }

    if ((extracted_length > 0) && memcmp(pubkey, flash_pub_key, key_size)) {
        if (dct_write_app_data(pubkey, offset, key_size))
        {
            return -3;
        }
    }
    return 0; // flash_pub_key
}

int HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved)
{
    if (construct) {
        info->platform_id = PLATFORM_ID;
        uint8_t count = module_bounds_length;
        info->modules = new hal_module_t[count];
        if (info->modules) {
            info->module_count = count;
            bool user_module_found = false;
            memset(info->modules, 0, sizeof(hal_module_t) * count);
            for (unsigned i = 0; i < count; i++) {
                const auto bounds = module_bounds[i];
                const auto module = info->modules + i;
                // IMPORTANT: if both types of modules are present (legacy 128KB and newer 256KB),
                // 128KB application will take precedence and the newer 256KB application will not
                // be reported in the modules info. It will be missing from the System Describe,
                // 'serial inspect` and any other facility that uses HAL_System_Info().
                if (bounds->store == MODULE_STORE_MAIN && bounds->module_function == MODULE_FUNCTION_USER_PART && user_module_found) {
                    // Make sure that we report only single user part (either 128KB or 256KB) in the
                    // list of modules.
                    // Make sure to still report correct bounds structure, normally it gets taken care of by fetch_module
                    module->bounds = *bounds;
                    continue;
                }
                bool valid = fetch_module(module, bounds, false, MODULE_VALIDATION_INTEGRITY);
                // NOTE: fetch_module may return other validation flags in module->validity_checked
                // and module->validity_result
                // Here specifically we are only concerned whether the integrity check passes or not
                // and skip such 'broken' modules from module info
                valid = valid && (module->validity_result & MODULE_VALIDATION_INTEGRITY);
                if (bounds->store == MODULE_STORE_MAIN && bounds->module_function == MODULE_FUNCTION_USER_PART) {
                    if (valid) {
                        user_module_found = true;
                    } else {
                        // IMPORTANT: we should not be reporting invalid user module in the describe
                        // as it may contain garbage data and may be presented as for example
                        // system module with invalid dependency on some non-existent module
                        // FIXME: we should potentially do a similar thing for other module types
                        // but for now only tackling user modules
                        memset(module, 0, sizeof(*module));
                    }
                }
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
    return 0;
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
        const module_info_t* info = &smod.info;
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
    module_info_t moduleInfo = {};
    CHECK_TRUE(locate_module(bounds, &moduleInfo) == SYSTEM_ERROR_NONE, false);
    if (moduleInfo.dependency.module_function == MODULE_FUNCTION_NONE || (userOptional && module_function(&moduleInfo) == MODULE_FUNCTION_USER_PART)) {
        valid = true;
    } else {
        // deliberately not transitive, so we only check the first dependency
        // so only user->system_part_2 is checked
        if (moduleInfo.dependency.module_function != MODULE_FUNCTION_NONE) {
            // NOTE: we ignore MCU type
            const module_bounds_t* dependency_bounds = find_module_bounds(moduleInfo.dependency.module_function, moduleInfo.dependency.module_index, HAL_PLATFORM_MCU_ANY);
            if (!dependency_bounds) {
                return false;
            }
            module_info_t infoDep = {};
            int ret = locate_module(dependency_bounds, &infoDep);
            valid = (ret == SYSTEM_ERROR_NONE) && (infoDep.module_version >= moduleInfo.dependency.module_version);
        } else {
            valid = true;
        }
        // Validate dependency2
        if (moduleInfo.dependency2.module_function == MODULE_FUNCTION_NONE ||
            (moduleInfo.dependency2.module_function == MODULE_FUNCTION_BOOTLOADER && userOptional)) {
            valid = valid && true;
        } else {
            // NOTE: we ignore MCU type
            const module_bounds_t* dependency_bounds = find_module_bounds(moduleInfo.dependency2.module_function, moduleInfo.dependency2.module_index, HAL_PLATFORM_MCU_ANY);
            if (!dependency_bounds) {
                return false;
            }
            module_info_t infoDep;
            int ret = locate_module(dependency_bounds, &infoDep);
            valid = valid && (ret == SYSTEM_ERROR_NONE) && (infoDep.module_version >= moduleInfo.dependency2.module_version);
        }
    }

    if (fullDeps && valid) {
        valid = valid && validate_module_dependencies_full(&moduleInfo, bounds);
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
    const int r = FLASH_Begin(address, length);
    if (r != FLASH_ACCESS_RESULT_OK) {
        return false;
    }
    return true;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
    return FLASH_Update(pBuffer, address, length);
}

int HAL_OTA_Flash_Read(uintptr_t address, uint8_t* buffer, size_t size)
{
#ifdef USE_SERIAL_FLASH
    return hal_exflash_read(address, buffer, size);
#else
    return hal_flash_read(address, buffer, size);
#endif
}

static int flash_bootloader(const hal_module_t* mod, uint32_t moduleLength)
{
    bool ok = false;
    uint32_t attempt = 0;
    for (;;) {
        int fres = FLASH_ACCESS_RESULT_ERROR;
        if (attempt++ > 0) {
            // NOTE: we have to use app_util_ciritical_region_enter/exit here
            // because it does not block SoftDevice interrupts (which we are dependent
            // on for flash operations)
            uint8_t n;
            app_util_critical_region_enter(&n);
            // If it's not the first flashing attempt, try with interrupts disabled
            fres = bootloader_update((const void*)mod->bounds.start_address, moduleLength + 4);
            app_util_critical_region_exit(n);
        } else {
            fres = bootloader_update((const void*)mod->bounds.start_address, moduleLength + 4);
        }
        if (fres == FLASH_ACCESS_RESULT_OK) {
            // Validate bootloader
            hal_module_t module;
            bool module_fetched = fetch_module(&module, &module_bootloader, true, MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL);
            if (module_fetched && (module.validity_checked == module.validity_result)) {
                ok = true;
                break;
            }
        }

        if (fres == FLASH_ACCESS_RESULT_BADARG) {
            // The bootloader is still intact, for some reason the module being copied has failed some checks.
            break;
        }

        // Random backoff
        system_tick_t period = random(BOOTLOADER_RANDOM_BACKOFF_MIN, BOOTLOADER_RANDOM_BACKOFF_MAX);
        LOG_DEBUG(WARN, "Failed to flash bootloader. Retrying in %lu ms", period);
        HAL_Delay_Milliseconds(period);
    }
    return ok ? (int)HAL_UPDATE_APPLIED : SYSTEM_ERROR_UNKNOWN;
}

namespace {

int validityResultToSystemError(unsigned result, unsigned checked) {
    if (result == checked) {
        return 0;
    }
    if ((checked & MODULE_VALIDATION_INTEGRITY) && !(result & MODULE_VALIDATION_INTEGRITY)) {
        return SYSTEM_ERROR_OTA_INTEGRITY_CHECK_FAILED;
    }
    if ((checked & MODULE_VALIDATION_DEPENDENCIES) && !(result & MODULE_VALIDATION_DEPENDENCIES)) {
        return SYSTEM_ERROR_OTA_DEPENDENCY_CHECK_FAILED;
    }
    if ((checked & MODULE_VALIDATION_DEPENDENCIES_FULL) && !(result & MODULE_VALIDATION_DEPENDENCIES_FULL)) {
        return SYSTEM_ERROR_OTA_DEPENDENCY_CHECK_FAILED;
    }
    if ((checked & MODULE_VALIDATION_RANGE) && !(result & MODULE_VALIDATION_RANGE)) {
        return SYSTEM_ERROR_OTA_INVALID_ADDRESS;
    }
    if ((checked & MODULE_VALIDATION_PLATFORM) && !(result & MODULE_VALIDATION_PLATFORM)) {
        return SYSTEM_ERROR_OTA_INVALID_PLATFORM;
    }
    return SYSTEM_ERROR_OTA_VALIDATION_FAILED;
}

// TODO: Current design of the OTA subsystem and the protocol doesn't allow for updating
// multiple modules at once. As a "temporary" workaround, multiple modules can be combined
// into a single binary
int fetchModules(hal_module_t* modules, size_t maxModuleCount, bool userDepsOptional, unsigned flags) {
    hal_module_t module = {};
    module_bounds_t bounds = module_ota;
    size_t count = 0;
    bool hasNext = true;
    do {
        if (!fetch_module(&module, &bounds, userDepsOptional, flags)) {
            SYSTEM_ERROR_MESSAGE("Unable to fetch module");
            return SYSTEM_ERROR_OTA_MODULE_NOT_FOUND;
        }
        if (count < maxModuleCount) {
            memcpy(modules + count, &module, sizeof(hal_module_t));
        }
        ++count;
        const module_info_t& info = module.info;
        hasNext = (info.flags & MODULE_INFO_FLAG_COMBINED);
        if (hasNext) {
            // Forge a module bounds structure for the next module in the OTA region
            const size_t moduleSize = module_length(&info) + 4 /* CRC-32 */;
            if (bounds.maximum_size < moduleSize) {
                SYSTEM_ERROR_MESSAGE("Invalid module size");
                return SYSTEM_ERROR_OTA_INVALID_SIZE;
            }
            bounds.start_address += moduleSize;
            bounds.maximum_size -= moduleSize;
        }
    } while (hasNext);
    return count;
}

int validateModules(const hal_module_t* modules, size_t moduleCount) {
    for (size_t i = 0; i < moduleCount; ++i) {
        const auto module = modules + i;
        const auto info = &module->info;
        LOG(INFO, "Validating module; type: %u; index: %u; version: %u", (unsigned)module_function(info),
                (unsigned)module_index(info), (unsigned)module_version(info));
        if (module->validity_result != module->validity_checked) {
            SYSTEM_ERROR_MESSAGE("Validation failed; result: 0x%02x; checked: 0x%02x", (unsigned)module->validity_result,
                    (unsigned)module->validity_checked);
            return validityResultToSystemError(module->validity_result, module->validity_checked);
        }
        const bool dropModuleInfo = (info->flags & MODULE_INFO_FLAG_DROP_MODULE_INFO);
        const bool compressed = (info->flags & MODULE_INFO_FLAG_COMPRESSED);
        if (module->module_info_offset > 0 && (dropModuleInfo || compressed)) {
            // Module with the DROP_MODULE_INFO or COMPRESSED flag set can't have a vector table
            SYSTEM_ERROR_MESSAGE("Invalid module format");
            return SYSTEM_ERROR_OTA_INVALID_FORMAT;
        }
        const auto moduleFunc = module_function(info);
        if (compressed && moduleFunc != MODULE_FUNCTION_USER_PART && moduleFunc != MODULE_FUNCTION_SYSTEM_PART
                && moduleFunc != MODULE_FUNCTION_ASSET) {
            SYSTEM_ERROR_MESSAGE("Unsupported compressed module"); // TODO
            return SYSTEM_ERROR_OTA_UNSUPPORTED_MODULE;
        }
        if (moduleFunc == MODULE_FUNCTION_NCP_FIRMWARE) {
#if HAL_PLATFORM_NCP_UPDATABLE
            const auto moduleNcp = module_mcu_target(info);
            const int ncpCount = platform_ncp_count();
            bool found = false;
            for (int i = 0; i < ncpCount; i++) {
                PlatformNCPInfo info = {};
                const auto r = platform_ncp_get_info(i, &info);
                if (!r && info.identifier == moduleNcp && info.updatable) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                SYSTEM_ERROR_MESSAGE("NCP module is not for this platform; module NCP: 0x%02x",
                        (unsigned)moduleNcp);
                for (int i = 0; i < ncpCount; i++) {
                    PlatformNCPInfo info = {};
                    const auto r = platform_ncp_get_info(i, &info);
                    if (!r && info.updatable) {
                        LOG(ERROR, "Supported updatable NCP: 0x%02x", info.identifier);
                    }
                }
                return SYSTEM_ERROR_OTA_INVALID_PLATFORM;
            }
#else
            SYSTEM_ERROR_MESSAGE("NCP module is not updatable on this platform");
            return SYSTEM_ERROR_OTA_UNSUPPORTED_MODULE;
#endif // !HAL_PLATFORM_NCP_UPDATABLE
        }
    }
    return 0;
}

// TODO: Anything above 2 will almost certainly fail the dependency check
const size_t MAX_COMBINED_MODULE_COUNT = 2;

} // namespace

int HAL_FLASH_OTA_Validate(bool userDepsOptional, module_validation_flags_t flags, void* reserved)
{
    hal_module_t modules[MAX_COMBINED_MODULE_COUNT] = {};
    size_t moduleCount = CHECK(fetchModules(modules, MAX_COMBINED_MODULE_COUNT, userDepsOptional, flags));
    if (moduleCount == 0) { // Sanity check
        return SYSTEM_ERROR_OTA_MODULE_NOT_FOUND;
    }
    if (moduleCount > MAX_COMBINED_MODULE_COUNT) {
        LOG(WARN, "Got more than %u combined modules", (unsigned)MAX_COMBINED_MODULE_COUNT);
        moduleCount = MAX_COMBINED_MODULE_COUNT;
    }
    CHECK(validateModules(modules, moduleCount));
    return 0;
}

int HAL_FLASH_End(void* reserved)
{
    hal_module_t modules[MAX_COMBINED_MODULE_COUNT] = {};
    size_t moduleCount = CHECK(fetchModules(modules, MAX_COMBINED_MODULE_COUNT, true /* userDepsOptional */,
            MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL));
    if (moduleCount == 0) {
        return SYSTEM_ERROR_OTA_MODULE_NOT_FOUND;
    }
    if (moduleCount > MAX_COMBINED_MODULE_COUNT) {
        LOG(WARN, "Got more than %u combined modules", (unsigned)MAX_COMBINED_MODULE_COUNT);
        moduleCount = MAX_COMBINED_MODULE_COUNT;
    }
    CHECK(validateModules(modules, moduleCount));
    bool restartPending = false;
    for (size_t i = 0; i < moduleCount; ++i) {
        const auto module = &modules[i];
        module_info_t& info = module->info;
        const auto moduleFunc = module_function(&info);
        const auto moduleSize = module_length(&info);
        LOG(INFO, "Applying module; type: %u; index: %u; version: %u", (unsigned)moduleFunc, (unsigned)module_index(&info),
                (unsigned)module_version(&info));
        int result = SYSTEM_ERROR_UNKNOWN;
        switch (moduleFunc) {
#if HAL_PLATFORM_NCP_UPDATABLE
        case MODULE_FUNCTION_NCP_FIRMWARE: {
            result = platform_ncp_update_module(module);
            break;
        }
#endif // HAL_PLATFORM_NCP_UPDATABLE
#if HAL_PLATFORM_ASSETS
        case MODULE_FUNCTION_ASSET: {
            result = AssetManager::instance().storeAsset(module);
            break;
        }
#endif // HAL_PLATFORM_ASSETS
        case MODULE_FUNCTION_BOOTLOADER: {
            if (bootloader_get_version() < BOOTLOADER_MBR_UPDATE_MIN_VERSION) {
                result = flash_bootloader(module, moduleSize);
                break;
            }
            // We are going to put the bootloader module temporary into internal flash
            // in place of the application (saving application into a different location on external flash).
            // MODULE_VERIFY_DESTINATION_IS_START_ADDRESS check will normally fail for bootloaders that
            // don't support updates via MBR, so we are not going to corrupt anything.
            const uintptr_t endAddress = (uintptr_t)module_user_compat.start_address + ((uintptr_t)info.module_end_address - (uintptr_t)info.module_start_address);
            info.module_start_address = (const void*)module_user_compat.start_address;
            info.module_end_address = (const void*)endAddress;
        }
        // Fall through after we've patched the destination.
        default: { // User part, system part or a radio stack module or bootloader if MBR updates are supported
            uint8_t slotFlags = MODULE_VERIFY_CRC | MODULE_VERIFY_DESTINATION_IS_START_ADDRESS | MODULE_VERIFY_FUNCTION;
            if (info.flags & MODULE_INFO_FLAG_DROP_MODULE_INFO) {
                slotFlags |= MODULE_DROP_MODULE_INFO;
            }
            if (info.flags & MODULE_INFO_FLAG_COMPRESSED) {
                slotFlags |= MODULE_COMPRESSED;
            }
            const bool ok = FLASH_AddToNextAvailableModulesSlot(FLASH_SERIAL, module->bounds.start_address, FLASH_INTERNAL,
                    (uint32_t)info.module_start_address, moduleSize + 4 /* CRC-32 */, moduleFunc, slotFlags);
            if (!ok) {
                SYSTEM_ERROR_MESSAGE("No module slot available");
                result = SYSTEM_ERROR_NO_MEMORY;
            } else {
                result = HAL_UPDATE_APPLIED_PENDING_RESTART;
            }
            break;
        }
        }
        if (result < 0) {
            LOG(ERROR, "Unable to apply module");
            // TODO: Clear module slots in DCT?
            return result;
        }
        if (result == HAL_UPDATE_APPLIED_PENDING_RESTART) {
            restartPending = true;
        }
    }
    return restartPending ? HAL_UPDATE_APPLIED_PENDING_RESTART : HAL_UPDATE_APPLIED;
}

// Todo this code and much of the code here is duplicated between Gen2 and Gen3
// This should be factored out into directory shared by both platforms.
int HAL_FLASH_ApplyPendingUpdate(bool dryRun, void* reserved)
{
    uint8_t otaUpdateFlag = DCT_OTA_UPDATE_FLAG_CLEAR;
    STATIC_ASSERT(sizeof(otaUpdateFlag)==DCT_OTA_UPDATE_FLAG_SIZE, "expected ota update flag size to be 1");
    dct_read_app_data_copy(DCT_OTA_UPDATE_FLAG_OFFSET, &otaUpdateFlag, DCT_OTA_UPDATE_FLAG_SIZE);
    int result = SYSTEM_ERROR_UNKNOWN;
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
            result = HAL_FLASH_End(nullptr);
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

int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned data_length)
{
    unsigned offset = 0;
    unsigned length = 0;
    bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);

    switch (config_item)
    {
    case SYSTEM_CONFIG_DEVICE_KEY:
        if (udp) {
            offset = DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET;
            length = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE;
        } else {
            offset = DCT_DEVICE_PRIVATE_KEY_OFFSET;
            length = DCT_DEVICE_PRIVATE_KEY_SIZE;
        }
        break;
    case SYSTEM_CONFIG_SERVER_KEY:
        if (udp) {
            offset = DCT_ALT_SERVER_PUBLIC_KEY_OFFSET;
            // Server address for UDP devices is in a separate, adjacent DCT field unlike TCP devices.
            // To update the server address along with the key, we need to write more data.
            length = DCT_ALT_SERVER_PUBLIC_KEY_SIZE + DCT_ALT_SERVER_ADDRESS_SIZE;
        } else {
            offset = DCT_SERVER_PUBLIC_KEY_OFFSET;
            length = DCT_SERVER_PUBLIC_KEY_SIZE;
        }
        break;
    case SYSTEM_CONFIG_SERVER_ADDRESS:
        if (udp) {
            offset = DCT_ALT_SERVER_ADDRESS_OFFSET;
            length = DCT_ALT_SERVER_ADDRESS_SIZE;
        } else {
            offset = DCT_SERVER_ADDRESS_OFFSET;
            length = DCT_SERVER_ADDRESS_SIZE;
        }
        break;
    case SYSTEM_CONFIG_SOFTAP_PREFIX:
        offset = DCT_SSID_PREFIX_OFFSET;
        length = DCT_SSID_PREFIX_SIZE-1;
        if (data_length>(unsigned)length)
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

    if (length>0)
        dct_write_app_data(data, offset, std::min<size_t>(data_length, length));

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
