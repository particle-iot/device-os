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

#include "filesystem.h"
#include "platform_config.h"
#include "exflash_hal.h"
#include <mutex>

using namespace particle::fs;

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#include "static_recursive_mutex.h"

namespace {

static StaticRecursiveMutex s_lfs_mutex;

} /* anonymous */

int filesystem_lock(filesystem_t* fs) {
    (void)fs;
    return !s_lfs_mutex.lock();
}

int filesystem_unlock(filesystem_t* fs) {
    (void)fs;
    return !s_lfs_mutex.unlock();
}

#else

__attribute__((weak)) int filesystem_lock(filesystem_t* fs) {
    (void)fs;
    return 0;
}

__attribute__((weak)) int filesystem_unlock(filesystem_t* fs) {
    (void)fs;
    return 0;
}

#endif /* MODULE_FUNCTION != MOD_FUNC_BOOTLOADER */


namespace {

int fs_read(const struct lfs_config* c, lfs_block_t block,
            lfs_off_t off, void* buffer, lfs_size_t size)
{
    return hal_exflash_read(block * c->block_size + off, (uint8_t*)buffer, size);
}

int fs_prog(const struct lfs_config* c, lfs_block_t block,
            lfs_off_t off, const void* buffer, lfs_size_t size)
{
    return hal_exflash_write(block * c->block_size + off, (const uint8_t*)buffer, size);
}

int fs_erase(const struct lfs_config* c, lfs_block_t block)
{
    return hal_exflash_erase_sector(block * c->block_size, 1);
}

int fs_sync(const struct lfs_config *c)
{
    return 0;
}

filesystem_t s_instance = {};

} /* anonymous */

int filesystem_mount(filesystem_t* fs) {
    std::lock_guard<FsLock> lk(FsLock(fs));
    int ret = 0;

    if (fs->state) {
        /* Assume that already mounted */
        return ret;
    }

    fs->config.context = fs;

    fs->config.read = &fs_read;
    fs->config.prog = &fs_prog;
    fs->config.erase = &fs_erase;
    fs->config.sync = &fs_sync;
    fs->config.read_size = FILESYSTEM_READ_SIZE;
    fs->config.prog_size = FILESYSTEM_PROG_SIZE;
    fs->config.block_size = FILESYSTEM_BLOCK_SIZE;
    fs->config.block_count = FILESYSTEM_BLOCK_COUNT;
    fs->config.lookahead = FILESYSTEM_LOOKAHEAD;

#ifdef LFS_NO_MALLOC
    fs->config.read_buffer = fs->read_buffer;
    fs->config.prog_buffer = fs->prog_buffer;
    fs->config.lookahead_buffer = fs->lookahead_buffer;
    fs->config.file_buffer = fs->file_buffer;
#endif /* LFS_NO_MALLOC */

    ret = lfs_mount(&fs->instance, &fs->config);
    if (ret) {
        /* Error, attempt to format */
        ret = lfs_format(&fs->instance, &fs->config);
        if (!ret) {
            /* Re-attempt to mount */
            ret = lfs_mount(&fs->instance, &fs->config);
        }
    }

    if (!ret) {
        fs->state = true;
    }

    return ret;
}

int filesystem_unmount(filesystem_t* fs) {
    std::lock_guard<FsLock> lk(FsLock(fs));

    int ret = 0;

    if (fs->state) {
        ret = lfs_unmount(&fs->instance);
        fs->state = false;
    }

    return ret;
}

filesystem_t* filesystem_get_instance(void* reserved) {
    (void)reserved;

    return &s_instance;
}
