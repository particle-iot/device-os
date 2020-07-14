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

#include "check.h"

namespace particle::system {

SimpleFileStorage::SimpleFileStorage(const char* file) :
        file_(),
        fileName_(file),
        openFlags_(0) {
}

int SimpleFileStorage::load(char* data, size_t size) {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return SYSTEM_ERROR_FILE;
    }
    const fs::FsLock lock(fs);
    CHECK(open(fs, LFS_O_RDONLY));
    const auto fileSize = lfs_file_size(&fs->instance, &file_);
    if (fileSize < 0) {
        LOG(ERROR, "%s: Error while loading file: %d", fileName_, fileSize);
        return SYSTEM_ERROR_FILE;
    }
    if (size > (size_t)fileSize) {
        size = fileSize;
    }
    const int r = lfs_file_read(&fs->instance, &file_, data, size);
    if (r != 0) {
        LOG(ERROR, "%s: Error while loading file: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    return fileSize;
}

int SimpleFileStorage::save(const char* data, size_t size) {
    const auto fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return SYSTEM_ERROR_FILE;
    }
    const fs::FsLock lock(fs);
    CHECK(open(fs, LFS_O_WRONLY | LFS_O_CREAT));
    const auto fileSize = lfs_file_size(&fs->instance, &file_);
    if (fileSize < 0) {
        LOG(ERROR, "%s: Error while saving file: %d", fileName_, fileSize);
        return SYSTEM_ERROR_FILE;
    }
    int r = lfs_file_write(&fs->instance, &file_, data, size);
    if (r != 0) {
        LOG(ERROR, "%s: Error while saving file: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    if (size < (size_t)fileSize) {
        r = lfs_file_truncate(&fs->instance, &file_, size);
        if (r != 0) {
            LOG(ERROR, "%s: Error while saving file: %d", fileName_, r);
            return SYSTEM_ERROR_FILE;
        }
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
    if (r != 0) {
        LOG(ERROR, "%s: Error while saving file: %d", fileName_, r);
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
    if (r != LFS_ERR_OK && r != LFS_ERR_NOENT) {
        LOG(ERROR, "%s: Error while removing file: %d", fileName_, r);
    }
}

int SimpleFileStorage::open(filesystem_t* fs, int flags) {
    if ((openFlags_ & flags) == flags) {
        return 0;
    }
    flags |= openFlags_;
    close(fs);
    const int r = lfs_file_open(&fs->instance, &file_, fileName_, flags);
    if (r != LFS_ERR_OK) {
        if (r == LFS_ERR_NOENT) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        LOG(ERROR, "%s: Error while opening file: %d", fileName_, r);
        return SYSTEM_ERROR_FILE;
    }
    openFlags_ = flags;
    return 0;
}

void SimpleFileStorage::close(filesystem_t* fs) {
    if (!openFlags_) {
        return;
    }
    const int r = lfs_file_close(&fs->instance, &file_);
    if (r != LFS_ERR_OK) {
        LOG(ERROR, "%s: Error while closing file: %d", fileName_, r);
    }
    // All resources associated with the file are freed at this point, so it's safe to consider
    // the file closed even if there was an error
    openFlags_ = 0;
}

} // namespace particle::system

#endif // HAL_PLATFORM_FILESYSTEM
