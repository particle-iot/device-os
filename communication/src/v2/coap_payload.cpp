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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "coap_payload.h"

#include "file_util.h"
#include "check.h"

namespace particle::protocol::v2 {

using fs::FsLock;

namespace {

// Maximum expected length of a filesystem path
const size_t MAX_PATH_LEN = 127;

// Directory for storing temporary files
const auto TEMP_DIR = "/tmp/coap";

unsigned g_lastFileNum = 0;
bool g_tempDirCreated = false;

int formatTempFilePath(char* buf, size_t size, unsigned fileNum) {
    int n = std::snprintf(buf, size, "%s/p%u", TEMP_DIR, fileNum);
    if (n < 0 || (size_t)n >= size) {
        return SYSTEM_ERROR_PATH_TOO_LONG;
    }
    return 0;
}

int seekInFile(lfs_t* fs, lfs_file_t* file, size_t pos) {
    size_t curPos = CHECK_FS(lfs_file_tell(fs, file));
    // Prevent LittleFS from flushing the file if the offset hasn't changed
    if (pos != curPos) {
        CHECK_FS(lfs_file_seek(fs, file, pos, LFS_SEEK_SET));
    }
    return 0;
}

inline int closeFile(lfs_t* fs, lfs_file_t* file) {
    CHECK_FS(lfs_file_close(fs, file));
    return 0;
}

inline int removeFile(lfs_t* fs, const char* path) {
    CHECK_FS(lfs_remove(fs, path));
    return 0;
}

} // namespace

CoapPayload::CoapPayload(size_t maxHeapSize) :
        dataSize_(0),
        maxHeapSize_(std::min<size_t>(maxHeapSize, COAP_MAX_PAYLOAD_SIZE)),
        fileNum_(0) {
}

CoapPayload::~CoapPayload() {
    if (file_) {
        FsLock fs;
        removeTempFile(fs.instance());
    }
}

int CoapPayload::read(char* data, size_t size, size_t pos) {
    if (pos >= dataSize_) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    char* d = data;
    size_t bytesToRead = std::min(size, dataSize_ - pos);
    if (pos < maxHeapSize_) {
        size_t bytesInRam = std::min(dataSize_, maxHeapSize_) - pos;
        size_t n = std::min(bytesToRead, bytesInRam);
        std::memcpy(d, buf_.data() + pos, n);
        bytesToRead -= n;
        pos += n;
        d += n;
    }
    if (bytesToRead > 0) {
        FsLock fs;
        assert(file_);
        CHECK(seekInFile(fs.instance(), file_.get(), pos - maxHeapSize_));
        size_t n = CHECK_FS(lfs_file_read(fs.instance(), file_.get(), d, bytesToRead));
        if (n < bytesToRead) {
            // The payload size was set using setSize() or the file was modifed by somebody else
            std::memset(d + n, 0, bytesToRead - n);
        }
        d += bytesToRead;
    }
    return d - data;
}

int CoapPayload::write(const char* data, size_t size, size_t pos) {
    if (pos + size > COAP_MAX_PAYLOAD_SIZE) {
        return SYSTEM_ERROR_COAP_TOO_LARGE_PAYLOAD;
    }
    size_t p = pos;
    size_t bytesToWrite = size;
    if (p < maxHeapSize_) {
        size_t n = std::min(bytesToWrite, maxHeapSize_ - p);
        auto newSize = buf_.size() + n;
        if (newSize > buf_.capacity()) {
            // TODO: Use a pool of fixed-size chainable buffers
            size_t newCapacity = 0;
            if (buf_.capacity() > 0) {
                newCapacity = std::max<size_t>({ newSize, buf_.capacity() * 3 / 2, 128 });
            } else {
                // Do not allocate too much extra space on the first write
                newCapacity = std::max<size_t>(newSize, 8);
            }
            if (newCapacity > maxHeapSize_) {
                newCapacity = maxHeapSize_;
            }
            if (!buf_.reserve(newCapacity)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        buf_.resize(newSize);
        std::memcpy(buf_.data() + p, data, n);
        bytesToWrite -= n;
        p += n;
        data += n;
    }
    if (bytesToWrite > 0) {
        FsLock fs;
        if (!file_) {
            CHECK(createTempFile(fs.instance()));
        }
        assert(file_);
        CHECK(seekInFile(fs.instance(), file_.get(), p - maxHeapSize_));
        size_t n = CHECK_FS(lfs_file_write(fs.instance(), file_.get(), data, bytesToWrite));
        if (n != bytesToWrite) {
            return SYSTEM_ERROR_FILESYSTEM;
        }
    }
    if (pos + size > dataSize_) {
        dataSize_ = pos + size;
    }
    return size;
}

int CoapPayload::setSize(size_t size) {
    if (size > COAP_MAX_PAYLOAD_SIZE) {
        return SYSTEM_ERROR_COAP_TOO_LARGE_PAYLOAD;
    }
    size_t bytesInRam = std::min(size, maxHeapSize_);
    if (!buf_.resize(bytesInRam)) { // The newly allocated data is zero-initialized as necessary
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (size > maxHeapSize_ && !file_) {
        FsLock fs;
        CHECK(createTempFile(fs.instance()));
    }
    dataSize_ = size;
    return 0;
}

int CoapPayload::createTempFile(lfs_t* fs) {
    assert(!file_);
    std::unique_ptr<lfs_file_t> file(new(std::nothrow) lfs_file_t());
    if (!file) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (!g_tempDirCreated) {
        CHECK(mkdirp(TEMP_DIR));
        g_tempDirCreated = true;
    }
    auto fileNum = ++g_lastFileNum;
    char path[MAX_PATH_LEN + 1];
    CHECK(formatTempFilePath(path, sizeof(path), fileNum));
    CHECK_FS(lfs_file_open(fs, file.get(), path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_EXCL));
    file_ = std::move(file);
    fileNum_ = fileNum;
    return 0;
}

void CoapPayload::removeTempFile(lfs_t* fs) {
    if (!file_) {
        return;
    }
    int r = closeFile(fs, file_.get());
    if (r < 0) {
        LOG(ERROR, "Error while closing file: %d", r);
        // Try removing the file anyway
    }
    file_.reset();
    char path[MAX_PATH_LEN + 1];
    r = formatTempFilePath(path, sizeof(path), fileNum_);
    if (r >= 0) {
        r = removeFile(fs, path);
    }
    if (r < 0) {
        LOG(ERROR, "Failed to remove file: %d", r);
    }
}

} // namespace particle::protocol::v2
