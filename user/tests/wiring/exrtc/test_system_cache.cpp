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

#include "test_system_cache.h"

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace particle {
namespace test {

namespace {

int systemErrorFromErrno(int err) {
    switch (err) {
    case ENOENT:
        return SYSTEM_ERROR_NOT_FOUND;
    case EINVAL:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    case ENOSPC:
        return SYSTEM_ERROR_NO_MEMORY;
    default:
        return SYSTEM_ERROR_IO;
    }
}

} // anonymous

SystemCache::SystemCache(const char* path) {
    if (path) {
        path_ = strdup(path);
    }
}

SystemCache::~SystemCache() {
    deInit();
    if (path_) {
        free(path_);
        path_ = nullptr;
    }
}

int SystemCache::init() {
    if (!path_) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int r = mkdirs();
    if (r) {
        return r;
    }
    return open();
}

int SystemCache::deInit() {
    if (fd_ >= 0) {
        if (::close(fd_) < 0) {
            int err = errno;
            fd_ = -1;
            return systemErrorFromErrno(err);
        }
        fd_ = -1;
    }
    return SYSTEM_ERROR_NONE;
}

int SystemCache::purge() {
    int r = deInit();
    if (r) {
        return r;
    }
    if (::unlink(path_) < 0 && errno != ENOENT) {
        return systemErrorFromErrno(errno);
    }
    return SYSTEM_ERROR_NONE;
}

int SystemCache::sync() {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (::fsync(fd_) < 0) {
        return systemErrorFromErrno(errno);
    }
    return SYSTEM_ERROR_NONE;
}

int SystemCache::get(SystemCacheKey key, void* value, size_t length) {
    if (length > std::numeric_limits<uint16_t>::max()) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    return (int)get((uint16_t)key, (uint8_t*)value, (uint16_t)length);
}

int SystemCache::set(SystemCacheKey key, const void* value, size_t length) {
    if (length > std::numeric_limits<uint16_t>::max()) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    return set((uint16_t)key, (const uint8_t*)value, (uint16_t)length);
}

int SystemCache::del(SystemCacheKey key) {
    return del((uint16_t)key);
}

int SystemCache::size(SystemCacheKey key) {
    return dataSize((uint16_t)key);
}

ssize_t SystemCache::size() {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    struct stat st = {};
    if (::fstat(fd_, &st) < 0) {
        return systemErrorFromErrno(errno);
    }
    return st.st_size;
}

ssize_t SystemCache::get(uint16_t key, uint8_t* value, uint16_t length, int index) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!value && length != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint16_t dataSize = 0;
    ssize_t pos = find(key, index, &dataSize);
    if (pos < 0) {
        return pos;
    }

    const size_t toRead = std::min<size_t>(length, dataSize);
    if (!toRead) {
        return 0;
    }
    ssize_t r = seek(pos + (ssize_t)sizeof(TlvHeader));
    if (r < 0) {
        return r;
    }
    return readExact(value, toRead);
}

int SystemCache::set(uint16_t key, const uint8_t* value, uint16_t length, int index) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    uint16_t existingSize = 0;
    ssize_t existingPos = find(key, index, &existingSize);
    if (existingPos == SYSTEM_ERROR_NOT_FOUND) {
        return add(key, value, length);
    }
    if (existingPos < 0) {
        return (int)existingPos;
    }

    FileFooter footer = {};
    int r = readFooter(footer);
    if (r) {
        return r;
    }

    std::unique_ptr<uint8_t[]> inBuf;
    if (footer.size > 0) {
        inBuf.reset(new(std::nothrow) uint8_t[footer.size]);
        if (!inBuf) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        r = (int)seek(0);
        if (r >= 0) {
            r = (int)readExact(inBuf.get(), footer.size);
        }
        if (r < 0) {
            return r;
        }
    }

    const size_t newEntrySize = sizeof(TlvHeader) + length;
    std::unique_ptr<uint8_t[]> outBuf(new(std::nothrow) uint8_t[footer.size + newEntrySize]);
    if (!outBuf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }

    size_t pos = 0;
    size_t outPos = 0;
    int currentIndex = 0;
    bool replaced = false;

    while (pos < footer.size) {
        if (footer.size - pos < sizeof(TlvHeader)) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        auto* header = reinterpret_cast<TlvHeader*>(inBuf.get() + pos);
        if (header->magic != HEADER_MAGIC) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        const size_t entrySize = sizeof(TlvHeader) + header->length;
        if (entrySize > footer.size - pos) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        const bool keyMatch = header->key == key;
        const bool replaceEntry = keyMatch && (index < 0 || currentIndex == index);
        if (!replaceEntry) {
            memcpy(outBuf.get() + outPos, inBuf.get() + pos, entrySize);
            outPos += entrySize;
        } else {
            replaced = true;
        }

        if (keyMatch) {
            ++currentIndex;
        }
        pos += entrySize;
    }

    TlvHeader newHeader = {};
    newHeader.magic = HEADER_MAGIC;
    newHeader.key = key;
    newHeader.length = length;
    memcpy(outBuf.get() + outPos, &newHeader, sizeof(newHeader));
    outPos += sizeof(newHeader);
    if (length > 0) {
        memcpy(outBuf.get() + outPos, value, length);
        outPos += length;
    }

    footer.magic = FILE_MAGIC;
    footer.size = outPos;

    r = (int)seek(0);
    if (r >= 0 && outPos > 0) {
        r = (int)writeExact(outBuf.get(), outPos);
    }
    if (r >= 0) {
        r = (int)writeExact(&footer, sizeof(footer));
    }
    if (r < 0) {
        return r;
    }
    if (::ftruncate(fd_, footer.size + (off_t)sizeof(footer)) < 0) {
        return systemErrorFromErrno(errno);
    }
    return sync();
}

int SystemCache::add(uint16_t key, const uint8_t* value, uint16_t length) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    FileFooter footer = {};
    int r = readFooter(footer);
    if (r) {
        return r;
    }

    r = (int)seek(footer.size);
    if (r < 0) {
        return r;
    }

    TlvHeader header = {};
    header.magic = HEADER_MAGIC;
    header.key = key;
    header.length = length;

    r = (int)writeExact(&header, sizeof(header));
    if (r < 0) {
        return r;
    }
    r = (int)writeExact(value, length);
    if (r < 0) {
        return r;
    }

    footer.magic = FILE_MAGIC;
    footer.size += sizeof(header) + length;
    r = (int)writeExact(&footer, sizeof(footer));
    if (r < 0) {
        return r;
    }

    return sync();
}

int SystemCache::del(uint16_t key, int index) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    FileFooter footer = {};
    int r = readFooter(footer);
    if (r) {
        return r;
    }

    std::unique_ptr<uint8_t[]> buf;
    if (footer.size > 0) {
        buf.reset(new(std::nothrow) uint8_t[footer.size]);
        if (!buf) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        r = (int)seek(0);
        if (r >= 0) {
            r = (int)readExact(buf.get(), footer.size);
        }
        if (r < 0) {
            return r;
        }
    }

    size_t pos = 0;
    size_t outPos = 0;
    int currentIndex = 0;
    bool deleted = false;

    while (pos < footer.size) {
        if (footer.size - pos < sizeof(TlvHeader)) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        auto* header = reinterpret_cast<TlvHeader*>(buf.get() + pos);
        if (header->magic != HEADER_MAGIC) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        const size_t entrySize = sizeof(TlvHeader) + header->length;
        if (entrySize > footer.size - pos) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        const bool keyMatch = header->key == key;
        const bool deleteEntry = keyMatch && (index < 0 || currentIndex == index);

        if (!deleteEntry) {
            if (outPos != pos) {
                memmove(buf.get() + outPos, buf.get() + pos, entrySize);
            }
            outPos += entrySize;
        } else {
            deleted = true;
        }

        if (keyMatch) {
            ++currentIndex;
        }
        pos += entrySize;
    }

    if (!deleted) {
        return SYSTEM_ERROR_NOT_FOUND;
    }

    footer.size = outPos;
    r = (int)seek(0);
    if (r >= 0 && outPos > 0) {
        r = (int)writeExact(buf.get(), outPos);
    }
    if (r >= 0) {
        r = (int)writeExact(&footer, sizeof(footer));
    }
    if (r < 0) {
        return r;
    }
    if (::ftruncate(fd_, footer.size + (off_t)sizeof(footer)) < 0) {
        return systemErrorFromErrno(errno);
    }
    return sync();
}

int SystemCache::dataSize(uint16_t key, int index) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    uint16_t dataSize = 0;
    ssize_t pos = find(key, index, &dataSize);
    if (pos < 0) {
        return (int)pos;
    }
    return dataSize;
}

int SystemCache::open() {
    if (fd_ >= 0) {
        return SYSTEM_ERROR_NONE;
    }

    fd_ = ::open(path_, O_CREAT | O_RDWR, 0644);
    if (fd_ < 0) {
        return systemErrorFromErrno(errno);
    }

    int r = validate();
    if (!r) {
        return SYSTEM_ERROR_NONE;
    }

    if (::ftruncate(fd_, 0) < 0) {
        r = systemErrorFromErrno(errno);
        deInit();
        return r;
    }
    if (seek(0) < 0) {
        r = SYSTEM_ERROR_IO;
        deInit();
        return r;
    }

    FileFooter footer = {};
    footer.magic = FILE_MAGIC;
    footer.size = 0;
    r = (int)writeExact(&footer, sizeof(footer));
    if (r < 0) {
        deInit();
        return r;
    }
    return sync();
}

int SystemCache::validate() {
    auto fileSize = size();
    if (fileSize < 0) {
        return (int)fileSize;
    }
    if ((size_t)fileSize < sizeof(FileFooter)) {
        return SYSTEM_ERROR_BAD_DATA;
    }

    FileFooter footer = {};
    int r = readFooter(footer);
    if (r) {
        return r;
    }
    if (footer.magic != FILE_MAGIC) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    if ((size_t)footer.size + sizeof(FileFooter) != (size_t)fileSize) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return SYSTEM_ERROR_NONE;
}

int SystemCache::mkdirs() {
    if (!path_) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto* path = strdup(path_);
    if (!path) {
        return SYSTEM_ERROR_NO_MEMORY;
    }

    for (char* p = path + 1; *p; ++p) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (::mkdir(path, 0755) < 0 && errno != EEXIST) {
            int r = systemErrorFromErrno(errno);
            free(path);
            return r;
        }
        *p = '/';
    }
    free(path);
    return SYSTEM_ERROR_NONE;
}

int SystemCache::readFooter(FileFooter& footer) {
    auto fileSize = size();
    if (fileSize < 0) {
        return (int)fileSize;
    }
    ssize_t r = seek(fileSize - (ssize_t)sizeof(footer));
    if (r < 0) {
        return (int)r;
    }
    r = readExact(&footer, sizeof(footer));
    return r < 0 ? (int)r : SYSTEM_ERROR_NONE;
}

ssize_t SystemCache::find(uint16_t key, int index, uint16_t* dataSize) {
    if (fd_ < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FileFooter footer = {};
    int r = readFooter(footer);
    if (r) {
        return r;
    }

    ssize_t pos = 0;
    int currentIndex = 0;
    while (pos < footer.size) {
        TlvHeader header = {};
        r = (int)seek(pos);
        if (r < 0) {
            return r;
        }
        r = (int)readExact(&header, sizeof(header));
        if (r < 0) {
            return r;
        }
        if (header.magic != HEADER_MAGIC) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        if (header.key == key && (index < 0 || currentIndex == index)) {
            if (dataSize) {
                *dataSize = header.length;
            }
            return pos;
        }
        if (header.key == key) {
            ++currentIndex;
        }
        pos += sizeof(TlvHeader) + header.length;
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

ssize_t SystemCache::seek(ssize_t offset, int whence) {
    const off_t r = ::lseek(fd_, offset, whence);
    if (r < 0) {
        return systemErrorFromErrno(errno);
    }
    return r;
}

ssize_t SystemCache::readExact(void* buf, size_t length) {
    uint8_t* p = (uint8_t*)buf;
    size_t remaining = length;
    while (remaining) {
        const ssize_t r = ::read(fd_, p, remaining);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return systemErrorFromErrno(errno);
        }
        if (r == 0) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        p += r;
        remaining -= r;
    }
    return length;
}

ssize_t SystemCache::writeExact(const void* buf, size_t length) {
    const uint8_t* p = (const uint8_t*)buf;
    size_t remaining = length;
    while (remaining) {
        const ssize_t r = ::write(fd_, p, remaining);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return systemErrorFromErrno(errno);
        }
        p += r;
        remaining -= r;
    }
    return length;
}

} // namespace test
} // namespace particle
