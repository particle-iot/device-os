/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "dct_hal.h"
#include "dct.h"
#include "dcd_flash_impl.h"
#include "service_debug.h"

#include "filesystem.h"

#include <algorithm>
#include <mutex>

namespace {

using namespace particle::fs;

class DcdFile {
public:
    DcdFile() {
        init();
    }

    ~DcdFile() {
        deinit();
    }

    ssize_t read(size_t offset, uint8_t* buffer, size_t size) {
        FsLock lk(fs_);
        open(LFS_O_RDONLY);
        ssize_t r = seek(offset);
        if (r < 0) {
            close();
            return r;
        }
        r = lfs_file_read(lfs(), &file_, buffer, size);
        close();
        return r;
    }

    ssize_t write(size_t offset, const uint8_t* buffer, size_t size) {
        FsLock lk(fs_);

        open(LFS_O_WRONLY);

        ssize_t r = seek(offset);
        if (r < 0) {
            close();
            return r;
        }
        size_t bytes_written;
        for (bytes_written = 0; bytes_written < size;) {
            ssize_t r = lfs_file_write(lfs(), &file_, buffer + bytes_written, size - bytes_written/* std::min(size - bytes_written, (size_t)FILESYSTEM_PROG_SIZE) */);
            if (r < 0) {
                /* Error */
                LOG_DEBUG(ERROR, "Failed to write to DCD: %d", r);
                close();
                return r;
            }
            bytes_written += r;
        }

        if (r < 0) {
            close();
            return r;
        }

        close();

        return bytes_written;
    }


private:

    bool open(int flags) {
        open_ = lfs_file_open(lfs(), &file_, path_, flags) == 0;
        return open_;
    }

    bool close() {
        if (!open_) {
            return false;
        }
        open_ = false;
        return lfs_file_close(lfs(), &file_) == 0;
    }

    ssize_t seek(size_t offset) {
        return lfs_file_seek(lfs(), &file_, offset, LFS_SEEK_SET);
    }

    void init() {
        fs_ = filesystem_get_instance(nullptr);
        SPARK_ASSERT(fs_);
        SPARK_ASSERT(!filesystem_mount(fs_));

        LOG_DEBUG(INFO, "Filesystem mounted");

        FsLock lk(fs_);

        int r = lfs_mkdir(lfs(), "/sys");
        SPARK_ASSERT((r == 0 || r == LFS_ERR_EXIST));

        /* Check that /sys/dct.bin exists */
        struct lfs_info info;
        r = lfs_stat(lfs(), path_, &info);

        int flags = LFS_O_RDWR;

        if (r) {
            /* Create as well */
            flags |= LFS_O_CREAT;
        }

        SPARK_ASSERT(open(flags));

        if (flags & LFS_O_CREAT) {
            LOG_DEBUG(INFO, "Initializing empty DCT");
            /* Fill with 0xff for compatibility with raw flash DCD */
            uint8_t tmp = 0xff;
            for (unsigned offset = 0; offset < sizeof(application_dct_t);) {
                r = lfs_file_write(lfs(), &file_, &tmp, sizeof(tmp));
                SPARK_ASSERT(r > 0);
                offset += r;
            }
        }

        SPARK_ASSERT(close());
    }

    void deinit() {
        FsLock lk(fs_);

        lfs_file_close(lfs(), &file_);
        filesystem_unmount(fs_);
    }

    lfs_t* lfs() {
        return &fs_->instance;
    }

    filesystem_t* fs_;
    lfs_file_t file_;
    bool open_ = false;
    static constexpr const char* path_ = "/sys/dct.bin";
};

DcdFile& dcd() {
    static DcdFile dcd;
    return dcd;
}

} // namespace

const void* dct_read_app_data(uint32_t offset) {
    /* Deprecated */
    return nullptr;
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    int result = -1;
    dct_lock(0);
    result = dcd().read(offset, (uint8_t*)ptr, size);
    dct_unlock(0);
    return result > 0 ? 0 : result;
}

const void* dct_read_app_data_lock(uint32_t offset) {
    /* Deprecated */
    return nullptr;
}

int dct_read_app_data_unlock(uint32_t offset) {
    /* Deprecated */
    return -1;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    dct_lock(1);
    const int result = dcd().write(offset, (const uint8_t*)data, size);
    dct_unlock(1);
    return result > 0 ? 0 : result;
}
