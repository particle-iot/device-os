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
#include "coap_api.h"

#include "file_util.h"
#include "check.h"

namespace particle::protocol::experimental {

using fs::FsLock;

namespace {

const auto COAP_TEMP_DIR = "/tmp/coap";

const size_t MAX_PAYLOAD_SIZE = 16 * 1024;
const size_t MAX_PAYLOAD_SIZE_IN_RAM = COAP_BLOCK_SIZE;

static_assert(MAX_PAYLOAD_SIZE_IN_RAM <= MAX_PAYLOAD_SIZE);

const size_t INITIAL_BUFFER_CAPACITY = 128;

const size_t MAX_PATH_LEN = 127;

unsigned g_tempFileCount = 0;

int formatTempFilePath(char* buf, size_t size, unsigned fileNum) {
    int n = std::snprintf(buf, size, "%s/p%u", COAP_TEMP_DIR, fileNum);
    if (n < 0 || (size_t)n >= size) {
        return SYSTEM_ERROR_PATH_TOO_LONG;
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

int CoapPayload::read(char* data, size_t size) {
    assert(pos_ <= size_);
    if (pos_ == size_) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    auto d = data;
    size_t p = pos_;
    size_t bytesToRead = std::min(size, size_ - pos_);
    if (p < MAX_PAYLOAD_SIZE_IN_RAM) {
        size_t bytesInRam = std::min(size_, MAX_PAYLOAD_SIZE_IN_RAM) - p;
        size_t n = std::min(bytesToRead, bytesInRam);
        std::memcpy(d, buf_.data() + p, n);
        bytesToRead -= n;
        d += n;
        p += n;
    }
    if (bytesToRead > 0) {
        FsLock fs;
        size_t bytesInFile = size_ + MAX_PAYLOAD_SIZE_IN_RAM - p;
        if (bytesToRead > bytesInFile) {
            bytesToRead = bytesInFile;
        }
        assert(file_);
        size_t n = CHECK_FS(lfs_file_read(fs.instance(), file_.get(), d, size));
        if (n != bytesToRead) {
            LOG(ERROR, "Unexpected end of payload data");
            return SYSTEM_ERROR_BAD_DATA; // Somebody modified the file
        }
        d += n;
        p += n;
    }
    pos_ = p;
    return d - data;
}

int CoapPayload::write(const char* data, size_t size) {
    if (pos_ + size > MAX_PAYLOAD_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    size_t p = pos_;
    size_t bytesToWrite = size;
    if (p < MAX_PAYLOAD_SIZE_IN_RAM) {
        size_t n = std::min(bytesToWrite, MAX_PAYLOAD_SIZE_IN_RAM - p);
        auto newSize = buf_.size() + n;
        if (newSize > buf_.capacity()) {
            // TODO: Use a pool of fixed-size chainable buffers
            auto newCapacity = std::min(std::max(std::max(buf_.capacity() * 3 / 2, newSize), INITIAL_BUFFER_CAPACITY),
                    MAX_PAYLOAD_SIZE_IN_RAM);
            if (!buf_.reserve(newCapacity)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        buf_.resize(newSize);
        std::memcpy(buf_.data() + p, data, n);
        bytesToWrite -= n;
        data += n;
        p += n;
    }
    if (bytesToWrite > 0) {
        FsLock fs;
        if (!file_) {
            CHECK(createTempFile());
        }
        assert(file_);
        size_t n = CHECK_FS(lfs_file_write(fs.instance(), file_.get(), data, bytesToWrite));
        if (n != bytesToWrite) {
            return SYSTEM_ERROR_FILESYSTEM;
        }
        p += n;
    }
    size_ += size;
    pos_ = p;
    return size;
}

int CoapPayload::setSize(size_t size) {
    if (size > MAX_PAYLOAD_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    size_t p = std::min(pos_, size);
    size_t bytesInRam = std::min(size, MAX_PAYLOAD_SIZE_IN_RAM);
    if (!buf_.resize(bytesInRam)) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (size > MAX_PAYLOAD_SIZE_IN_RAM) {
        FsLock fs;
        if (!file_) {
            CHECK(createTempFile());
        }
        size_t fileOffs = 0;
        if (p > MAX_PAYLOAD_SIZE_IN_RAM) {
            fileOffs = p - MAX_PAYLOAD_SIZE_IN_RAM;
        }
        CHECK_FS(lfs_file_seek(fs.instance(), file_.get(), fileOffs, LFS_SEEK_SET));
    } else if (file_) {
        FsLock fs;
        CHECK_FS(lfs_file_seek(fs.instance(), file_.get(), 0 /* off */, LFS_SEEK_SET));
    }
    size_ = size;
    pos_ = p;
    return 0;
}

int CoapPayload::setPos(size_t pos) {
    if (pos > size_) {
        pos = size_;
    }
    if (pos > MAX_PAYLOAD_SIZE_IN_RAM) {
        FsLock fs;
        assert(file_);
        size_t fileOffs = pos - MAX_PAYLOAD_SIZE_IN_RAM;
        CHECK_FS(lfs_file_seek(fs.instance(), file_.get(), fileOffs, LFS_SEEK_SET));
    } else if (file_) {
        FsLock fs;
        CHECK_FS(lfs_file_seek(fs.instance(), file_.get(), 0 /* off */, LFS_SEEK_SET));
    }
    pos_ = pos;
    return 0;
}

int CoapPayload::initTempDir() {
    CHECK(rmrf(COAP_TEMP_DIR)); // FIXME: Don't remove the directory itself
    CHECK(mkdirp(COAP_TEMP_DIR));
    return 0;
}

int CoapPayload::createTempFile() {
    assert(!file_);
    std::unique_ptr<lfs_file_t> file(new(std::nothrow) lfs_file_t());
    if (!file) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    FsLock fs;
    auto fileNum = ++g_tempFileCount; // Incremented under the filesystem lock
    char path[MAX_PATH_LEN + 1];
    CHECK(formatTempFilePath(path, sizeof(path), fileNum));
    CHECK_FS(lfs_file_open(fs.instance(), file.get(), path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_EXCL));
    file_ = std::move(file);
    fileNum_ = fileNum;
    return 0;
}

void CoapPayload::removeTempFile() {
    if (!file_) {
        return;
    }
    FsLock fs;
    int r = closeFile(fs.instance(), file_.get());
    if (r < 0) {
        LOG(ERROR, "Error while closing file: %d", r);
        // Try removing the file anyway
    }
    file_.reset();
    char path[MAX_PATH_LEN + 1];
    r = formatTempFilePath(path, sizeof(path), fileNum_);
    if (r >= 0) {
        r = removeFile(fs.instance(), path);
    }
    if (r < 0) {
        LOG(ERROR, "Failed to remove file: %d", r);
    }
}

} // namespace particle::protocol::experimental
