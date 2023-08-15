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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_NONE

#include "hal_platform.h"

#if HAL_PLATFORM_COMPRESSED_OTA

#include "inflate_impl.h"

#include "system_error.h"

#include <memory>
#include "timer_hal.h"
#include "scope_guard.h"

#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "filesystem.h"
#include "file_util.h"
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
#include "check.h"
#include "delay_hal.h"
#include "random.h"

namespace {

const char INFLATE_TMP_FILE_BASE_PATH[] = "/tmp/inflate";
const size_t INFLATE_TMP_FILE_RAND_PART_SIZE = 10;

#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM

size_t regionsOverlap(size_t offset1, size_t size1, size_t offset2, size_t size2) {
    size_t overlapping = 0;
    if (offset1 >= offset2 && offset1 < (offset2 + size2)) {
        overlapping += std::min(offset1 + size1, offset2 + size2) - offset1;
    }
    if (offset1 < offset2 && (offset1 + size1) >= offset2) {
        overlapping += std::min(offset1 + size1, offset2 + size2) - offset2;
    }
    return overlapping;
}

#if 0
void syncReadWriteCacheIfNeeded(inflate_ctx* ctx) {
    if (regionsOverlap(ctx->read_cache_pos, ctx->read_cache_size, ctx->write_cache_pos, ctx->write_cache_size)) {
        for (unsigned i = 0; i < ctx->read_cache_size; i++) {
            size_t idx = ctx->read_cache_pos + i;
            if (idx >= ctx->write_cache_pos && idx < ctx->write_cache_pos + ctx->write_cache_size) {
                ctx->read_cache[i] = ctx->write_cache[idx - ctx->write_cache_pos];
            }
        }
    }
}
#endif // 0

#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM

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
    if (ctx->temp_file_name) {
        lfs_remove(&lfs->instance, ctx->temp_file_name);
        free(ctx->temp_file_name);
    }
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    delete ctx;
    delete[] buf;
}

int inflate_reset_impl(inflate_ctx* ctx) {
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    if (ctx->buf) {
        // Not using filesystem
        return 0;
    }
    auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    const fs::FsLock lock(fs);
    Random rand;
    if (!ctx->temp_file_name) {
        size_t len = sizeof(INFLATE_TMP_FILE_BASE_PATH) + INFLATE_TMP_FILE_RAND_PART_SIZE;
        ctx->temp_file_name = (char*)malloc(len);
        CHECK_TRUE(ctx->temp_file_name, SYSTEM_ERROR_NO_MEMORY);
        memset(ctx->temp_file_name, 0, len);
        strncpy(ctx->temp_file_name, INFLATE_TMP_FILE_BASE_PATH, len);
        rand.genBase32(ctx->temp_file_name + sizeof(INFLATE_TMP_FILE_BASE_PATH) - 1, INFLATE_TMP_FILE_RAND_PART_SIZE);
    }
    CHECK(openFile(&ctx->temp_file, ctx->temp_file_name, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC));
    // Pre-allocate
    if (lfs_file_seek(&fs->instance, &ctx->temp_file, (1 << INFLATE_MAX_WINDOW_BITS), LFS_SEEK_SET) < 0) {
        return SYSTEM_ERROR_FILE;
    }
    if (lfs_file_seek(&fs->instance, &ctx->temp_file, 0, LFS_SEEK_SET) < 0) {
        return SYSTEM_ERROR_FILE;
    }
    ctx->decomp.read_write_ctx = ctx;

    ctx->decomp.read_buf = [](void* buf, size_t offset, size_t size, void* context) -> int {
        auto ctx = (inflate_ctx*)context;
        auto f = &ctx->temp_file;
        auto fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);

        size_t inWriteCacheAbsOffset = 0;
        size_t inWriteCacheToRead = 0;

        size_t inReadCacheAbsOffset = 0;
        size_t inReadCacheToRead = 0;

        size_t readWriteOverlap = 0;

        ctx->read_request = nullptr;
        ctx->read_request_size = 0;

        // Three-ish cases:
        // 1. Some data may be in write cache -> read out of write cache
        // 2. Some data may already be in read cache -> read out of read cache
        // 3. Some data needs to be read from filesystem
        // 3.1. If requested size below read cache size -> extend read size
        //      to read cache size, fill read cache
        // 3.2. If requested size above read cache size -> read into target
        //      buffer directly (if possible), fill read cache up to read cache size

        // 1. Some data may be in write cache -> read out of write cache
        if (std::max(offset, ctx->write_cache_pos) < std::min(offset + size, ctx->write_cache_pos + ctx->write_cache_size)) {
            if (offset >= ctx->write_cache_pos) {
                // Scenario 1
                inWriteCacheAbsOffset = offset;
                inWriteCacheToRead = std::min(size, ctx->write_cache_pos + ctx->write_cache_size - offset);
            } else if (offset < ctx->write_cache_pos) {
                // Scenario 2
                size_t outSideWriteCache = std::min(size, ctx->write_cache_pos - offset);
                inWriteCacheToRead = size - outSideWriteCache;
                inWriteCacheAbsOffset = offset + outSideWriteCache;
            }
        }
        // 2. Some data may already be in read cache -> read out of read cache
        if (std::max(offset, ctx->read_cache_pos) < std::min(offset + size, ctx->read_cache_pos + ctx->read_cache_size)) {
            if (offset >= ctx->read_cache_pos) {
                // Scenario 1
                inReadCacheAbsOffset = offset;
                inReadCacheToRead = std::min(size, ctx->read_cache_pos + ctx->read_cache_size - offset);
            } else if (offset < ctx->read_cache_pos) {
                // Scenario 2
                size_t outSideReadCache = std::min(size, ctx->read_cache_pos - offset);
                inReadCacheToRead = size - outSideReadCache;
                inReadCacheAbsOffset = offset + outSideReadCache;
            }

            readWriteOverlap = regionsOverlap(inReadCacheAbsOffset, inReadCacheToRead, inWriteCacheAbsOffset, inWriteCacheToRead);
        }

        // 3. Some data needs to be read from filesystem
        size_t toRead = size - (inReadCacheToRead + inWriteCacheToRead - readWriteOverlap);

        if (buf) {
            if (toRead > 0) {
                const fs::FsLock lock(fs);
                // Read into target buffer
                if (lfs_file_seek(&fs->instance, f, offset, LFS_SEEK_SET) < 0) {
                    return 0;
                }
                if (lfs_file_read(&fs->instance, f, buf, size) < 0) {
                    return 0;
                }
                if (lfs_file_seek(&fs->instance, f, offset, LFS_SEEK_SET) < 0) {
                    return 0;
                }
                if (lfs_file_read(&fs->instance, f, ctx->read_cache, ctx->read_cache_size) < 0) {
                    return 0;
                }
                ctx->read_cache_pos = offset;
            } else if (inReadCacheToRead > 0) {
                memcpy((uint8_t*)buf + (inReadCacheAbsOffset - offset), ctx->read_cache + (inReadCacheAbsOffset - ctx->read_cache_pos), inReadCacheToRead);
            }

            if (inWriteCacheToRead > 0) {
                memcpy((uint8_t*)buf + (inWriteCacheAbsOffset - offset), ctx->write_cache + (inWriteCacheAbsOffset - ctx->write_cache_pos), inWriteCacheToRead);
            }
            ctx->read_request_size = size;
        } else {
            ctx->read_request = nullptr;
            ctx->read_request_size = 0;

            if (inWriteCacheToRead > 0 && inWriteCacheAbsOffset == offset) {
                ctx->read_request = ctx->write_cache + (inWriteCacheAbsOffset - ctx->write_cache_pos);
                ctx->read_request_size = inWriteCacheToRead;
                return ctx->read_request_size;
            } else if (inReadCacheToRead && inReadCacheAbsOffset == offset) {
                ctx->read_request = ctx->read_cache + (inReadCacheAbsOffset - ctx->read_cache_pos);
                ctx->read_request_size = inReadCacheToRead - readWriteOverlap;
                return ctx->read_request_size;
            }
            if (toRead > 0) {
                const fs::FsLock lock(fs);
                // Fill read cache
                if (lfs_file_seek(&fs->instance, f, offset, LFS_SEEK_SET) < 0) {
                    return 0;
                }
                if (lfs_file_read(&fs->instance, f, ctx->read_cache, ctx->read_cache_size) < 0) {
                    return 0;
                }

                ctx->read_cache_pos = offset;
                ctx->read_request = ctx->read_cache;
                ctx->read_request_size = std::min(size, ctx->read_cache_size);
                if (regionsOverlap(ctx->read_cache_pos, ctx->read_cache_size, ctx->write_cache_pos, ctx->write_cache_size)) {
                    ctx->read_request_size = ctx->write_cache_pos - ctx->read_cache_pos;
                }
            }
        }

        return ctx->read_request_size;
    };

    ctx->decomp.write_buf = [](const void* buf, size_t offset, size_t size, void* context) -> int {
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
        
        if (std::max(offset, ctx->write_cache_pos) < std::min(offset + size, ctx->write_cache_pos + ctx->write_cache_size)) {
            // Some of the data targets cache, some doesn't
            if (offset >= ctx->write_cache_pos) {
                // Scenario 1
                inCacheAbsOffset = offset;
                inCacheToWrite = std::min(size, ctx->write_cache_pos + ctx->write_cache_size - offset);
                toWrite = size - inCacheToWrite;
                toWriteAbsOffset = inCacheAbsOffset + inCacheToWrite;
            } else if (offset < ctx->write_cache_pos) {
                // Scenario 2
                toWriteAbsOffset = offset;
                toWrite = std::min(size, ctx->write_cache_pos - offset);
                inCacheToWrite = size - toWrite;
                inCacheAbsOffset = toWriteAbsOffset + toWrite;
            }
        } else {
            // Scenario 3
            toWriteAbsOffset = offset;
            toWrite = size;
        }

        if (inCacheToWrite > 0) {
            memcpy(ctx->write_cache + (inCacheAbsOffset - ctx->write_cache_pos), (const uint8_t*)buf + (inCacheAbsOffset - offset), inCacheToWrite);
        }
        size_t prevBlock = ctx->write_cache_pos;
        if (toWrite > 0) {
            // Flush the current block
            const fs::FsLock lock(fs);
            // NOTE: triggers a flush!

            size_t newBlock = (toWriteAbsOffset / ctx->write_cache_block_size) * ctx->write_cache_block_size;
            size_t flushSize = ctx->write_cache_block_size;
            if (newBlock != prevBlock + ctx->write_cache_block_size) {
                flushSize = ctx->write_cache_size;
            }
            if (lfs_file_seek(&fs->instance, f, ctx->write_cache_pos, LFS_SEEK_SET) < 0) {
                return 0;
            }
            if (lfs_file_write(&fs->instance, f, ctx->write_cache, flushSize) < 0) {
                return 0;
            }
            // Swap the block
            ctx->write_cache_pos = newBlock;
            // Move read cache over
            if (ctx->write_cache_pos > prevBlock && (ctx->write_cache_pos - prevBlock) == ctx->write_cache_block_size) {
                // Moved one block over to the right, update read cache
                ctx->read_cache_pos = ctx->write_cache_pos - ctx->read_cache_size;
                memcpy(ctx->read_cache, ctx->write_cache + ctx->write_cache_block_size - ctx->read_cache_size, ctx->read_cache_size);
            } else if (ctx->write_cache_pos > ctx->read_cache_size) {
                // Fill read cache
                if (!ctx->decomp.read_buf(nullptr, ctx->write_cache_pos - ctx->read_cache_size, ctx->read_cache_size, ctx)) {
                    return 0;
                }
            } else {
                // Invalidate
                ctx->read_cache_pos = ctx->write_cache_pos;
            }
            memcpy(ctx->write_cache, ctx->write_cache + ctx->write_cache_block_size, ctx->write_cache_size - flushSize);
            if (lfs_file_seek(&fs->instance, f, ctx->write_cache_pos + (ctx->write_cache_size - flushSize), LFS_SEEK_SET) < 0) {
                return 0;
            }
            if (lfs_file_read(&fs->instance, f, ctx->write_cache + (ctx->write_cache_size - flushSize), flushSize) < 0) {
                return 0;
            }
            // Write into cache now
            memcpy(ctx->write_cache + (toWriteAbsOffset - ctx->write_cache_pos), (const uint8_t*)buf + (toWriteAbsOffset - offset), toWrite);
        }
        return 1; // success
    };
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    return 0;
}

#endif // HAL_PLATFORM_COMPRESSED_OTA
