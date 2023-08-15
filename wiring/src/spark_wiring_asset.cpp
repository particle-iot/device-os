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

#include "spark_wiring_asset.h"
#include "check.h"

#if HAL_PLATFORM_ASSETS

using namespace particle;

ApplicationAsset::ApplicationAsset(const asset_manager_asset* asset)
        : ApplicationAsset() {
    if (!asset) {
        return;
    }
    if (asset->name) {
        name_ = String(asset->name);
    }
    if (asset->hash && asset->hash_length) {
        hash_ = AssetHash(asset->hash, asset->hash_length, (AssetHash::Type)asset->hash_type);
    }
    if (asset->size) {
        size_ = asset->size;
    }
    storageSize_ = asset->storage_size;
}

String ApplicationAsset::name() const {
    return name_;
}

AssetHash ApplicationAsset::hash() const {
    return hash_;
}

size_t ApplicationAsset::size() const {
    return size_;
}

size_t ApplicationAsset::storageSize() const {
    return storageSize_;
}

bool ApplicationAsset::isValid() const {
    return name_.length() > 0 && hash_.isValid();
}

bool ApplicationAsset::isReadable() {
    return !prepareForReading(false /* keepOpen */);
}

int ApplicationAsset::available() {
    int r = prepareForReading();
    if (!r) {
        r = asset_manager_available(data_->stream, nullptr);
    }
    if (r == SYSTEM_ERROR_END_OF_STREAM && !eof_) {
        data_.reset();
        eof_ = true;
    }
    if (r >= 0) {
        return r;
    }
    return 0;
}

int ApplicationAsset::read() {
    char c;
    int r = read(&c, sizeof(c));
    if (r == 1) {
        return c;
    }
    return r;
}

int ApplicationAsset::read(char* buffer, size_t size) {
    CHECK(prepareForReading());
    CHECK_TRUE(buffer && size, SYSTEM_ERROR_INVALID_ARGUMENT);
    size_t pos = 0;
    // Attempt to fill the whole provided buffer
    while (size > 0) {
        int actuallyRead = asset_manager_read(data_->stream, buffer + pos, size, nullptr);
        if (actuallyRead <= 0) {
            if (actuallyRead == SYSTEM_ERROR_END_OF_STREAM) {
                data_.reset();
                eof_ = true;
            }
            return pos > 0 ? pos : actuallyRead;
        }
        pos += actuallyRead;
        size -= actuallyRead;
    }
    return pos;
}

int ApplicationAsset::peek() {
    char c;
    int r = peek(&c, sizeof(c));
    if (r == 1) {
        return c;
    }
    return r;
}

int ApplicationAsset::peek(char* buffer, size_t size) {
    CHECK(prepareForReading());
    CHECK_TRUE(buffer && size, SYSTEM_ERROR_INVALID_ARGUMENT);
    return asset_manager_peek(data_->stream, buffer, size, nullptr);
}

int ApplicationAsset::skip(size_t size) {
    CHECK(prepareForReading());
    return asset_manager_skip(data_->stream, size, nullptr);
}

void ApplicationAsset::flush() {
    return;
}

void ApplicationAsset::reset() {
    eof_ = false;
    data_.reset();
}

size_t ApplicationAsset::write(uint8_t c) {
    return 0;
}

bool ApplicationAsset::operator==(const ApplicationAsset& other) const {
    return (name_ == other.name() && hash_ == other.hash());
}

bool ApplicationAsset::operator!=(const ApplicationAsset& other) const {
    return !(*this == other);
}

int ApplicationAsset::prepareForReading(bool keepOpen) {
    if (!isValid()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (data_ && data_->stream) {
        return 0;
    } else if (eof_) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }

    if (size() == 0) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }

    asset_manager_stream* stream = nullptr;
    asset_manager_asset a = {};
    a.name = name_.c_str();
    a.hash = hash_.hash().data();
    a.hash_length = hash_.hash().size();
    a.hash_type = hash_.type();
    CHECK(asset_manager_open(&stream, &a, nullptr));
    if (keepOpen) {
        data_ = std::make_shared<Data>(stream);
    }
    if (!data_) {
        asset_manager_close(stream, nullptr);
    }
    if (keepOpen && !data_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return 0;
}

ApplicationAsset::Data::Data(asset_manager_stream* s)
        : stream(s) {
}

ApplicationAsset::Data::~Data() {
    if (stream) {
        asset_manager_close(stream, nullptr);
        stream = nullptr;
    }
}

#endif // HAL_PLATFORM_ASSETS
