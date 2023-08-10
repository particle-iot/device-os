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

#pragma once

#include <cstdint>
#include "spark_wiring_vector.h"
#include "buffer.h"
#include "spark_wiring_string.h"
#include "ota_flash_hal.h"
#include "stream.h"
#include "asset_manager_api.h"

namespace particle {

class Asset {
public:
    Asset() = default;
    Asset(const char* name, const AssetHash& hash, size_t size = 0, size_t storageSize = 0);

    const String& name() const;
    const AssetHash& hash() const;
    size_t size() const;
    size_t storageSize() const;

    bool isValid() const;

    bool operator==(const Asset& other) const;
    bool operator!=(const Asset& other) const;
private:
    String name_;
    AssetHash hash_;
    size_t size_ = 0;
    size_t storageSize_ = 0;
};

inline Asset::Asset(const char* name, const AssetHash& hash, size_t size, size_t storageSize)
        : name_(name),
          hash_(hash),
          size_(size),
          storageSize_(storageSize) {
}

inline const String& Asset::name() const {
    return name_;
}

inline const AssetHash& Asset::hash() const {
    return hash_;
}

inline size_t Asset::size() const {
    return size_;
}

inline size_t Asset::storageSize() const {
    return storageSize_;
}

inline bool Asset::isValid() const {
    return name_.length() > 0 && hash_.isValid();
}

inline bool Asset::operator==(const Asset& other) const {
    return (name_ == other.name() && hash_ == other.hash());
}

inline bool Asset::operator!=(const Asset& other) const {
    return !(*this == other);
}

class AssetReader {
public:
    AssetReader();

    int init(InputStream* stream);
    int init(const char* filename);

    int validate(bool full = true);

    // Values obtained by reading from fs
    bool isValid() const;
    Asset asset() const;
    size_t size() const;
    bool isCompressed() const;
    size_t originalSize() const;

    int assetStream(InputStream*& stream);

private:
    int calculateCrc(uint32_t* crc);

private:
    InputStream* stream_;
    Asset asset_;
    bool valid_;
    bool compressed_;
    size_t size_;
    size_t originalSize_;

    std::unique_ptr<InputStream> fileStream_;
    std::unique_ptr<InputStream> decompressorStream_;
    std::unique_ptr<InputStream> proxyStream_;

    size_t dataOffset_;
    size_t dataSize_;
};

class AssetManager {
public:
    static AssetManager& instance();

    int init();

    const Vector<Asset>& requiredAssets() const;
    const Vector<Asset>& availableAssets() const;
    Vector<Asset> missingAssets() const;
    Vector<Asset> unusedAssets() const;

    int storeAsset(const hal_module_t* module);

    static int requiredAssetsForModule(const hal_module_t* module, Vector<Asset>& assets);

    int setNotifyHook(asset_manager_notify_hook hook, void* context);
    int notifyIfNeeded();
    int setConsumerState(asset_manager_consumer_state state);
    int prepareForOta(size_t size, unsigned flags, int moduleFunction);

    int formatStorage(bool remount = false);

protected:
    AssetManager() = default;

private:
    int parseRequiredAssets();
    int parseAvailableAssets();
    int clearUnusedAssets();

private:
    Vector<Asset> requiredAssets_;
    Vector<Asset> availableAssets_;
    asset_manager_notify_hook hook_ = nullptr;
    void* hookContext_ = nullptr;
};

} // particle
