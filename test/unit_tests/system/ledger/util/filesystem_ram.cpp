/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

// Real LittleFS backed by a RAM block device

#include "filesystem.h"
#include "system_error.h"

#include <vector>
#include <cstring>

namespace {

std::vector<uint8_t> g_flash(FILESYSTEM_BLOCK_SIZE * FILESYSTEM_BLOCK_COUNT, 0xff);
filesystem_t g_fs = {};

int ramRead(const lfs_config* c, lfs_block_t block, lfs_off_t off, void* buf, lfs_size_t size) {
    std::memcpy(buf, g_flash.data() + block * c->block_size + off, size);
    return 0;
}

int ramProg(const lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buf, lfs_size_t size) {
    std::memcpy(g_flash.data() + block * c->block_size + off, buf, size);
    return 0;
}

int ramErase(const lfs_config* c, lfs_block_t block) {
    std::memset(g_flash.data() + block * c->block_size, 0xff, c->block_size);
    return 0;
}

int ramSync(const lfs_config* c) {
    return 0;
}

} // namespace

int filesystem_mount(filesystem_t* fs) {
    if (fs->state) {
        return 0;
    }
    auto& c = fs->config;
    c = {};
    c.context = fs;
    c.read = ramRead;
    c.prog = ramProg;
    c.erase = ramErase;
    c.sync = ramSync;
    c.read_size = FILESYSTEM_READ_SIZE;
    c.prog_size = FILESYSTEM_PROG_SIZE;
    c.block_size = FILESYSTEM_BLOCK_SIZE;
    c.block_count = FILESYSTEM_BLOCK_COUNT;
    c.lookahead = FILESYSTEM_LOOKAHEAD;
    if (lfs_mount(&fs->instance, &c) != 0) {
        int r = lfs_format(&fs->instance, &c);
        if (r != 0) {
            return filesystem_to_system_error(r);
        }
        r = lfs_mount(&fs->instance, &c);
        if (r != 0) {
            return filesystem_to_system_error(r);
        }
    }
    fs->state = true;
    return 0;
}

int filesystem_unmount(filesystem_t* fs) {
    if (!fs->state) {
        return 0;
    }
    lfs_unmount(&fs->instance);
    fs->state = false;
    return 0;
}

filesystem_t* filesystem_get_instance(filesystem_instance_t index, void* reserved) {
    // The code under test expects the default instance to be mounted
    if (!g_fs.state) {
        filesystem_mount(&g_fs);
    }
    return &g_fs;
}

int filesystem_lock(filesystem_t* fs) {
    return 0;
}

int filesystem_unlock(filesystem_t* fs) {
    return 0;
}

// Copied from hal/src/gcc/littlefs/lfs_utils.cpp, which depends on the platform headers
void lfs_crc(uint32_t* __restrict__ crc, const void* buffer, size_t size) {
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };
    const uint8_t *data = (const uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        *crc = (*crc >> 4) ^ rtable[(*crc ^ (data[i] >> 0)) & 0xf];
        *crc = (*crc >> 4) ^ rtable[(*crc ^ (data[i] >> 4)) & 0xf];
    }
}

namespace particle::fs {

// fs::File is referenced by services/src/file_util.cpp but not used by the ledger code
struct File::Data {
};

File::File() {
}

File::~File() {
}

int File::open(const char* path, int flags, filesystem_t* fs) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int File::close() {
    return 0;
}

int File::write(const void* buf, lfs_size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

} // namespace particle::fs

// Copied from hal/shared/filesystem.cpp, which can't be compiled for the tests
int filesystem_to_system_error(int error) {
    if (error >= 0) {
        return error;
    }
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
