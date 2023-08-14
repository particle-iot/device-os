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

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

#include "spark_wiring_string.h"
#include "buffer.h"
#include "str_util.h"

namespace particle {

// Shared hash class between application and system

/**
 * Asset hash.
 * 
 */
class AssetHash {
public:
    /**
     * Hash type. SHA256 is the default.
     * 
     */
    enum Type {
        INVALID = -1,
        SHA256 = 0,
        DEFAULT = SHA256,
    };

    /**
     * SHA256 size in bytes.
     * 
     */
    static const size_t SHA256_HASH_SIZE = 32;

    /**
     * Construct a new Asset Hash object (default).
     * 
     */
    AssetHash();
    /**
     * Construct a new Asset Hash object from buffer `hash` of
     * length `length`.
     * 
     * @param hash Hash.
     * @param length Hash length.
     * @param type (optional) Hash type, SHA256 is the default.
     */
    ///@{
    AssetHash(const char* hash, size_t length, Type type = Type::DEFAULT);
    AssetHash(const uint8_t* hash, size_t length, Type type = Type::DEFAULT);
    ///@}

    /**
     * Construct a new Asset Hash object from `Buffer` object.
     * 
     * @param hash Hash.
     * @param type Hash type.
     */
    AssetHash(const Buffer& hash, Type type = Type::DEFAULT);

    /**
     * Copy cosntructor.
     * 
     * @param other Asset Hash object to copy from.
     */
    AssetHash(const AssetHash& other) = default;

    /**
     * Move constructor.
     * 
     * @param other 
     */
    AssetHash(AssetHash&& other) = default;

    /**
     * Assignment/move-assignment operators.
     * 
     * @param other 
     * @return AssetHash& 
     */
    ///@{
    AssetHash& operator=(const AssetHash& other) = default;
    AssetHash& operator=(AssetHash&& other) = default;
    ///@}

    /**
     * Hash type.
     * 
     * @return `Type` enum value.
     */
    Type type() const;

    /**
     * Get raw hash contents.
     * 
     * @return const Buffer& Buffer object reference.
     */
    const Buffer& hash() const;

    /**
     * Check if Asset Hash object is valid.
     * 
     * @return true 
     * @return false 
     */
    bool isValid() const;

    /**
     * Convert Asset Hash to hexedemical string.
     * 
     * @return String Hex-encoded hash.
     */
    String toString() const;

    /**
     * Comparison operators. Asset Hashes match if their type, length and values match.
     * 
     * @param other Object to compare with.
     * @return true 
     * @return false 
     */
    ///@{
    bool operator==(const AssetHash& other) const;
    bool operator!=(const AssetHash& other) const;
    ///@}

private:
    Type type_;
    Buffer hash_;
};

inline AssetHash::AssetHash()
        : type_(Type::INVALID) {
}

inline AssetHash::AssetHash(const char* hash, size_t length, Type type)
        : AssetHash(Buffer(hash, length), type) {
}

inline AssetHash::AssetHash(const uint8_t* hash, size_t length, Type type)
        : AssetHash(Buffer(hash, length), type) {
}

inline AssetHash::AssetHash(const Buffer& hash, Type type)
        : AssetHash() {
    // Only SHA256 for now
    if (type == Type::SHA256 && hash.size() == SHA256_HASH_SIZE) {
        hash_ = hash;
        type_ = type;
    }
}

inline AssetHash::Type AssetHash::type() const {
    return type_;
}

inline const Buffer& AssetHash::hash() const {
    return hash_;
}

inline bool AssetHash::isValid() const {
    return type_ != Type::INVALID && hash_.size() > 0;
}

inline String AssetHash::toString() const {
    if (hash_.size() > 0) {
        Buffer buf(hash_.size() * 2 + 1);
        if (buf.size()) {
            toHex((const uint8_t*)hash_.data(), hash_.size(), buf.data(), buf.size());
            return String(buf.data());
        }
    }
    return String();
}

inline bool AssetHash::operator==(const AssetHash& other) const {
    return (type_ == other.type_ && hash_ == other.hash_);
}

inline bool AssetHash::operator!=(const AssetHash& other) const {
    return !(*this == other);
}

} // particle

extern "C" {
#endif // __cplusplus

typedef struct asset_manager_asset {
    const char* name;
    
    const char* hash;
    uint16_t hash_length;
    uint8_t hash_type;

    uint32_t size;
    uint32_t storage_size;
} asset_manager_asset_t;

typedef struct asset_manager_info {
    uint16_t size;
    uint16_t version;

    void* internal;

    uint16_t asset_size;

    asset_manager_asset* required;
    uint16_t required_count;

    asset_manager_asset* available;
    uint16_t available_count;
} asset_manager_info;

struct asset_manager_stream;
typedef struct asset_manager_stream asset_manager_stream;

typedef enum asset_manager_consumer_state {
    ASSET_MANAGER_CONSUMER_STATE_NONE = 0,
    ASSET_MANAGER_CONSUMER_STATE_HANDLED = 1,
    ASSET_MANAGER_CONSUMER_STATE_WANT = 2,
    ASSET_MANAGER_CONSUMER_STATE_MAX = 0xff
} asset_manager_consumer_state;

typedef void (*asset_manager_notify_hook)(void* context);

/**
 * Register a notification hook to be called whenever new assets are available
 * 
 * @param hook Hook function.
 * @param context Hook function context.
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code 
 */
int asset_manager_set_notify_hook(asset_manager_notify_hook hook, void* context, void* reserved);

/**
 * Query Asset Manager for info on available and required assets
 * 
 * @param info Info to be populated (most be initialized by the caller at least with size)
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code 
 */
int asset_manager_get_info(asset_manager_info* info, void* reserved);

/**
 * Frees previously filled in info structure
 * 
 * @param info Info structure to be freed after asset_manager_get_info() call
 * @param reserved Reserved (NULL)
 */
void asset_manager_free_info(asset_manager_info* info, void* reserved);

/**
 * Sets the asset consumer (application) state.
 * 
 * Whenever a new asset is received this gets automatically reset to ASSET_MANAGER_CONSUMER_STATE_WANT,
 * which triggers execution of registered notify hook on next boot.
 * 
 * Setting state to ASSET_MANAGER_CONSUMER_STATE_HANDLED, will prevent further execution
 * of the notify hook until a new asset is received.
 * 
 * @param state One of `asset_manager_consumer_state` e.g. ASSET_MANAGER_CONSUMER_STATE_HANDLED
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code
 */
int asset_manager_set_consumer_state(asset_manager_consumer_state state, void* reserved);

/**
 * Creates/allocates and opens a stream object to read out the request asset.
 * 
 * @param stream[out] Stream object
 * @param asset Asset identifier (name, hash)
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code
 */
int asset_manager_open(asset_manager_stream** stream, const asset_manager_asset* asset, void* reserved);

/**
 * Query number of readily available bytes in stream. The data is generated in chunks, so this will
 * never return the total size of the asset.
 * 
 * @param stream Stream object
 * @param reserved Reserved (NULL)
 * @return Number of readily available bytes or `system_error_t` error code.
 */
int asset_manager_available(asset_manager_stream* stream, void* reserved);

/**
 * Read from asset stream.
 * 
 * @param stream Stream object
 * @param data Target buffer
 * @param size Target buffer size
 * @param reserved Reserved (NULL)
 * @return Number of bytes written into `data` on success or `system_error_t` error code
 */
int asset_manager_read(asset_manager_stream* stream, char* data, size_t size, void* reserved);

/**
 * Peek from asset stream.
 * 
 * @param stream Stream object
 * @param data Target buffer
 * @param size Target buffer size
 * @param reserved Reserved (NULL)
 * @return Number of bytes written into `data` on success or `system_error_t` error code
 */
int asset_manager_peek(asset_manager_stream* stream, char* data, size_t size, void* reserved);

/**
 * Skip `size` bytes within asset stream.
 * 
 * @param stream Stream object
 * @param size Number of bytes to skip.
 * @param reserved Reserved (NULL)
 * @return Number of bytes skipped or `system_error_t` error code
 */
int asset_manager_skip(asset_manager_stream* stream, size_t size, void* reserved);

/**
 * Seek within asset stream.
 * 
 * NOTE: This method can only be used to rewind (`offset` = 0), or essentially
 * skip using an absolute offset as opposed to `asset_manager_skip()` taking a relative
 * offset.
 * 
 * @param stream Stream object
 * @param offset Offset within the stream to seek to (0 or an offset past the current stream read point)
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code
 */
int asset_manager_seek(asset_manager_stream* stream, size_t offset, void* reserved);

/**
 * Close asset stream and cleanup.
 * 
 * @param stream Stream object (will be freed)
 * @param reserved Reserved (NULL)
 */
void asset_manager_close(asset_manager_stream* stream, void* reserved);

#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
/**
 * Format asset storage.
 * 
 * @param reserved Reserved (NULL)
 * @return 0 on success or `system_error_t` error code
 */
int asset_manager_format_storage(void* reserved);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

#ifdef __cplusplus
}
#endif // __cplusplus
