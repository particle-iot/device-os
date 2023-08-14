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

#include "hal_platform.h"
#include "asset_manager_api.h"
#include "spark_wiring_stream.h"
#include <memory>
#include "spark_wiring_vector.h"
#include <functional>

namespace particle {

/**
 * Application asset.
 * 
 * Instances of this class are returned in `System.assetsAvailable()` and `System.assetsRequired()`
 */
class ApplicationAsset: public Stream {
public:
    /**
     * Default constructor.
     * 
     * Construct a new Application Asset object.
     */
    ApplicationAsset() = default;

    /**
     * Construct a new Application Asset object from system asset manager API asset.
     * 
     * Internal use only.
     * 
     * @param asset `asset_manager_asset` struct obtained from `asset_manager_get_info()`
     */
    ApplicationAsset(const asset_manager_asset* asset);

    /**
     * Destroy the Application Asset object.
     * 
     */
    virtual ~ApplicationAsset() = default;

    /**
     * Get asset name (filename).
     * 
     * @return String Asset name (filename).
     */
    String name() const;

    /**
     * Get asset hash.
     * 
     * @return AssetHash Asset hash.
     */
    AssetHash hash() const;

    /**
     * Get asset size (in bytes).
     * 
     * @return size_t Asset size in bytes.
     */
    size_t size() const;

    /**
     * Get asset 'storage' size taking into account compression and metadata.
     * 
     * @return size_t Asset size in asset storage.
     */
    size_t storageSize() const;

    /**
     * Check if asset is valid.
     * 
     * @return true Asset is valid.
     * @return false Asset is not valid.
     */

    bool isValid() const;

    /**
     * Check if asset is readable.
     * 
     * @return true Asset is readable.
     * @return false Asset is not readable.
     */
    bool isReadable();

    /**
     * Get number of bytes readily available to be peeked/read/consumed.
     * 
     * @return int Number of bytes readily available to be peeked/read/consumed.
     */
    int available() override;

    /**
     * Read one byte.
     * 
     * @return int Positive values are data, negative values are values from `system_error_t`.
     */
    int read() override;

    /**
     * Peek one byte.
     * 
     * @return int Positive values are data, negative values are values from `system_error_t`.
     */
    int peek() override;

    // Not present in Stream
    /**
     * Read asset data up to `size` bytes.
     * 
     * The `buffer` will be filled up to `size` completely (if EOF is not reached), even if
     * `available` currently reports less than requested `size`.
     * 
     * @param buffer Pointer to the buffer to be filled.
     * @param size Size of the buffer / number of requested bytes.
     * @return int Filled size on success, `system_error_t` error code on error.
     */
    virtual int read(char* buffer, size_t size);

    /**
     * Peek asset data up to `size` bytes.
     * 
     * The `buffer` may be partially filled depending on the current value
     * of readily `available()` bytes.
     * 
     * @param buffer Pointer to the buffer to be filled.
     * @param size Size of the buffer.
     * @return int Filled size on success, `system_error_t` error code on error.
     */
    virtual int peek(char* buffer, size_t size);

    /**
     * Skip `size` bytes.
     * 
     * @param size Number of bytes to skip.
     * @return int Number of bytes skipped or `system_error_t` error code.
     */
    virtual int skip(size_t size);

    /**
     * No-op, conforming to `Stream` interface.
     * 
     */
    void flush() override;
    /**
     * No-op, conforming to `Print` interface.
     * 
     * @param c 
     * @return size_t 
     */
    size_t write(uint8_t c) override;

    /**
     * Comparison operators. Assets are equal if their name and hash match or
     * they are both empty/invalid/default-cosntructed.
     * 
     * @param other 
     * @return true 
     * @return false 
     */
    ///@{
    bool operator==(const ApplicationAsset& other) const;
    bool operator!=(const ApplicationAsset& other) const;
    ///@}

    /**
     * Reset internal state of the asset reader - start reading from the beginning.
     * 
     */
    void reset();

private:
    int prepareForReading(bool keepOpen = true);

private:
    String name_;
    AssetHash hash_;
    size_t size_ = 0;
    size_t storageSize_ = 0;
    bool eof_ = false;

    struct Data {
        Data() = delete;
        Data(asset_manager_stream* s);
        ~Data();
        asset_manager_stream* stream;
    };
    std::shared_ptr<Data> data_;
};

} // particle

#if HAL_PLATFORM_ASSETS
using particle::ApplicationAsset;

/**
 * AssetHash.
 * 
 */
using particle::AssetHash;

/**
 * Callbacks used in `System.onAssetOta()` to be executed early on boot before `setup()`
 * but after global constructors.
 * 
 */
///@{
typedef void (*OnAssetOtaCallback)(spark::Vector<ApplicationAsset> assets, void* context);
typedef std::function<void(spark::Vector<ApplicationAsset> assets)> OnAssetOtaStdFunc;
///@}

// FIXME: there is an issue with System construction, so storage for the hook has
// to be initialized in a singleton-like manner to avoid global constructor initialization
// order issues.
inline OnAssetOtaStdFunc& onAssetOtaHookStorage() {
    static OnAssetOtaStdFunc f;
    return f;
}

#endif // HAL_PLATFORM_ASSETS

