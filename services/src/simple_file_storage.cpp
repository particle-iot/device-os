/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "simple_file_storage.h"

#if HAL_PLATFORM_FILESYSTEM

#include "endian_util.h"
#include "check.h"

namespace particle {

SimpleFileStorage::SimpleFileStorage(const char* fileName) :
        file_(),
        fileName_(fileName),
        dataSize_(-1),
        openFlags_(0) {
}

int SimpleFileStorage::load(void* data, size_t size) {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return SYSTEM_ERROR_FILE;
    }
    const fs::FsLock lock(fs);
    CHECK(open(fs, true /* readOnly */));
    const auto fileSize = lfs_file_size(&fs->instance, &file_);
    if (fileSize < 0) {
        LOG(ERROR, "%s: lfs_file_size() failed: %d", fileName_, fileSize);
        return SYSTEM_ERROR_FILE;
    }
    if (fileSize < 4) {
        return SYSTEM_ERROR_BAD_DATA; // Unexpected file size
    }
    if (dataSize_ < 0) {
        const size_t n = CHECK(readDataSize(fs));
        if ((n == 0 && fileSize > 4) || (n > 0 && (fileSize - 4) < (int)n)) {
            return SYSTEM_ERROR_BAD_DATA; // Unexpected file size
        }
        dataSize_ = n;
    }
    if ((int)size > dataSize_) {
        size = dataSize_;
    }
    int r = lfs_file_seek(&fs->instance, &file_, fileSize - dataSize_, LFS_SEEK_SET);
    if (r < 0) {
        LOG(ERROR, "%s: lfs_file_seek() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    r = lfs_file_read(&fs->instance, &file_, data, size);
    if (r != (int)size) {
        LOG(ERROR, "%s: lfs_file_read() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    return size;
}

int SimpleFileStorage::save(const void* data, size_t size) {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return SYSTEM_ERROR_FILE;
    }
    const fs::FsLock lock(fs);
    CHECK(open(fs, false /* readOnly */));
    const auto fileSize = lfs_file_size(&fs->instance, &file_);
    if (fileSize < 0) {
        LOG(ERROR, "%s: lfs_file_size() failed: %d", fileName_, fileSize);
        return SYSTEM_ERROR_FILE;
    }
    bool trunc = true;
    if (fileSize >= 4) {
        if (dataSize_ < 0) {
            dataSize_ = CHECK(readDataSize(fs));
        }
        // https://app.clubhouse.io/particle/story/60340/investigate-filesystem-performance-issues-encountered-with-ota-v3
        if (dataSize_ == (int)size && fileSize + (int)size <= FILESYSTEM_BLOCK_SIZE) {
            trunc = false;
        }
    }
    if (trunc) {
        int r = lfs_file_truncate(&fs->instance, &file_, 0);
        if (r < 0) {
            LOG(ERROR, "%s: lfs_file_truncate() failed: %d", fileName_, r);
            return SYSTEM_ERROR_FILE;
        }
        // lfs_file_truncate() doesn't change the current position in the file
        r = lfs_file_seek(&fs->instance, &file_, 0, LFS_SEEK_SET);
        if (r < 0) {
            LOG(ERROR, "%s: lfs_file_seek() failed: %d", fileName_, r);
            return SYSTEM_ERROR_FILE;
        }
        const auto n = nativeToLittleEndian<uint32_t>(size);
        r = lfs_file_write(&fs->instance, &file_, &n, 4);
        if (r != 4) {
            LOG(ERROR, "%s: lfs_file_write() failed: %d", fileName_, r);
            return SYSTEM_ERROR_FILE;
        }
        dataSize_ = size;
    }
    const int r = lfs_file_write(&fs->instance, &file_, data, size);
    if (r != (int)size) {
        LOG(ERROR, "%s: lfs_file_write() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    return size;
}

int SimpleFileStorage::sync() {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return SYSTEM_ERROR_FILE;
    }
    const fs::FsLock lock(fs);
    if (!(openFlags_ & LFS_O_WRONLY)) {
        return 0;
    }
    const int r = lfs_file_sync(&fs->instance, &file_);
    if (r < 0) {
        LOG(ERROR, "%s: lfs_file_sync() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    return 0;
}

void SimpleFileStorage::close() {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return;
    }
    const fs::FsLock lock(fs);
    close(fs);
}

void SimpleFileStorage::clear() {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return;
    }
    const fs::FsLock lock(fs);
    close(fs);
    const int r = lfs_remove(&fs->instance, fileName_);
    if (r < 0 && r != LFS_ERR_NOENT) {
        LOG(ERROR, "%s: lfs_remove() failed: %d", fileName_, r);
    }
}

int SimpleFileStorage::readDataSize(filesystem_t* fs) {
    int r = lfs_file_seek(&fs->instance, &file_, 0, LFS_SEEK_SET);
    if (r < 0) {
        LOG(ERROR, "%s: lfs_file_seek() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    uint32_t n = 0;
    r = lfs_file_read(&fs->instance, &file_, &n, 4);
    if (r != 4) {
        LOG(ERROR, "%s: lfs_file_read() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    n = littleEndianToNative(n);
    return n;
}

int SimpleFileStorage::open(filesystem_t* fs, bool readOnly) {
    const int flags = readOnly ? LFS_O_RDONLY : LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND;
    if ((openFlags_ & flags) == flags) {
        return 0;
    }
    close(fs);
    const int r = lfs_file_open(&fs->instance, &file_, fileName_, flags);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        LOG(ERROR, "%s: lfs_file_open() failed: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    openFlags_ = flags;
    return 0;
}

void SimpleFileStorage::close(filesystem_t* fs) {
    if (openFlags_ == 0) {
        return;
    }
    const int r = lfs_file_close(&fs->instance, &file_);
    if (r < 0) {
        LOG(ERROR, "%s: lfs_file_close() failed: %d", fileName_, r);
    }
    // All resources associated with the file are freed at this point, so it's safe to consider
    // the file closed even if there was an error
    openFlags_ = 0;
}

} // namespace particle

#endif // HAL_PLATFORM_FILESYSTEM
