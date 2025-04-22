/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "service_debug.h"
#include "filesystem.h"
#include "scope_guard.h"
#include "system_error.h"

#include <algorithm>
#include <mutex>
#include "check.h"

namespace particle {
namespace dct {

using namespace particle::fs;

class DctFile {
public:
    DctFile() {
        init();
    }

    ~DctFile() {
        deinit();
    }

    ssize_t read(size_t offset, uint8_t* buffer, size_t size) {
        FsLock lk(fs_);
        CHECK_FS(open(LFS_O_RDONLY));
        SCOPE_GUARD({
            close();
        });
        CHECK_FS(seek(offset));
        return lfs_file_read(lfs(), &file_, buffer, size);
    }

    ssize_t write(size_t offset, const uint8_t* buffer, size_t size) {
        FsLock lk(fs_);

        CHECK_FS(open(LFS_O_WRONLY));
        SCOPE_GUARD({
            close();
        });

        ssize_t r = CHECK_FS(seek(offset));

        r = lfs_file_write(lfs(), &file_, buffer, size);
        if (r < 0) {
            /* Error */
            LOG_DEBUG(ERROR, "Failed to write to DCT: %d", r);
        }

        return r;
    }

    bool clear() {
        FsLock lk(fs_);
        if (open(LFS_O_WRONLY) < 0) {
            return false;
        }
        SCOPE_GUARD({
            close();
        });
        char buf[128];
        memset(buf, 0xff, sizeof(buf));
        const lfs_ssize_t size = lfs_file_size(lfs(), &file_);
        if (size < 0) {
            return false;
        }
        size_t offs = 0;
        while (offs < (size_t)size) {
            const size_t n = std::min(sizeof(buf), (size_t)size - offs);
            const lfs_ssize_t r = lfs_file_write(lfs(), &file_, buf, n);
            if (r != (lfs_ssize_t)n) {
                return false;
            }
            offs += n;
        }
        return true;
    }


private:

    int open(int flags) {
        int r = lfs_file_open(lfs(), &file_, path_, flags);
        open_ = (r == 0);
        return r;
    }

    int close() {
        if (!open_) {
            return false;
        }
        open_ = false;
        return lfs_file_close(lfs(), &file_);
    }

    ssize_t seek(size_t offset) {
        return lfs_file_seek(lfs(), &file_, offset, LFS_SEEK_SET);
    }

    void init() {
        fs_ = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
        SPARK_ASSERT(fs_);

        FsLock lk(fs_);
        SPARK_ASSERT(!filesystem_mount(fs_));

        LOG_DEBUG(INFO, "Filesystem mounted");

        int r = lfs_mkdir(lfs(), "/sys");
        SPARK_ASSERT((r == 0 || r == LFS_ERR_EXIST));

        /* Check that /sys/dct.bin exists */
        struct lfs_info info;
        r = lfs_stat(lfs(), path_, &info);

        int flags = LFS_O_RDWR;

        if (r || info.size == 0) {
            /* Create as well if doesn't exist or was just created and not written */
            flags |= LFS_O_CREAT;
        }

        SPARK_ASSERT(open(flags) == 0);

        if (flags & LFS_O_CREAT) {
            LOG_DEBUG(INFO, "Initializing empty DCT");
            /* Fill with 0xff for compatibility with raw flash DCT */
            uint8_t tmp = 0xff;
            for (unsigned offset = 0; offset < sizeof(application_dct_t);) {
                r = lfs_file_write(lfs(), &file_, &tmp, sizeof(tmp));
                SPARK_ASSERT(r > 0);
                offset += r;
            }
        }

        SPARK_ASSERT(close() == 0);
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

} } // particle::dct