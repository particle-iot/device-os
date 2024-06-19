/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "system_info_encoding.h"

#if HAL_PLATFORM_PROTOBUF

#include "control/common.h"
#include "cloud/describe.pb.h"
#include "security_mode.h"

#define PB(name) particle_cloud_##name

using namespace particle;
using namespace particle::system;
using particle::control::common::EncodedString;

namespace {

PB(FirmwareModuleType) moduleFunctionToPb(module_function_t func) {
    switch (func) {
    case MODULE_FUNCTION_RESOURCE:
        return PB(FirmwareModuleType_RESOURCE_MODULE);
    case MODULE_FUNCTION_BOOTLOADER:
        return PB(FirmwareModuleType_BOOTLOADER_MODULE);
    case MODULE_FUNCTION_MONO_FIRMWARE:
        return PB(FirmwareModuleType_MONO_FIRMWARE_MODULE);
    case MODULE_FUNCTION_SYSTEM_PART:
        return PB(FirmwareModuleType_SYSTEM_PART_MODULE);
    case MODULE_FUNCTION_USER_PART:
        return PB(FirmwareModuleType_USER_PART_MODULE);
    case MODULE_FUNCTION_SETTINGS:
        return PB(FirmwareModuleType_SETTINGS_MODULE);
    case MODULE_FUNCTION_NCP_FIRMWARE:
        return PB(FirmwareModuleType_NCP_FIRMWARE_MODULE);
    case MODULE_FUNCTION_RADIO_STACK:
        return PB(FirmwareModuleType_RADIO_STACK_MODULE);
    case MODULE_FUNCTION_ASSET:
        return PB(FirmwareModuleType_ASSET_MODULE);
    default:
        return PB(FirmwareModuleType_INVALID_MODULE);
    }
}

PB(FirmwareModuleStore) moduleStoreToPb(module_store_t store) {
    switch (store) {
    case MODULE_STORE_FACTORY:
        return PB(FirmwareModuleStore_FACTORY_MODULE_STORE);
    case MODULE_STORE_BACKUP:
        return PB(FirmwareModuleStore_BACKUP_MODULE_STORE);
    case MODULE_STORE_SCRATCHPAD:
        return PB(FirmwareModuleStore_SCRATCHPAD_MODULE_STORE);
    case MODULE_STORE_MAIN:
    default:
        return PB(FirmwareModuleStore_MAIN_MODULE_STORE);
    }
}

#if HAL_PLATFORM_ASSETS

bool encodeAssetDependencies(pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
    auto assets = (const spark::Vector<Asset>*)*arg;
    for (const auto& asset: *assets) {
        PB(FirmwareModuleAsset) pbModuleAsset = {};
        EncodedString pbName(&pbModuleAsset.name);
        EncodedString pbHash(&pbModuleAsset.hash);
        // TODO: hash type
        pbName.data = asset.name().c_str();
        pbName.size = asset.name().length();

        pbHash.data = asset.hash().hash().data();
        pbHash.size = asset.hash().hash().size();

        pbModuleAsset.size = asset.size();
        pbModuleAsset.storage_size = asset.storageSize();

        if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModuleAsset_msg), &pbModuleAsset)) {
            return false;
        }
    }
    return true;
}

#endif // HAL_PLATFORM_ASSETS

} // anonymous

#if HAL_PLATFORM_PROTOBUF

EncodeFirmwareModules::EncodeFirmwareModules(pb_callback_t* cb, Flags flags)
        : sysInfo_{},
          flags_(flags) {
    sysInfo_.size = sizeof(sysInfo_);
    sysInfo_.flags = (flags & Flag::SYSTEM_INFO_CLOUD) ? HAL_SYSTEM_INFO_FLAGS_CLOUD : 0;
    system_info_get_unstable(&sysInfo_, 0, nullptr);

    cb->arg = this;
    cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
        const auto self = (const EncodeFirmwareModules*)*arg;
        for (unsigned i = 0; i < self->sysInfo_.module_count; ++i) {
            const auto& module = self->sysInfo_.modules[i];
            if (!is_module_function_valid((module_function_t)module.info.module_function)) {
                continue;
            }
            // If sending a describe, skip factory modules entirely just in case
            if (module.bounds.store == MODULE_STORE_FACTORY && (self->flags_ & Flag::SYSTEM_INFO_CLOUD)) {
                continue;
            }
            // if (module.bounds.store == MODULE_STORE_FACTORY && (module.validity_result & MODULE_VALIDATION_INTEGRITY) == 0) {
            //     continue; // See system_info_to_json()
            // }
            PB(FirmwareModule) pbModule = {};
            pbModule.type = moduleFunctionToPb((module_function_t)module.info.module_function);
            pbModule.index = module.info.module_index;
            pbModule.version = module.info.module_version;
            pbModule.store = moduleStoreToPb((module_store_t)module.bounds.store);
            pbModule.max_size = module.bounds.maximum_size;
            pbModule.size = (uintptr_t)module.info.module_end_address - (uintptr_t)module.info.module_start_address;
            if (pbModule.size > 0) {
                pbModule.size += sizeof(uint32_t) /* CRC */;
            }
            pbModule.checked_flags = module.validity_checked;
            pbModule.passed_flags = module.validity_result;
            EncodedString pbHash(&pbModule.hash);
            if (module.info.module_function == MODULE_FUNCTION_USER_PART || (self->flags_ & Flag::FORCE_SHA)) {
                pbHash.data = (const char*)module.suffix.sha;
                pbHash.size = sizeof(module.suffix.sha);
            }
            if (module.info.module_function == MODULE_FUNCTION_BOOTLOADER && (module.validity_result & MODULE_VALIDATION_INTEGRITY)) {
                module_info_security_mode_ext_t ext = {};
                ext.ext.length = sizeof(ext);
                if (!security_mode_find_module_extension(module.bounds.location == MODULE_BOUNDS_LOC_INTERNAL_FLASH ? HAL_STORAGE_ID_INTERNAL_FLASH : HAL_STORAGE_ID_EXTERNAL_FLASH,
                        module.bounds.start_address, &ext)) {
                    if (ext.security_mode == MODULE_INFO_SECURITY_MODE_PROTECTED) {
                        pbModule.security.mode = PB(FirmwareModuleSecurityMode_PROTECTED);
                    }
                }
            }
            pbModule.dependencies.arg = (void*)&module; // arg is a non-const pointer
            pbModule.dependencies.funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
                auto module = (const hal_module_t*)*arg;
                for (unsigned i = 0; i < 2; ++i) {
                    const auto& dep = (i == 0) ? module->info.dependency : module->info.dependency2;
                    PB(FirmwareModuleDependency) pbDep = {};
                    if (!is_module_function_valid((module_function_t)dep.module_function)) {
                        continue;
                    }
                    pbDep.type = moduleFunctionToPb((module_function_t)dep.module_function);
                    pbDep.index = dep.module_index;
                    pbDep.version = dep.module_version;
                    if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModuleDependency_msg), &pbDep)) {
                        return false;
                    }
                }
                return true;
            };
#if HAL_PLATFORM_ASSETS
            spark::Vector<Asset> assets;
            if ((module.info.module_function == MODULE_FUNCTION_USER_PART) && (module.validity_result & MODULE_VALIDATION_INTEGRITY)) {
                if (!AssetManager::requiredAssetsForModule(&module, assets) && assets.size() > 0) {
                    pbModule.asset_dependencies.arg = (void*)&assets;
                    pbModule.asset_dependencies.funcs.encode = encodeAssetDependencies;
                }
            }
#endif // HAL_PLATFORM_ASSETS
            if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModule_msg), &pbModule)) {
                return false;
            }
        }
        return true;
    };
}

EncodeFirmwareModules::~EncodeFirmwareModules() {
    system_info_free_unstable(&sysInfo_, nullptr);
}

#endif // HAL_PLATFORM_PROTOBUF

#if HAL_PLATFORM_ASSETS

EncodeAssets::EncodeAssets(pb_callback_t* cb, const Vector<Asset>& assets)
        : assets_(assets) {
    cb->arg = (void*)&this->assets_;
    cb->funcs.encode = encodeAssetDependencies;
}
#endif // HAL_PLATFORM_ASSETS

#endif // HAL_PLATFORM_PROTOBUF