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

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef FILESYSTEM_BLOCK_SIZE
#define FILESYSTEM_BLOCK_SIZE 4096
#endif

typedef uint32_t lfs_size_t;
typedef int32_t lfs_ssize_t;
typedef uint32_t lfs_off_t;
typedef int32_t lfs_soff_t;

enum lfs_error {
    LFS_ERR_OK = 0,
    LFS_ERR_IO = -5,
    LFS_ERR_CORRUPT = -52,
    LFS_ERR_NOENT = -2,
    LFS_ERR_EXIST = -17,
    LFS_ERR_NOTDIR = -20,
    LFS_ERR_ISDIR = -21,
    LFS_ERR_NOTEMPTY = -39,
    LFS_ERR_BADF = -9,
    LFS_ERR_FBIG = -27,
    LFS_ERR_INVAL = -22,
    LFS_ERR_NOSPC = -28,
    LFS_ERR_NOMEM = -12
};

enum lfs_open_flags {
    LFS_O_RDONLY = 1,
    LFS_O_WRONLY = 2,
    LFS_O_RDWR = 3,
    LFS_O_CREAT = 0x0100,
    LFS_O_EXCL = 0x0200,
    LFS_O_TRUNC = 0x0400,
    LFS_O_APPEND = 0x0800
};

enum lfs_whence_flags {
    LFS_SEEK_SET = 0,
    LFS_SEEK_CUR = 1,
    LFS_SEEK_END = 2
};

typedef struct lfs {
} lfs_t;

typedef struct lfs_file {
    size_t pos;
    int flags;
    int fd;
} lfs_file_t;

typedef struct {
    lfs_t instance;
} filesystem_t;

#ifdef __cplusplus
extern "C" {
#endif

int lfs_file_open(lfs_t* lfs, lfs_file_t* file, const char* path, int flags);
int lfs_file_close(lfs_t* lfs, lfs_file_t* file);
lfs_ssize_t lfs_file_read(lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size);
lfs_ssize_t lfs_file_write(lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size);
lfs_soff_t lfs_file_size(lfs_t* lfs, lfs_file_t* file);
lfs_soff_t lfs_file_seek(lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence);
lfs_soff_t lfs_file_tell(lfs_t* lfs, lfs_file_t* file);
int lfs_file_truncate(lfs_t* lfs, lfs_file_t* file, lfs_off_t size);
int lfs_file_sync(lfs_t* lfs, lfs_file_t* file);
int lfs_remove(lfs_t* lfs, const char* path);
// TODO: Add stubs for remaining API functions

filesystem_t* filesystem_get_instance(void* reserved);
int filesystem_lock(filesystem_t* fs);
int filesystem_unlock(filesystem_t* fs);

#ifdef __cplusplus
} // extern "C"

namespace particle::fs {

class FsLock {
public:
    FsLock(filesystem_t* fs)
            : fs_(fs) {
        lock();
    }

    ~FsLock() {
        unlock();
    }

    void lock() {
        filesystem_unlock(fs_);
    }

    void unlock() {
        filesystem_unlock(fs_);
    }

private:
    filesystem_t* fs_;
};

} // namespace particle::fs

#endif // defined(__cplusplus)
