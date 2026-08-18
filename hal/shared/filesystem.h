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

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "filesystem_impl.h"

#include <lfs_util.h>
#include <lfs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum filesystem_instance_t {
    FILESYSTEM_INSTANCE_DEFAULT = 0,
    FILESYSTEM_INSTANCE_ASSET_STORAGE = 1
} filesystem_instance_t;

/* FIXME */
typedef struct filesystem_t {
    uint16_t version;
    uint32_t size;

    struct lfs_config config;
    lfs_t instance;

    bool state;

#ifdef LFS_NO_MALLOC
    uint8_t read_buffer[FILESYSTEM_READ_SIZE] __attribute__((aligned(4)));
    uint8_t prog_buffer[FILESYSTEM_PROG_SIZE] __attribute__((aligned(4)));
    uint8_t lookahead_buffer[FILESYSTEM_LOOKAHEAD / 8] __attribute__((aligned(4)));
    uint8_t file_buffer[FILESYSTEM_PROG_SIZE] __attribute__((aligned(4)));
#endif /* LFS_NO_MALLOC */

    filesystem_instance_t index;
    uintptr_t first_block;
} filesystem_t;

int filesystem_mount(filesystem_t* fs);
int filesystem_unmount(filesystem_t* fs);
int filesystem_invalidate(filesystem_t* fs);
filesystem_t* filesystem_get_instance(filesystem_instance_t index, void* reserved);
int filesystem_dump_info(filesystem_t* fs);

void filesystem_lock(filesystem_t* fs);
void filesystem_unlock(filesystem_t* fs);

// Returns how many times the calling thread has acquired the filesystem lock via `filesystem_lock()`
// without a corresponding `filesystem_unlock()`. The lock must be acquired before calling this
// function
int filesystem_lock_depth(filesystem_t* fs);

int filesystem_to_system_error(int error);

#ifdef __cplusplus
} // extern "C"

#include <memory>

#define CHECK_FS(expr) \
        ({ \
            auto _r = expr; \
            if (_r < 0) { \
                return filesystem_to_system_error(_r); \
            } \
            _r; \
        })

namespace particle::fs {

inline filesystem_t* defaultFs() {
    return filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr /* reserved */);
}

struct FsLock {
    explicit FsLock(filesystem_t* fs = defaultFs())
            : fs_(fs) {
        lock();
    }

    ~FsLock() {
        unlock();
    }

    void lock() {
        filesystem_lock(fs_);
    }

    void unlock() {
        filesystem_unlock(fs_);
    }

    // TODO: Rename this method to avoid confusion with filesystem_get_instance()
    lfs_t* instance() const {
        return &fs_->instance;
    }

private:
    filesystem_t* fs_;
};

// TODO: Implement Input/OutputStream?
class File {
public:
    File();
    File(const File& file) = delete;
    File(File&& file);
    ~File();

    int open(const char* path, int flags, filesystem_t* fs = defaultFs());
    int close();

    bool isOpen() const {
        return d_.get();
    }

    int read(void* buf, lfs_size_t size);
    int write(const void* buf, lfs_size_t size);
    int tell();
    int seek(lfs_soff_t offs, int whence = LFS_SEEK_SET);
    int size();
    int truncate(lfs_off_t size);
    int sync();

    lfs_file_t* handle();
    filesystem_t* fs() const;

    File& operator=(const File& file) = delete;
    File& operator=(File&& file);

private:
    struct Data;

    std::unique_ptr<Data> d_;
};

int mount(filesystem_t* fs = defaultFs());
int unmount(filesystem_t* fs = defaultFs());

int remove(const char* path, filesystem_t* fs = defaultFs());
int rename(const char* oldPath, const char* newPath, filesystem_t* fs = defaultFs());
int stat(const char* path, lfs_info* info, filesystem_t* fs = defaultFs());

} // particle::fs

#endif /* __cplusplus */
