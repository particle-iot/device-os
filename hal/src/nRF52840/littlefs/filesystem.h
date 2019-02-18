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
#include "platform_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <lfs_util.h>
#include <lfs.h>

/* FIXME */
#define FILESYSTEM_PROG_SIZE    (256)
#define FILESYSTEM_READ_SIZE    (256)

#define FILESYSTEM_BLOCK_SIZE   (sFLASH_PAGESIZE)
/* XXX: Using half of the external flash for now */
#define FILESYSTEM_BLOCK_COUNT  (sFLASH_PAGECOUNT / 2)
#define FILESYSTEM_LOOKAHEAD    (128)

/* FIXME */
typedef struct {
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
} filesystem_t;

int filesystem_mount(filesystem_t* fs);
int filesystem_unmount(filesystem_t* fs);
filesystem_t* filesystem_get_instance(void* reserved);
int filesystem_dump_info(filesystem_t* fs);

int filesystem_lock(filesystem_t* fs);
int filesystem_unlock(filesystem_t* fs);

#ifdef __cplusplus
}

namespace particle { namespace fs {

struct FsLock {
    FsLock(filesystem_t* fs)
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

private:
    filesystem_t* fs_;
};

} } /* particle::fs */

#endif /* __cplusplus */
