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

#include "hal_platform.h"

#if HAL_PLATFORM_COMPRESSED_OTA

#include "inflate_impl.h"

#include "system_error.h"

#include <memory>

#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "filesystem.h"
#include "file_util.h"
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "check.h"
#include "delay_hal.h"

namespace {

const char INFLATE_TMP_FILE_PATH[] = "/tmp/inflate.dat";

} // anonymous

using namespace particle;

int inflate_alloc_ctx(inflate_ctx** ctx, char** buf, size_t buf_size) {
    std::unique_ptr<char[]> bufPtr(new(std::nothrow) char[buf_size]);
    if (!bufPtr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    std::unique_ptr<inflate_ctx> ctxPtr(new(std::nothrow) inflate_ctx);
    if (!ctxPtr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    *ctx = ctxPtr.release();
    memset(*ctx, 0, sizeof(inflate_ctx));
    *buf = bufPtr.release();
    return 0;
}

void inflate_free_ctx(inflate_ctx* ctx, char* buf) {
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    const fs::FsLock lk(lfs);
    lfs_file_close(&lfs->instance, &ctx->temp_file);
    lfs_remove(&lfs->instance, INFLATE_TMP_FILE_PATH);
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    delete ctx;
    delete[] buf;
}

int inflate_reset_impl(inflate_ctx* ctx) {
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    const fs::FsLock lock(fs);
    lfs_remove(&fs->instance, INFLATE_TMP_FILE_PATH);
    CHECK(openFile(&ctx->temp_file, INFLATE_TMP_FILE_PATH, LFS_O_RDWR | LFS_O_CREAT));
    // Pre-allocate ctx->buf_size
    if (lfs_file_seek(&fs->instance, &ctx->temp_file, ctx->buf_size, LFS_SEEK_SET) < 0) {
        return SYSTEM_ERROR_FILE;
    }
    if (lfs_file_seek(&fs->instance, &ctx->temp_file, 0, LFS_SEEK_SET) < 0) {
        return SYSTEM_ERROR_FILE;
    }
    ctx->decomp.read_write_ctx = ctx;

    ctx->decomp.read_buf = [](void* buf, size_t offset, size_t size, void* context) -> int {
        // LOG(INFO, "read_buf offset=%u size=%u ctx=%x", offset, size, context);
        auto ctx = (inflate_ctx*)context;
        auto f = &ctx->temp_file;
        auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);

        size_t inCacheAbsOffset = 0;
        size_t inCacheToRead = 0;

        size_t toReadAbsOffset = 0;
        size_t toRead = 0;

        // Three scenarios:
        // 1. Starting offset is within the current cache block and may go over the cache block
        // 2. Starting offset is outside of the current cache block and may go into the cache block
        // 3. None of the requested data is in the cache block
        
        if (std::max(offset, ctx->cache_abs_pos) < std::min(offset + size, ctx->cache_abs_pos + ctx->cache_buf_size)) {
            // Some of the data is in cache, some potentially needs to be read from fs
            if (offset >= ctx->cache_abs_pos) {
                // Scenario 1
                inCacheAbsOffset = offset;
                inCacheToRead = std::min(size, ctx->cache_abs_pos + ctx->cache_buf_size - offset);
                toRead = size - inCacheToRead;
                toReadAbsOffset = inCacheAbsOffset + inCacheToRead;
            } else if (offset < ctx->cache_abs_pos) {
                // Scenario 2
                toReadAbsOffset = offset;
                toRead = std::min(size, ctx->cache_abs_pos - offset);
                inCacheToRead = size - toRead;
                inCacheAbsOffset = toReadAbsOffset + toRead;
            }
        } else {
            // Scenario 3
            toReadAbsOffset = offset;
            toRead = size;
        }

        // LOG(INFO, "inCacheAbsOffset=%u inCacheToRead=%u toReadAbsOffset=%u toRead=%u ctx->cache_abs_pos=%u", inCacheAbsOffset, inCacheToRead, toReadAbsOffset, toRead, ctx->cache_abs_pos);

        if (inCacheToRead > 0) {
            memcpy((uint8_t*)buf + (inCacheAbsOffset - offset), ctx->cache + (inCacheAbsOffset - ctx->cache_abs_pos), inCacheToRead);
        }
        if (toRead > 0) {
            const fs::FsLock lock(fs);
            // NOTE: Triggers a flush!
            LOG(INFO, "read_buf offset=%u size=%u ctx=%x", offset, size, context);
            LOG(INFO, "inCacheAbsOffset=%u inCacheToRead=%u toReadAbsOffset=%u toRead=%u ctx->cache_abs_pos=%u", inCacheAbsOffset, inCacheToRead, toReadAbsOffset, toRead, ctx->cache_abs_pos);
            LOG(INFO, "seek");
            if (lfs_file_seek(&fs->instance, f, toReadAbsOffset, LFS_SEEK_SET) < 0) {
                return 0;
            }
            if (lfs_file_read(&fs->instance, f, (uint8_t*)buf + (toReadAbsOffset - offset), toRead) < 0) {
                return 0;
            }
            LOG(INFO, "read_buf done");
        }
        // LOG(INFO, "read_buf done");
        return 1; // success
    };
    ctx->decomp.write_buf = [](const void* buf, size_t offset, size_t size, void* context) -> int {
        // LOG(INFO, "write_buf offset=%u size=%u ctx=%x", offset, size, context);
        auto ctx = (inflate_ctx*)context;
        auto f = &ctx->temp_file;
        auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);

        size_t inCacheAbsOffset = 0;
        size_t inCacheToWrite = 0;

        size_t toWriteAbsOffset = 0;
        size_t toWrite = 0;

        // Three scenarios:
        // 1. Starting offset is within the current cache block and may go over the cache block
        // 2. Starting offset is outside of the current cache block and may go into the cache block
        // 3. None of the requested data is in the cache block
        
        if (std::max(offset, ctx->cache_abs_pos) < std::min(offset + size, ctx->cache_abs_pos + ctx->cache_buf_size)) {
            // Some of the data targets cache, some doesn't
            if (offset >= ctx->cache_abs_pos) {
                // Scenario 1
                inCacheAbsOffset = offset;
                inCacheToWrite = std::min(size, ctx->cache_abs_pos + ctx->cache_buf_size - offset);
                toWrite = size - inCacheToWrite;
                toWriteAbsOffset = inCacheAbsOffset + inCacheToWrite;
            } else if (offset < ctx->cache_abs_pos) {
                // Scenario 2
                toWriteAbsOffset = offset;
                toWrite = std::min(size, ctx->cache_abs_pos - offset);
                inCacheToWrite = size - toWrite;
                inCacheAbsOffset = toWriteAbsOffset + toWrite;
            }
        } else {
            // Scenario 3
            toWriteAbsOffset = offset;
            toWrite = size;
        }

        // LOG(INFO, "inCacheAbsOffset=%u inCacheToWrite=%u toWriteAbsOffset=%u toWrite=%u ctx->cache_abs_pos=%u", inCacheAbsOffset, inCacheToWrite, toWriteAbsOffset, toWrite, ctx->cache_abs_pos);

        if (inCacheToWrite > 0) {
            memcpy(ctx->cache + (inCacheAbsOffset - ctx->cache_abs_pos), (const uint8_t*)buf + (inCacheAbsOffset - offset), inCacheToWrite);
        }
        if (toWrite > 0) {
            // Flush the current block
            const fs::FsLock lock(fs);
            // NOTE: triggers a flush!
            LOG(INFO, "write_buf offset=%u size=%u ctx=%x", offset, size, context);
            LOG(INFO, "inCacheAbsOffset=%u inCacheToWrite=%u toWriteAbsOffset=%u toWrite=%u ctx->cache_abs_pos=%u", inCacheAbsOffset, inCacheToWrite, toWriteAbsOffset, toWrite, ctx->cache_abs_pos);
            LOG(INFO, "seek");
            if (lfs_file_seek(&fs->instance, f, ctx->cache_abs_pos, LFS_SEEK_SET) < 0) {
                return 0;
            }
            if (lfs_file_write(&fs->instance, f, ctx->cache, ctx->cache_buf_size) < 0) {
                return 0;
            }
            // Swap the block
            ctx->cache_abs_pos = (toWriteAbsOffset / ctx->cache_buf_size) * ctx->cache_buf_size;
            LOG(INFO, "new ctx->cache_abs_pos=%u", ctx->cache_abs_pos);
            LOG(INFO, "seek");
            if (lfs_file_seek(&fs->instance, f, ctx->cache_abs_pos, LFS_SEEK_SET) < 0) {
                return 0;
            }
            if (lfs_file_read(&fs->instance, f, ctx->cache, ctx->cache_buf_size) < 0) {
                return 0;
            }
            // Write into cache now
            memcpy(ctx->cache + (toWriteAbsOffset - ctx->cache_abs_pos), (const uint8_t*)buf + (toWriteAbsOffset - offset), toWrite);
            LOG(INFO, "write_buf done");
        }
        // LOG(INFO, "write_buf done");
        return 1; // success
    };
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    return 0;
}

#endif // HAL_PLATFORM_COMPRESSED_OTA
