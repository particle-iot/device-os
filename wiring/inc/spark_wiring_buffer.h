/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include <limits>
#include <cstring>
#include <cstdint>

#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"

namespace particle {

/**
 * A dynamically allocated buffer.
 */
class Buffer {
public:
    /**
     * Construct an empty buffer.
     */
    Buffer() = default;

    /**
     * Construct a buffer of a specific size.
     *
     * The buffer is filled with zero bytes.
     *
     * @param size Buffer size.
     */
    explicit Buffer(size_t size) :
            d_(size) {
    }

    /**
     * Construct a buffer by copying existing data.
     *
     * @param data Data to copy.
     * @param size Data size.
     */
    Buffer(const char* data, size_t size) :
            d_(data, size) {
    }

    /**
     * Get a pointer to the buffer data.
     *
     * @return Buffer data.
     */
    char* data() {
        return d_.data();
    }

    /**
     * Get a pointer to the buffer data.
     *
     * @return Buffer data.
     */
    const char* data() const {
        return d_.data();
    }

    /**
     * Get the size of the buffer data.
     *
     * @return Data size.
     */
    size_t size() const {
        return d_.size();
    }

    /**
     * Check if the buffer is empty.
     *
     * @return `true` if the buffer is empty, otherwise `false`.
     */
    bool isEmpty() const {
        return d_.isEmpty();
    }

    /**
     * Resize the buffer.
     *
     * @param size New size.
     * @return `true` if the buffer was resized successfully, or `false` if a memory allocation error occured.
     */
    bool resize(size_t size) {
        return d_.resize(size);
    }

    /**
     * Get the capacity of the buffer.
     *
     * @return Buffer capacity.
     */
    size_t capacity() const {
        return d_.capacity();
    }

    /**
     * Ensure the buffer has enough capacity.
     *
     * @param size Desired capacity.
     * @return `true` if the capacity was changed successfully, or `false` if a memory allocation error occured.
     */
    bool reserve(size_t size) {
        return d_.reserve(size);
    }

    /**
     * Free the unused buffer capacity.
     *
     * @return `true` if the capacity was changed successfully, or `false` if a memory allocation error occured.
     */
    bool trimToSize() {
        return d_.trimToSize();
    }

    /**
     * Copy a portion of the buffer to a new buffer.
     *
     * @param pos Offset of the data to copy.
     * @param size Size of the data to copy.
     * @return New buffer.
     */
    Buffer slice(size_t pos, size_t size = std::numeric_limits<size_t>::max()) const;

    /**
     * Concatenate this buffer with another buffer.
     *
     * The contents of the buffer `buf` is appended to the contents of this buffer. The result is
     * returned in a new buffer.
     *
     * @param buf Buffer to concatenate with.
     * @return Concatenated buffer.
     */
    Buffer concat(const Buffer& buf) const;

    /**
     * Convert the contents of the buffer to a hex-encoded string.
     *
     * @return Hex-encoded string.
     */
    String toHex() const;

    /**
     * Convert the contents of the buffer to a hex-encoded string.
     *
     * The output is always null-terminated, unless the size of the output buffer is 0.
     *
     * @param out Output buffer.
     * @param size Size of the output buffer.
     * @return Number of characters written to the output buffer, not including the trailing `\0`.
     */
    size_t toHex(char* out, size_t size) const;

    /**
     * Compare this buffer with another buffer.
     *
     * @return `true` if the buffers contain the same bytes, otherwise `false`.
     */
    bool operator==(const Buffer& buf) const {
        return d_.size() == buf.d_.size() && std::memcmp(d_.data(), buf.d_.data(), d_.size()) == 0;
    }

    /**
     * Compare this buffer with another buffer.
     *
     * @return `true` if the buffers contain different bytes, otherwise `false`.
     */
    bool operator!=(const Buffer& buf) const {
        return !(operator==(buf));
    }

    /**
     * Construct a buffer from a hex-encoded string.
     *
     * @param str Hex-encoded string.
     * @return A buffer.
     */
    static Buffer fromHex(const String& str) {
        return fromHex(str.c_str(), str.length());
    }

    /**
     * Construct a buffer from a hex-encoded string.
     *
     * @param str Hex-encoded string.
     * @return A buffer.
     */
    static Buffer fromHex(const char* str) {
        return fromHex(str, std::strlen(str));
    }

    /**
     * Construct a buffer from a hex-encoded string.
     *
     * @param str Hex-encoded string.
     * @param len String length.
     * @return A buffer.
     */
    static Buffer fromHex(const char* str, size_t len);

private:
    Vector<char> d_;
};

} // namespace particle
