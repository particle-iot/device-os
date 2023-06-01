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

bool ApplicationAsset::isValid() const {
    return name_.length() > 0 && hash_.isValid();
}

bool ApplicationAsset::isReadable() {
    if (!isValid()) {
        return false;
    }
    if (size() == 0) {
        return false;
    }
    if (prepareForReading()) {
        return false;
    }
    return true;
}

int ApplicationAsset::available() {
    if (!isReadable()) {
        return 0;
    }
    int r = asset_manager_available(data_->stream, nullptr);
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
    if (!isReadable()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK_TRUE(buffer && size, SYSTEM_ERROR_INVALID_ARGUMENT);
    return asset_manager_read(data_->stream, buffer, size, nullptr);
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
    if (!isReadable()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK_TRUE(buffer && size, SYSTEM_ERROR_INVALID_ARGUMENT);
    return asset_manager_peek(data_->stream, buffer, size, nullptr);
}

int ApplicationAsset::skip(size_t size) {
    if (!isReadable()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return asset_manager_skip(data_->stream, size, nullptr);
}

void ApplicationAsset::flush() {
    return;
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

int ApplicationAsset::prepareForReading() {
    if (data_ && data_->stream) {
        return 0;
    }

    if (!isValid()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    asset_manager_stream* stream = nullptr;
    asset_manager_asset a = {};
    a.name = name_.c_str();
    a.hash = hash_.hash().data();
    a.hash_length = hash_.hash().size();
    a.hash_type = hash_.type();
    CHECK(asset_manager_open(&stream, &a, nullptr));
    data_ = std::make_shared<Data>(stream);
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
