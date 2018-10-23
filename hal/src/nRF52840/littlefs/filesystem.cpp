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
#include "rgbled.h"
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
    int r = hal_exflash_read(block * c->block_size + off, (uint8_t*)buffer, size);
    if (r) {
        LOG_DEBUG(ERROR, "fs_read error %d", r);
    }
    return r;
}

int fs_prog(const struct lfs_config* c, lfs_block_t block,
            lfs_off_t off, const void* buffer, lfs_size_t size)
{
    int r = hal_exflash_write(block * c->block_size + off, (const uint8_t*)buffer, size);
    if (r) {
        LOG_DEBUG(ERROR, "fs_prog error %d", r);
    }
    return r;
}

int fs_erase(const struct lfs_config* c, lfs_block_t block)
{
    int r = hal_exflash_erase_sector(block * c->block_size, 1);
    if (r) {
        LOG_DEBUG(ERROR, "fs_erase error %d", r);
    }
    return r;
}

int fs_sync(const struct lfs_config *c)
{
    return 0;
}

#ifdef DEBUG_BUILD

typedef size_t fsblkcnt_t;
typedef size_t fsfilcnt_t;

struct statvfs {
    unsigned long  f_bsize;    /* file system block size */
    unsigned long  f_frsize;   /* fragment size */
    fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
    fsblkcnt_t     f_bfree;    /* # free blocks */
    fsblkcnt_t     f_bavail;   /* # free blocks for unprivileged users */
    fsfilcnt_t     f_files;    /* # inodes */
    fsfilcnt_t     f_ffree;    /* # free inodes */
    fsfilcnt_t     f_favail;   /* # free inodes for unprivileged users */
    unsigned long  f_fsid;     /* file system ID */
    unsigned long  f_flag;     /* mount flags */
    unsigned long  f_namemax;  /* maximum filename length */
};

int statvfs(const char* path, struct statvfs* s)
{
    (void)path;

    if (!s) {
        return -1;
    }

    filesystem_t* fs = filesystem_get_instance(nullptr);
    if (!fs) {
        return -1;
    }

    FsLock lk(fs);

    size_t inUse = 0;

    int r = lfs_traverse(&fs->instance, [](void* p, lfs_block_t b) -> int {
        size_t* inUse = (size_t*)p;
        ++(*inUse);
        return 0;
    }, &inUse);

    if (r) {
        return r;
    }

    memset(s, 0, sizeof(*s));

    s->f_bsize = s->f_frsize = fs->config.block_size;
    s->f_blocks = fs->config.block_count;
    s->f_bfree = s->f_bavail = s->f_blocks - inUse;
    s->f_namemax = LFS_NAME_MAX;

    return 0;
}

void fs_dump_dir(filesystem_t* fs, char* path, size_t len)
{
    lfs_dir_t dir = {};
    int r = lfs_dir_open(&fs->instance, &dir, path);
    size_t pathLen = strnlen(path, len);

    if (r) {
        return;
    }

    LOG_PRINTF(TRACE, "%s:\r\n", path);

    struct lfs_info info = {};
    while (true) {
        r = lfs_dir_read(&fs->instance, &dir, &info);
        if (r != 1) {
            break;
        }
        LOG_PRINTF(TRACE, "%crw-rw-rw- %8lu %s\r\n", info.type == LFS_TYPE_REG ? '-' : 'd', info.size, info.name);
    }

    LOG_PRINTF(TRACE, "\r\n", path);

    r = lfs_dir_rewind(&fs->instance, &dir);

    while (true) {
        r = lfs_dir_read(&fs->instance, &dir, &info);
        if (r != 1) {
            break;
        }
        /* Restore path */
        path[pathLen] = '\0';
        if (info.type == LFS_TYPE_DIR && info.name[0] != '.') {
            int plen = snprintf(path + pathLen, len - pathLen, "%s%s", pathLen != 1 ? "/" : "", info.name);
            if (plen >= (int)(len - pathLen)) {
                /* Didn't fit */
                continue;
            }

            fs_dump_dir(fs, path, len);
        }
    }


    lfs_dir_close(&fs->instance, &dir);
}

void fs_dump(filesystem_t* fs)
{
    struct statvfs svfs;
    int r = statvfs(nullptr, &svfs);

    if (!r) {
        LOG_PRINTF(TRACE, "%-11s %11s %7s %4s %5s %8s %8s %8s  %4s\r\n",
            "Filesystem",
            "Block size",
            "Blocks",
            "Used",
            "Avail",
            "Size",
            "Used",
            "Avail",
            "Use%");
        LOG_PRINTF(TRACE, "%-11s %11lu %7lu %4lu %5lu %8lu %8lu %8lu %4lu%%\r\n\r\n",
            "littlefs",
            svfs.f_bsize,
            svfs.f_blocks,
            svfs.f_blocks - svfs.f_bfree,
            svfs.f_bfree,
            svfs.f_bsize * svfs.f_blocks,
            svfs.f_bsize * (svfs.f_blocks - svfs.f_bfree),
            svfs.f_bsize * svfs.f_bfree,
            (unsigned long)(100.0f - (((float)svfs.f_bfree / (float)svfs.f_blocks) * 100)));
    }

    /* Recursively traverse directories */
    char tmpbuf[(LFS_NAME_MAX + 1) * 2] = {};
    tmpbuf[0] = '/';
    fs_dump_dir(fs, tmpbuf, sizeof(tmpbuf));
}
#endif /* DEBUG_BUILD */

filesystem_t s_instance = {};

} /* anonymous */

int filesystem_mount(filesystem_t* fs) {
    FsLock lk(fs);
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
    if (!ret) {
        /* IMPORTANT: manually calling deorphan here to validate the filesystem.
         * We've added another check to avoid inifite loop when traversing
         * metadata-pair linked-list: when pair == tail (which means that the next entry is
         * the current pair, lfs_deorphan() will return LFS_ERR_CORRUPT).
         */
        ret = lfs_deorphan(&fs->instance);
    }
    if (ret) {
        /* Error, attempt to format:
         * (disabled) 1. Completely erase the flash
         * 2. lfs_format
         */

        /* This operation shouldn't fail, but just in case adding SPARK_ASSERT
         * to cause a reset if something goes wrong during the erasure
         */
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
        /* Give some indication that the bootloader is alive */
        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(LED_RGB);
#endif /* MODULE_FUNCTION == MOD_FUNC_BOOTLOADER */
        /* This operation takes about 5-10 seconds. It isn't strictly necessary
         * and was added simlpy as a precaution. We should still be able to recover
         * by just performing littlefs formatting instead of full flash erasure.
         */
        // SPARK_ASSERT(hal_exflash_erase_sector(0, FILESYSTEM_BLOCK_COUNT) == 0);
        ret = lfs_format(&fs->instance, &fs->config);
        if (!ret) {
            /* Re-attempt to mount */
            ret = lfs_mount(&fs->instance, &fs->config);
        }
    }

    if (!ret) {
        fs->state = true;
    }

    SPARK_ASSERT(fs->state);

    return ret;
}

int filesystem_unmount(filesystem_t* fs) {
    FsLock lk(fs);

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

int filesystem_dump_info(filesystem_t* fs) {
    if (!fs) {
        return -1;
    }

#ifdef DEBUG_BUILD
    fs_dump(fs);
#endif /* DEBUG_BUILD */

    return 0;
}