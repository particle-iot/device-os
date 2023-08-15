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

#include "asset_manager_api.h"
#include "asset_manager.h"
#include "check.h"
#include "scope_guard.h"

using namespace particle;

namespace {

struct Info {
    Vector<Asset> required;
    Vector<Asset> available;
};

void assetToApiAsset(const Asset& asset, asset_manager_asset* apiAsset) {
    apiAsset->name = asset.name().c_str();
    apiAsset->size = asset.size();
    apiAsset->hash_type = asset.hash().type();
    apiAsset->hash_length = asset.hash().hash().size();
    apiAsset->hash = asset.hash().hash().data();
    apiAsset->storage_size = asset.storageSize();
}

Asset assetFromApiAsset(const asset_manager_asset* apiAsset) {
    return Asset(apiAsset->name, AssetHash(apiAsset->hash, apiAsset->hash_length, (particle::AssetHash::Type)apiAsset->hash_type));
}

} // anonymous

int asset_manager_set_notify_hook(asset_manager_notify_hook hook, void* context, void* reserved) {
    return AssetManager::instance().setNotifyHook(hook, context);
}

int asset_manager_get_info(asset_manager_info* info, void* reserved) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto internal = new Info();
    CHECK_TRUE(internal, SYSTEM_ERROR_NO_MEMORY);
    internal->required = AssetManager::instance().requiredAssets();
    internal->available = AssetManager::instance().availableAssets();

    info->internal = (void*)internal;

    NAMED_SCOPE_GUARD(sg, {
        asset_manager_free_info(info, nullptr);
    });

    info->asset_size = sizeof(asset_manager_asset);
    info->required_count = internal->required.size();
    info->available_count = internal->available.size();

    if (info->required_count) {
        info->required = new asset_manager_asset[info->required_count];
        CHECK_TRUE(info->required, SYSTEM_ERROR_NO_MEMORY);
        memset(info->required, 0, sizeof(asset_manager_asset) * info->required_count);
        for (size_t i = 0; i < info->required_count; i++) {
            assetToApiAsset(internal->required[i], &info->required[i]);
        }
    }

    if (info->available_count) {
        info->available = new asset_manager_asset[info->available_count];
        CHECK_TRUE(info->available, SYSTEM_ERROR_NO_MEMORY);
        memset(info->available, 0, sizeof(asset_manager_asset) * info->available_count);
        for (size_t i = 0; i < info->available_count; i++) {
            assetToApiAsset(internal->available[i], &info->available[i]);
        }
    }
    sg.dismiss();
    return 0;
}

void asset_manager_free_info(asset_manager_info* info, void* reserved) {
    if (!info) {
        return;
    }
    if (info->internal) {
        auto internal = static_cast<Info*>(info->internal);
        delete internal;
    }
    if (info->required) {
        delete[] info->required;
    }
    if (info->available) {
        delete[] info->available;
    }
}

int asset_manager_set_consumer_state(asset_manager_consumer_state state, void* reserved) {
    return AssetManager::instance().setConsumerState(state);
}

int asset_manager_open(asset_manager_stream** stream, const asset_manager_asset* asset, void* reserved) {
    CHECK_TRUE(stream && asset, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto a = assetFromApiAsset(asset);
    CHECK_TRUE(a.isValid(), SYSTEM_ERROR_INVALID_ARGUMENT);

    auto reader = std::make_unique<AssetReader>();
    CHECK_TRUE(reader, SYSTEM_ERROR_NO_MEMORY);
    CHECK(reader->init(a.name()));
    CHECK(reader->validate(false));
    CHECK_TRUE(reader->isValid(), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(reader->asset() == a, SYSTEM_ERROR_NOT_FOUND);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    *stream = (asset_manager_stream*)reader.get();
    reader.release();
    return 0;
}

int asset_manager_available(asset_manager_stream* stream, void* reserved) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto reader = (AssetReader*)(stream);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    CHECK(assetStream->waitEvent(InputStream::READABLE));
    return assetStream->availForRead();
}

int asset_manager_read(asset_manager_stream* stream, char* data, size_t size, void* reserved) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto reader = (AssetReader*)(stream);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    return assetStream->read(data, size);
}

int asset_manager_peek(asset_manager_stream* stream, char* data, size_t size, void* reserved) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto reader = (AssetReader*)(stream);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    return assetStream->peek(data, size);
}

int asset_manager_skip(asset_manager_stream* stream, size_t size, void* reserved) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto reader = (AssetReader*)(stream);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    return assetStream->skip(size);
}

int asset_manager_seek(asset_manager_stream* stream, size_t offset, void* reserved) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto reader = (AssetReader*)(stream);
    InputStream* assetStream;
    CHECK(reader->assetStream(assetStream));
    return assetStream->seek(offset);
}

void asset_manager_close(asset_manager_stream* stream, void* reserved) {
    if (!stream) {
        return;
    }
    auto reader = (AssetReader*)(stream);
    delete reader;
}

int asset_manager_format_storage(void* reserved) {
    return AssetManager::instance().formatStorage(true /* remount */);
}