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

#include "hal_platform.h"

#if HAL_PLATFORM_ASSETS

#include "asset_manager.h"
#include "storage_streams.h"
#include "check.h"
#include "ota_flash_hal_impl.h"
#include "user_hal.h"
#include <memory>
#include "logging.h"
#include "flash_mal.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "system_cache.h"
#include "system_defs.h"

namespace particle {

namespace {

const size_t FREE_BLOCKS_REQUIRED = 16;

// TODO: move to using streams as well?
int parseAssetDependencies(Vector<Asset>& assets, hal_storage_id storageId, uintptr_t start, uintptr_t end) {
    for (uintptr_t offset = start; offset < end;) {
        module_info_extension_t ext = {};
        CHECK(hal_storage_read(storageId, offset, (uint8_t*)&ext, sizeof(ext)));
        SCOPE_GUARD({
            offset += ext.length;
        });
        if (ext.type == MODULE_INFO_EXTENSION_ASSET_DEPENDENCY) {
            Buffer assetExtBuf(ext.length + 1 /* NULL name terminator */);
            auto assetExt = (module_info_asset_dependency_ext_t*)assetExtBuf.data();
            CHECK_TRUE(assetExt, SYSTEM_ERROR_NO_MEMORY);
            CHECK(hal_storage_read(storageId, offset, (uint8_t*)assetExt, ext.length));
            if (assetExt->hash.type != MODULE_INFO_HASH_TYPE_SHA256) {
                LOG(WARN, "Unsupported hash type %u with length %u", assetExt->hash.type, assetExt->hash.length);
                continue;
            }
            if (assetExt->hash.type != AssetHash::DEFAULT) {
                LOG(WARN, "Unknown hash type for asset dependency: 0x%x", assetExt->hash.type);
            }
            CHECK_TRUE(assets.append(Asset(assetExt->name, AssetHash(assetExt->hash.hash, assetExt->hash.length))), SYSTEM_ERROR_NO_MEMORY);
        } else if (ext.type == MODULE_INFO_EXTENSION_END) {
            break;
        }
    }
    return 0;
}

int parseAssetInfo(InputStream* stream, size_t size, Asset& asset) {
    Buffer nameExtBuf;
    AssetHash hash;
    while (size > 0) {
        module_info_extension_t ext = {};
        CHECK(stream->peek((char*)&ext, sizeof(ext)));
        CHECK_TRUE(ext.length <= size, SYSTEM_ERROR_BAD_DATA);
        if (ext.type == MODULE_INFO_EXTENSION_NAME) {
            CHECK_TRUE(ext.length > sizeof(module_info_name_ext_t), SYSTEM_ERROR_BAD_DATA);
            nameExtBuf = Buffer(ext.length + 1 /* NULL name terminator */);
            auto nameExt = (module_info_name_ext_t*)nameExtBuf.data();
            CHECK_TRUE(nameExt, SYSTEM_ERROR_NO_MEMORY);
            CHECK(stream->peek((char*)nameExt, ext.length));
        } else if (ext.type == MODULE_INFO_EXTENSION_HASH) {
            CHECK_TRUE(ext.length >= sizeof(module_info_hash_ext_t), SYSTEM_ERROR_BAD_DATA);
            module_info_hash_ext_t hashExt = {};
            CHECK(stream->peek((char*)&hashExt, sizeof(hashExt)));
            hash = AssetHash((const uint8_t*)hashExt.hash.hash, hashExt.hash.length, (AssetHash::Type)hashExt.hash.type);
        } else if (ext.type == MODULE_INFO_EXTENSION_END) {
            break;
        }
        CHECK(stream->skip(ext.length));
        size -= ext.length;
    }
    if (nameExtBuf.data() && hash.isValid()) {
        auto nameExt = (module_info_name_ext_t*)nameExtBuf.data();
        asset = Asset(nameExt->name, hash);
        return 0;
    }
    return SYSTEM_ERROR_BAD_DATA;
}

} // anynomous

AssetManager& AssetManager::instance() {
    static AssetManager manager;
    return manager;
}

int AssetManager::init() {
    const auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_ASSET_STORAGE, nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));

    // Best effort: reporting what we can
    parseRequiredAssets();
    parseAvailableAssets();
    return 0;
}

const Vector<Asset>& AssetManager::requiredAssets() const {
    // No thread safety required here, these do not change during device runtime and a copy is returned
    return requiredAssets_;
}

const Vector<Asset>& AssetManager::availableAssets() const {
    // No thread safety required here, these do not change during device runtime and a copy is returned
    return availableAssets_;
}

Vector<Asset> AssetManager::missingAssets() const {
    Vector<Asset> missing;
    for (const auto& asset: requiredAssets()) {
        if (!availableAssets().contains(asset)) {
            // Best-effort
            missing.append(asset);
        }
    }
    if (missing.size() > 0) {
        LOG(INFO, "There are %u missing assets", missing.size());
        for (const auto& asset: missing) {
            LOG(INFO, "Missing %s (%s)", asset.name().c_str(), asset.hash().toString().c_str());
        }
    }
    return missing;
}

Vector<Asset> AssetManager::unusedAssets() const {
    Vector<Asset> unused;
    for (const auto& asset: availableAssets()) {
        if (!requiredAssets().contains(asset)) {
            // Best-effort
            unused.append(asset);
        }
    }
    return unused;
}

int AssetManager::requiredAssetsForModule(const hal_module_t* module, Vector<Asset>& assets) {
    CHECK_TRUE(module, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto storageId = HAL_STORAGE_ID_INVALID;
    if (module->bounds.location == MODULE_BOUNDS_LOC_INTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_INTERNAL_FLASH;
    } else if (module->bounds.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_EXTERNAL_FLASH;
    }
    CHECK_FALSE(storageId == HAL_STORAGE_ID_INVALID, SYSTEM_ERROR_INVALID_STATE);

    if (module->info.flags & MODULE_INFO_FLAG_PREFIX_EXTENSIONS) {
        // There are extensions in the prefix
        uintptr_t extensionsStart = (uintptr_t)module->info.module_start_address + module->module_info_offset + sizeof(module_info_t);
        CHECK(parseAssetDependencies(assets, storageId, extensionsStart, (uintptr_t)module->info.module_end_address));
    }
    // Suffix
    module_info_suffix_base_t suffix = {};
    uintptr_t suffixStart = (uintptr_t)module->info.module_end_address - sizeof(module_info_suffix_base_t);
    CHECK(hal_storage_read(storageId, suffixStart, (uint8_t*)&suffix, sizeof(suffix)));
    if (suffix.size > sizeof(module_info_suffix_base_t) + sizeof(module_info_extension_t) * 2) {
        // There are some additional extensions in suffix
        uintptr_t extensionsStart = (uintptr_t)module->info.module_end_address - suffix.size;
        CHECK(parseAssetDependencies(assets, storageId, extensionsStart, suffixStart));
    }

    return 0;
}

int AssetManager::parseRequiredAssets() {
    hal_user_module_descriptor desc = {};
    // This will perform any validation needed
    CHECK(hal_user_module_get_descriptor(&desc));
    Vector<Asset> assets;

    hal_module_t mod = {};
    memcpy(&mod.info, &desc.info, sizeof(desc.info));
    mod.bounds = module_user;

    CHECK(requiredAssetsForModule(&mod, assets));

    for (auto a: assets) {
        LOG(INFO, "Required asset name=%s hash=%s", a.name().c_str(), a.hash().toString().c_str());
    }

    requiredAssets_ = std::move(assets);
    return 0;
}

int AssetManager::parseAvailableAssets() {
    auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_ASSET_STORAGE, nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_INVALID_STATE);

    Vector<Asset> assets;

    const fs::FsLock lock(fs);

    lfs_dir_t dir = {};
    CHECK_FS(lfs_dir_open(&fs->instance, &dir, "/"));
    SCOPE_GUARD({
        lfs_dir_close(&fs->instance, &dir);
    });
    lfs_info info = {};
    while (true) {
        int r = CHECK_FS(lfs_dir_read(&fs->instance, &dir, &info));
        if (r != 1) {
            break;
        }
        if (info.type == LFS_TYPE_DIR) {
            continue;
        }
        AssetReader reader;
        r = reader.init(info.name);
        if (r) {
            LOG(WARN, "Failed to open asset %s", info.name);
        } else {
            reader.validate(false /* full */);
        }
        if (reader.isValid() && reader.size() == info.size) {
            if (!assets.append(reader.asset())) {
                LOG(WARN, "Failed to add asset %s to the list of available", info.name);
            }
        } else {
            LOG(WARN, "Invalid asset %s, removing", info.name);
            lfs_remove(&fs->instance, info.name);
        }
    }
    
    availableAssets_ = assets;
    return 0;
}

int AssetManager::clearUnusedAssets() {
    auto unused = unusedAssets();
    for (const auto& asset: unused) {
        LOG(INFO, "Removing unused asset %s (hash=%s)", asset.name().c_str(), asset.hash().toString().c_str());
        auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_ASSET_STORAGE, nullptr);
        CHECK_TRUE(fs, SYSTEM_ERROR_INVALID_STATE);
        const fs::FsLock lock(fs);
        CHECK_FS(lfs_remove(&fs->instance, asset.name().c_str()));
    }
    return 0;
}

int AssetManager::storeAsset(const hal_module_t* module) {
    CHECK_TRUE(module, SYSTEM_ERROR_INVALID_ARGUMENT);

    StorageHalInputStream stream(module->bounds.location, (uintptr_t)module->bounds.start_address + (uintptr_t)module->info.module_start_address,
            (uintptr_t)module->info.module_end_address - (uintptr_t)module->info.module_start_address + 4,
            module->module_info_offset);
    AssetReader reader;
    CHECK(reader.init(&stream));
    CHECK(reader.validate());
    CHECK_TRUE(reader.isValid(), SYSTEM_ERROR_BAD_DATA);

    auto info = reader.asset();
    LOG(INFO, "Storing asset %s (hash=%s) size=%u original size=%u", info.name().c_str(), info.hash().toString().c_str(), reader.size(), reader.originalSize());

    CHECK(clearUnusedAssets());

    CHECK(stream.seek(0));
    char tmp[256];

    const auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_ASSET_STORAGE, nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);

    lfs_file_t file = {};
    lfs_remove(&fs->instance, info.name().c_str());
    CHECK_FS(lfs_file_open(&fs->instance, &file, info.name().c_str(), LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND));
    SCOPE_GUARD({     
        lfs_file_close(&fs->instance, &file);
    });

    while (stream.availForRead() > 0) {
        int read = stream.read(tmp, sizeof(tmp));
        if (read < 0 && read != SYSTEM_ERROR_END_OF_STREAM) {
            return read;
        }
        if (read == SYSTEM_ERROR_END_OF_STREAM) {
            break;
        }
        CHECK_FS(lfs_file_write(&fs->instance, &file, tmp, (size_t)read));
    }

    CHECK(setConsumerState(ASSET_MANAGER_CONSUMER_STATE_WANT));
    return 0;
}

int AssetManager::setNotifyHook(asset_manager_notify_hook hook, void* context) {
    hook_ = hook;
    hookContext_ = context;
    return 0;
}

int AssetManager::notifyIfNeeded() {
    if (!hook_) {
        return 0;
    }
    asset_manager_consumer_state state = ASSET_MANAGER_CONSUMER_STATE_NONE;
    auto& cache = services::SystemCache::instance();
    CHECK(cache.get(services::SystemCacheKey::ASSET_MANAGER_CONSUMER_STATE, &state, sizeof(state)));
    if (state == ASSET_MANAGER_CONSUMER_STATE_WANT) {
        hook_(hookContext_);
    }

    return 0;
}

int AssetManager::setConsumerState(asset_manager_consumer_state state) {
    return services::SystemCache::instance().set(services::SystemCacheKey::ASSET_MANAGER_CONSUMER_STATE, &state, sizeof(state));
}

int AssetManager::prepareForOta(size_t size, unsigned flags, int moduleFunction) {
    auto fwUpdateFlags = FirmwareUpdateFlags::fromUnderlying(flags);
    if (fwUpdateFlags & FirmwareUpdateFlag::VALIDATE_ONLY) {
        return 0;
    }
    const size_t blockSize = EXTERNAL_FLASH_ASSET_STORAGE_SIZE / EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT;
    const size_t freeBlocksSize = FREE_BLOCKS_REQUIRED * blockSize;
    if (moduleFunction == MODULE_FUNCTION_ASSET && size > (EXTERNAL_FLASH_ASSET_STORAGE_SIZE - freeBlocksSize)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
#if HAL_PLATFORM_ASSETS_OTA_OVERLAP
    if (size > EXTERNAL_FLASH_OTA_SAFE_LENGTH) {
        LOG(WARN, "Receiving module of size %u (function = %d) will invalidate asset storage", size, moduleFunction);
        // We cannot receive an asset that will also cause the asset filesystem to be overwritten
        if (moduleFunction == MODULE_FUNCTION_ASSET) {
            return SYSTEM_ERROR_TOO_LARGE;
        }
        // Otherwise unmount/invalidate filesystem
        CHECK(formatStorage(false /* remount */));
    }
#endif // HAL_PLATFORM_ASSETS_OTA_OVERLAP
    return 0;
}

int AssetManager::formatStorage(bool remount) {
    const auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_ASSET_STORAGE, nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    filesystem_unmount(fs);
    filesystem_invalidate(fs);
    availableAssets_.clear();
    CHECK(filesystem_mount(fs));
    return 0;
}

// AssetReader
AssetReader::AssetReader()
        : stream_(nullptr),
          valid_(false),
          compressed_(false),
          size_(0),
          originalSize_(0),
          dataOffset_(0),
          dataSize_(0) {
}

int AssetReader::init(const char* filename) {
    auto stream = std::make_unique<FileInputStream>();
    CHECK_TRUE(stream.get(), SYSTEM_ERROR_NO_MEMORY);
    CHECK(stream->open(filename, FILESYSTEM_INSTANCE_ASSET_STORAGE));
    fileStream_ = std::move(stream);
    stream_ = fileStream_.get();
    return 0;
}

int AssetReader::init(InputStream* stream) {
    stream_ = stream;
    return 0;
}

int AssetReader::validate(bool full) {
    // Sanity checks
    CHECK_TRUE(stream_, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(stream_->availForRead() > (int)(sizeof(module_info_t) + sizeof(compressed_module_header) + sizeof(module_info_suffix_base_t) + sizeof(uint32_t)),
            SYSTEM_ERROR_NOT_ENOUGH_DATA);
    size_t moduleSize = CHECK(stream_->availForRead());

    // Prefix
    module_info_t prefix = {};
    CHECK(stream_->read((char*)&prefix, sizeof(prefix)));
    CHECK_TRUE(prefix.module_function == MODULE_FUNCTION_ASSET, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(prefix.flags & MODULE_INFO_FLAG_DROP_MODULE_INFO, SYSTEM_ERROR_BAD_DATA);
    CHECK_FALSE(prefix.flags & MODULE_INFO_FLAG_PREFIX_EXTENSIONS, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(moduleSize == ((uintptr_t)prefix.module_end_address - (uintptr_t)prefix.module_start_address + sizeof(uint32_t) /* CRC32 */), SYSTEM_ERROR_BAD_DATA);
    bool compressed = prefix.flags & MODULE_INFO_FLAG_COMPRESSED;

    // Comperssion header
    compressed_module_header compHeader = {};
    size_t origSize = 0;
    if (compressed) {
        CHECK(stream_->peek((char*)&compHeader, sizeof(compHeader)));
        CHECK_TRUE(compHeader.size >= sizeof(compHeader), SYSTEM_ERROR_BAD_DATA);
        CHECK_TRUE(compHeader.method == 0, SYSTEM_ERROR_BAD_DATA);
        origSize = compHeader.original_size;
    }
    // Base suffix
    CHECK(stream_->seek(0));
    CHECK_TRUE(stream_->availForRead() == (int)moduleSize, SYSTEM_ERROR_IO);
    module_info_suffix_base_t suffix = {};
    CHECK(stream_->skipAll(stream_->availForRead() - sizeof(uint32_t) - sizeof(module_info_suffix_base_t)));
    CHECK(stream_->read((char*)&suffix, sizeof(suffix)));
    // CRC and check integrity
    uint32_t crc = 0;
    CHECK(stream_->read((char*)&crc, sizeof(crc)));
    CHECK(stream_->seek(0));
    if (full) {
        uint32_t calculatedCrc = 0;
        CHECK(calculateCrc(&calculatedCrc));
        CHECK_TRUE(calculatedCrc == bigEndianToNative(crc), SYSTEM_ERROR_BAD_DATA);
    }
    CHECK(stream_->seek(0));
    // Suffix extensions
    CHECK_TRUE(moduleSize >= sizeof(module_info_t) + sizeof(compressed_module_header) + suffix.size + sizeof(uint32_t), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(suffix.size >= sizeof(module_info_suffix_base_t) + 2 * sizeof(module_info_extension_t), SYSTEM_ERROR_BAD_DATA);
    CHECK(stream_->seek(moduleSize - suffix.size - sizeof(uint32_t)));
    Asset asset;
    CHECK(parseAssetInfo(stream_, suffix.size - sizeof(module_info_suffix_base_t), asset));
    valid_ = true;
    compressed_ = compressed;
    if (compressed) {
        dataOffset_ = sizeof(module_info_t) + compHeader.size;
    }
    dataSize_ = moduleSize - suffix.size - sizeof(uint32_t) - dataOffset_;
    size_ = moduleSize;
    originalSize_ = compressed ? origSize : (moduleSize - sizeof(module_info_t) - sizeof(uint32_t) - suffix.size);
    asset_ = Asset(asset.name(), asset.hash(), originalSize_, moduleSize);
    return 0;
}

int AssetReader::calculateCrc(uint32_t* crc) {
    size_t toRead = stream_->availForRead() - sizeof(uint32_t);
    char tmp[256];
    uint32_t tmpCrc = 0;
    while (toRead > 0) {
        size_t readSize = std::min(toRead, sizeof(tmp));
        CHECK(stream_->read(tmp, readSize));
        toRead -= readSize;
        tmpCrc = Compute_CRC32((const uint8_t*)tmp, readSize, &tmpCrc);
    }
    *crc = tmpCrc;
    return 0;
}

bool AssetReader::isValid() const {
    return valid_;
}

Asset AssetReader::asset() const {
    return asset_;
}

size_t AssetReader::size() const {
    return size_;
}

bool AssetReader::isCompressed() const {
    return compressed_;
}

size_t AssetReader::originalSize() const {
    return originalSize_;
}

int AssetReader::assetStream(InputStream*& stream) {
    CHECK_TRUE(isValid(), SYSTEM_ERROR_INVALID_STATE);

    if (!proxyStream_) {
        proxyStream_ = std::make_unique<ProxyInputStream>(stream_, dataOffset_, dataSize_);
    }
    CHECK_TRUE(proxyStream_, SYSTEM_ERROR_NO_MEMORY);

    if (!isCompressed()) {
        stream = proxyStream_.get();
        return 0;
    }

    if (!decompressorStream_) {
        auto stream = std::make_unique<InflatorStream>(proxyStream_.get(), originalSize_);
        CHECK_TRUE(stream, SYSTEM_ERROR_NO_MEMORY);
        CHECK(stream->init());
        decompressorStream_ = std::move(stream);
    }

    stream = decompressorStream_.get();
    return 0;
}

} // particle

#endif // HAL_PLATFORM_ASSETS
