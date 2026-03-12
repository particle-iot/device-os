/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "system_cache.h"
#include "system_error.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace particle {
namespace test {

using SystemCacheKey = particle::services::SystemCacheKey;

class SystemCache {
public:
    static constexpr uint32_t FILE_MAGIC = 0x714f11e5;
    static constexpr uint32_t HEADER_MAGIC = 0x4ead;

    explicit SystemCache(const char* path = "/sys/cache.dat");
    ~SystemCache();

    int init();
    int deInit();

    int purge();
    int sync();

    int get(SystemCacheKey key, void* value, size_t length);
    int set(SystemCacheKey key, const void* value, size_t length);
    int del(SystemCacheKey key);
    int size(SystemCacheKey key);

    ssize_t get(uint16_t key, uint8_t* value, uint16_t length, int index = 0);
    int set(uint16_t key, const uint8_t* value, uint16_t length, int index = -1);
    int add(uint16_t key, const uint8_t* value, uint16_t length);
    int del(uint16_t key, int index = -1);
    int dataSize(uint16_t key, int index = -1);
    ssize_t size();

    SystemCache(const SystemCache&) = delete;
    SystemCache(SystemCache&&) = delete;
    SystemCache& operator=(const SystemCache&) = delete;
    SystemCache& operator=(SystemCache&&) = delete;

private:
    struct FileFooter {
        uint32_t reserved;
        uint32_t size;
        uint16_t reserved1;
        uint16_t version;
        uint32_t magic;
    } __attribute__((__packed__));
    static_assert(sizeof(FileFooter) == sizeof(uint32_t) * 4, "Unexpected FileFooter size");

    struct TlvHeader {
        uint16_t magic;
        uint16_t key;
        uint16_t length;
        uint16_t reserved;
    } __attribute__((__packed__));
    static_assert(sizeof(TlvHeader) == sizeof(uint32_t) * 2, "Unexpected TlvHeader size");

    int open();
    int validate();
    int mkdirs();
    int readFooter(FileFooter& footer);
    ssize_t find(uint16_t key, int index, uint16_t* dataSize);

    ssize_t seek(ssize_t offset, int whence = SEEK_SET);
    ssize_t readExact(void* buf, size_t length);
    ssize_t writeExact(const void* buf, size_t length);

private:
    char* path_ = nullptr;
    int fd_ = -1;
};

} // namespace test
} // namespace particle
