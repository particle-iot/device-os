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
#include "flash_mal.h"
#include "system_error.h"
#include "file_util.h"
#include "scope_guard.h"

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
    if (!((filesystem_t*)c->context)->state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int r = hal_exflash_read((block + ((filesystem_t*)c->context)->first_block) * c->block_size + off, (uint8_t*)buffer, size);
    if (r) {
        LOG_DEBUG(ERROR, "fs_read error %d", r);
    }
    return r;
}

int fs_prog(const struct lfs_config* c, lfs_block_t block,
            lfs_off_t off, const void* buffer, lfs_size_t size)
{
    if (!((filesystem_t*)c->context)->state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int r = hal_exflash_write((block + ((filesystem_t*)c->context)->first_block) * c->block_size + off, (const uint8_t*)buffer, size);
    if (r) {
        LOG_DEBUG(ERROR, "fs_prog error %d", r);
    }
    return r;
}

int fs_erase(const struct lfs_config* c, lfs_block_t block)
{
    if (!((filesystem_t*)c->context)->state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int r = hal_exflash_erase_sector((block + ((filesystem_t*)c->context)->first_block) * c->block_size, 1);
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

int statvfs(const char* path, struct statvfs* s, filesystem_t* fs = nullptr)
{
    (void)path;

    if (!s) {
        return -1;
    }

    if (!fs) {
        fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    }
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
    int r = statvfs(nullptr, &svfs, fs);

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
filesystem_t s_asset_storage_instance = {};

} /* anonymous */

int filesystem_mount(filesystem_t* fs) {
    FsLock lk(fs);
    int ret = 0;

    if (fs->state) {
        /* Assume that already mounted */
        return ret;
    }

    fs->state = true;
    SCOPE_GUARD({
        if (ret) {
            fs->state = false;
        }
        SPARK_ASSERT(fs->state);
    });

    ret = lfs_mount(&fs->instance, &fs->config);
    if (!ret) {
        /* IMPORTANT: manually calling deorphan here to validate the filesystem.
         * We've added another check to avoid inifite loop when traversing
         * metadata-pair linked-list: when pair == tail (which means that the next entry is
         * the current pair, lfs_deorphan() will return LFS_ERR_CORRUPT).
         */
        /* IMPORTANT: this should no longer be necessary, as we've mostly figured out what
         * caused the filesystem corruption. Depending on the size of the fs, this might take a while.
         * Disabled for now.
         */
        // ret = lfs_deorphan(&fs->instance);
    }
    if (ret) {
        SPARK_ASSERT(!(ret==LFS_ERR_IO));
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
        LED_On(PARTICLE_LED_RGB);
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
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        if (fs->index == FILESYSTEM_INSTANCE_DEFAULT) {
            // Make sure /usr and /tmp folders exist
            int r = lfs_mkdir(&fs->instance, "/usr");
            SPARK_ASSERT((r == 0 || r == LFS_ERR_EXIST));
            particle::rmrf("/tmp");
            r = lfs_mkdir(&fs->instance, "/tmp");
            SPARK_ASSERT((r == 0 || r == LFS_ERR_EXIST));
            // FIXME: recursively cleanup /tmp
        }
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    }

    return ret;
}

int filesystem_unmount(filesystem_t* fs) {
    FsLock lk(fs);

    int ret = 0;

    if (fs->state) {
        ret = lfs_unmount(&fs->instance);
        fs->state = false;
        // This should not be required as storage read/write/erase are gated
        // by fs->state, but just in case invalidate at least files.
        for (lfs_file_t* f = fs->instance.files; f; f = f->next) {
            f->pos = LFS_FILE_MAX;
            f->flags = LFS_F_ERRED;
            f->size = 0;
            f->pair[0] = 0;
            f->pair[1] = 0;
        }
    }

    return ret;
}

int filesystem_invalidate(filesystem_t* fs) {
    FsLock lk(fs);

    if (fs->state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // Just in case
    filesystem_get_instance(fs->index, nullptr);

    // Erase two superblocks
    return hal_exflash_erase_sector(fs->first_block * fs->config.block_size, 2);
}

void filesystem_config(filesystem_t* fs) {
    fs->config.context = fs;

    fs->config.read = &fs_read;
    fs->config.prog = &fs_prog;
    fs->config.erase = &fs_erase;
    fs->config.sync = &fs_sync;
    fs->config.read_size = FILESYSTEM_READ_SIZE;
    fs->config.prog_size = FILESYSTEM_PROG_SIZE;
    fs->config.block_size = FILESYSTEM_BLOCK_SIZE;
    // Already configured in filesystem_get_instance
    // fs->config.block_count = 0;
    fs->config.lookahead = FILESYSTEM_LOOKAHEAD;

#ifdef LFS_NO_MALLOC
    fs->config.read_buffer = fs->read_buffer;
    fs->config.prog_buffer = fs->prog_buffer;
    fs->config.lookahead_buffer = fs->lookahead_buffer;
    fs->config.file_buffer = fs->file_buffer;
#endif /* LFS_NO_MALLOC */
}

filesystem_t* filesystem_get_instance(filesystem_instance_t index, void* reserved) {
    (void)reserved;
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
#else
    static int onceFlag = 0;
    if (!onceFlag) ({
        onceFlag = 1;
#endif
        // FIXME: make this cleaner and move to using C++ classes etc
        s_instance.index = FILESYSTEM_INSTANCE_DEFAULT;
        s_asset_storage_instance.index = FILESYSTEM_INSTANCE_ASSET_STORAGE;

        s_instance.first_block = FILESYSTEM_FIRST_BLOCK;
        s_instance.config.block_count = FILESYSTEM_BLOCK_COUNT;

        s_asset_storage_instance.first_block = EXTERNAL_FLASH_ASSET_STORAGE_FIRST_PAGE;
        s_asset_storage_instance.config.block_count = EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT;

        filesystem_config(&s_instance);
        filesystem_config(&s_asset_storage_instance);
    });

    if (index == FILESYSTEM_INSTANCE_DEFAULT) {
        return &s_instance;
    } else if (index == FILESYSTEM_INSTANCE_ASSET_STORAGE) {
        return &s_asset_storage_instance;
    }
    return nullptr;
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

int filesystem_to_system_error(int error) {
    switch (error) {
    case LFS_ERR_OK: return SYSTEM_ERROR_NONE;
    case LFS_ERR_IO: return SYSTEM_ERROR_FILESYSTEM_IO;
    case LFS_ERR_CORRUPT: return SYSTEM_ERROR_FILESYSTEM_CORRUPT;
    case LFS_ERR_NOENT: return SYSTEM_ERROR_FILESYSTEM_NOENT;
    case LFS_ERR_EXIST: return SYSTEM_ERROR_FILESYSTEM_EXIST;
    case LFS_ERR_NOTDIR: return SYSTEM_ERROR_FILESYSTEM_NOTDIR;
    case LFS_ERR_ISDIR: return SYSTEM_ERROR_FILESYSTEM_ISDIR;
    case LFS_ERR_NOTEMPTY: return SYSTEM_ERROR_FILESYSTEM_NOTEMPTY;
    case LFS_ERR_BADF: return SYSTEM_ERROR_FILESYSTEM_BADF;
    case LFS_ERR_FBIG: return SYSTEM_ERROR_FILESYSTEM_FBIG;
    case LFS_ERR_INVAL: return SYSTEM_ERROR_FILESYSTEM_INVAL;
    case LFS_ERR_NOSPC: return SYSTEM_ERROR_FILESYSTEM_NOSPC;
    case LFS_ERR_NOMEM: return SYSTEM_ERROR_FILESYSTEM_NOMEM;
    default: return SYSTEM_ERROR_FILESYSTEM;
    }
}
