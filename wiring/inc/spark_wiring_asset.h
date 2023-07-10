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

class ApplicationAsset: public Stream {
public:
    ApplicationAsset() = default;
    ApplicationAsset(const asset_manager_asset* asset);
    virtual ~ApplicationAsset() = default;

    String name() const;
    AssetHash hash() const;
    size_t size() const;

    bool isValid() const;
    bool isReadable();

    int available() override;
    int read() override;
    int peek() override;
    // Not present in Stream
    virtual int read(char* buffer, size_t size);
    virtual int peek(char* buffer, size_t size);
    virtual int skip(size_t size);
    //
    void flush() override;
    // Print
    size_t write(uint8_t c) override;

    bool operator==(const ApplicationAsset& other) const;
    bool operator!=(const ApplicationAsset& other) const;

    void reset();

private:
    int prepareForReading(bool keepOpen = true);

private:
    String name_;
    AssetHash hash_;
    size_t size_ = 0;
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
using particle::AssetHash;

typedef void (*OnAssetsOtaCallback)(spark::Vector<ApplicationAsset> assets, void* context);
typedef std::function<void(spark::Vector<ApplicationAsset> assets)> OnAssetsOtaStdFunc;

// FIXME: there is an issue with System construction, so storage for the hook has
// to be initialized in a singleton-like manner to avoid global constructor initialization
// order issues.
inline OnAssetsOtaStdFunc& onAssetsOtaHookStorage() {
    static OnAssetsOtaStdFunc f;
    return f;
}

#endif // HAL_PLATFORM_ASSETS

