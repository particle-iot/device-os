/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#include "stream.h"
#include "check.h"
#include "scope_guard.h"
#include "storage_hal.h"
#if HAL_PLATFORM_FILESYSTEM
#include "filesystem.h"
#include "lfs.h"
#endif // HAL_PLATFORM_FILESYSTEM
#include <memory>
#if HAL_PLATFORM_COMPRESSED_OTA
#include "inflate.h"
#endif // HAL_PLATFORM_COMPRESSED_OTA

namespace particle {

class StorageHalInputStream : public InputStream {
public:
    StorageHalInputStream(hal_storage_id storageId, uintptr_t address, size_t length, size_t offset = 0)
            : storageId_(storageId),
              address_(address),
              remaining_(length),
              offset_(offset) {
    }

    StorageHalInputStream(module_bounds_location_t location, uintptr_t address, size_t length, size_t offset = 0)
            : storageId_(HAL_STORAGE_ID_INVALID),
              address_(address),
              remaining_(length),
              offset_(offset) {
        if (location == MODULE_BOUNDS_LOC_INTERNAL_FLASH) {
            storageId_ = HAL_STORAGE_ID_INTERNAL_FLASH;
        } else if (location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH) {
            storageId_ = HAL_STORAGE_ID_EXTERNAL_FLASH;
        } else {
            remaining_ = 0;
        }
    }

    virtual ~StorageHalInputStream() = default;

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        CHECK_TRUE(data, SYSTEM_ERROR_INVALID_ARGUMENT);
        size = std::min(size, remaining_);
        CHECK(hal_storage_read(storageId_, address_ + offset_, (uint8_t*)data, size));
        return size;
    }

    int skip(size_t size) override {
        //         size, storageId_, address_, remaining_, offset_);
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining_);
        offset_ += size;
        remaining_ -= size;
        return size;
    }

    int seek(size_t offset) override {
        //         offset, storageId_, address_, remaining_, offset_);
        CHECK_TRUE(offset <= (offset_ + remaining_), SYSTEM_ERROR_NOT_ENOUGH_DATA);
        if (offset < offset_) {
            remaining_ += (offset_ - offset);
            offset_ = offset;
        } else {
            remaining_ -= (offset - offset_);
            offset_ = offset;
        }
        return offset_;
    }

    int availForRead() override {
        return remaining_;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    hal_storage_id storageId_;
    uintptr_t address_;
    size_t remaining_;
    size_t offset_;
};

#if HAL_PLATFORM_FILESYSTEM
class FileInputStream : public InputStream {
public:
    FileInputStream()
            : fs_(nullptr),
              file_{},
              remaining_(0),
              offset_(0),
              isOpen_(false) {
    }

    virtual ~FileInputStream() {
        if (isOpen_) {
            isOpen_ = false;
            fs::FsLock lock(fs_);
            lfs_file_close(&fs_->instance, &file_);
        }
    }

    int open(const char* filename, filesystem_instance_t instance, size_t offset = 0) {
        offset_ = offset;
        fs_ = filesystem_get_instance(instance, nullptr);
        CHECK_TRUE(fs_, SYSTEM_ERROR_INVALID_ARGUMENT);

        fs::FsLock lock(fs_);
        lfs_info info = {};
        CHECK_FS(lfs_stat(&fs_->instance, filename, &info));
        CHECK_TRUE(info.type == LFS_TYPE_REG, SYSTEM_ERROR_BAD_DATA);
        CHECK_FS(lfs_file_open(&fs_->instance, &file_, filename, LFS_O_RDONLY));
        isOpen_ = true;
        remaining_ = info.size;
        return 0;
    }

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        CHECK_TRUE(data, SYSTEM_ERROR_INVALID_ARGUMENT);
        size = std::min(size, remaining_);
        fs::FsLock lock(fs_);
        CHECK_FS(lfs_file_seek(&fs_->instance, &file_, offset_, LFS_SEEK_SET));
        size = CHECK_FS(lfs_file_read(&fs_->instance, &file_, data, size));
        return size;
    }

    int skip(size_t size) override {
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining_);
        offset_ += size;
        remaining_ -= size;
        return size;
    }

    int seek(size_t offset) override {
        CHECK_TRUE(offset <= (offset_ + remaining_), SYSTEM_ERROR_NOT_ENOUGH_DATA);
        if (offset < offset_) {
            remaining_ += (offset_ - offset);
            offset_ = offset;
        } else {
            remaining_ -= (offset - offset_);
            offset_ = offset;
        }
        return offset_;
    }

    int availForRead() override {
        return remaining_;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    filesystem_t* fs_;
    lfs_file_t file_;
    size_t remaining_;
    size_t offset_;
    bool isOpen_;
};

#endif // HAL_PLATFORM_FILESYSTEM

#if HAL_PLATFORM_COMPRESSED_OTA
class InflatorStream: public InputStream {
public:
    InflatorStream(InputStream* compressedStream, size_t inflatedSize)
            : compressedStream_(compressedStream),
              inflatedSize_(inflatedSize),
              inflate_(nullptr),
              inflatedChunk_(nullptr),
              inflatedChunkSize_(0),
              posInChunk_(0),
              offset_(0) {
    }

    int init() {
        // inflate_ will stay nullptr in case something goes wrong in inflate_create
        return inflate_create(&inflate_, nullptr, [](const char* data, size_t size, void* ctx) -> int {
            auto self = static_cast<InflatorStream*>(ctx);
            return self->inflatedChunk(data, size);
        }, this);
    }

    virtual ~InflatorStream() {
        if (inflate_) {
            inflate_destroy(inflate_);
        }
    }

    int read(char* data, size_t size) override {
        size = CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        CHECK_TRUE(data, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(waitEvent(InputStream::READABLE, 0));
        size = std::min<size_t>(size, availForRead());
        memcpy(data, inflatedChunk_ + posInChunk_, size);
        return size;
    }

    int skip(size_t size) override {
        size = std::min(size, toInflate());
        size_t skipped = 0;
        while (size > 0) {
            CHECK(waitEvent(InputStream::READABLE, 0));
            size_t toSkip = std::min<size_t>(size, availForRead());
            posInChunk_ += toSkip;
            offset_ += toSkip;
            size -= toSkip;
            skipped += toSkip;
        }
        return skipped;
    }

    int seek(size_t offset) override {
        CHECK_TRUE(offset == 0 || (offset >= offset_ && offset <= inflatedSize_), SYSTEM_ERROR_NOT_ALLOWED);
        if (offset == 0) {
            return rewind();
        } else {
            return skip(offset - offset_);
        }
    }

    int availForRead() override {
        return inflatedChunkSize_ - posInChunk_;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!toInflate()) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        if (CHECK(availForRead()) == 0) {
            CHECK(inflateUntilNextChunk());   
        }
        return InputStream::READABLE;
    }

private:
    size_t toInflate() {
        return inflatedSize_ - offset_;
    }

    int rewind() {
        CHECK_TRUE(inflate_ && compressedStream_, SYSTEM_ERROR_INVALID_STATE);
        CHECK(inflate_reset(inflate_));
        CHECK(compressedStream_->seek(0));
        offset_ = 0;
        posInChunk_ = 0;
        inflatedChunkSize_ = 0;
        inflatedChunk_ = nullptr;
        return 0;
    }

    int inflateUntilNextChunk() {
        CHECK_TRUE(inflate_ && compressedStream_, SYSTEM_ERROR_INVALID_STATE);

        char tmp[256];

        while (true) {
            size_t compressedChunk = 0;
            if (compressedStream_->waitEvent(InputStream::READABLE) == InputStream::READABLE) {
                compressedChunk = CHECK(compressedStream_->peek(tmp, sizeof(tmp)));
            }
            size_t compressedPos = 0;
            int r = 0;
            do {
                size_t n = compressedChunk - compressedPos;
                r = inflate_input(inflate_, tmp + compressedPos, &n, INFLATE_HAS_MORE_INPUT);
                CHECK(r);
                compressedPos += n;
                compressedStream_->skip(n);
            } while (compressedPos < compressedChunk && r != INFLATE_HAS_MORE_OUTPUT);
            if (r == INFLATE_HAS_MORE_OUTPUT && availForRead() > 0) {
                break;
            }
        }
        return availForRead();
    }

    int inflatedChunk(const char* data, size_t size) {
        if (availForRead() == 0 && inflatedChunk_ && posInChunk_ > 0 && posInChunk_ == size) {
            // Acknowledge inflated chunk as consumed
            inflatedChunk_ = nullptr;
            posInChunk_ = 0;
            inflatedChunkSize_ = 0;
            return size;
        }
        inflatedChunkSize_ = size;
        inflatedChunk_ = data;
        posInChunk_ = 0;
        return 0;
    }

private:
    InputStream* compressedStream_;
    size_t inflatedSize_;
    inflate_ctx* inflate_;
    const char* inflatedChunk_;
    size_t inflatedChunkSize_;
    size_t posInChunk_;
    size_t offset_;
};

#endif // HAL_PLATFORM_COMPRESSED_OTA

class ProxyInputStream : public InputStream {
public:
    ProxyInputStream(InputStream* stream, size_t offset, size_t size)
            : stream_(stream),
              baseOffset_(offset),
              offset_(0),
              size_(size),
              error_(0) {
        if (stream_) {
            error_ = stream_->seek(baseOffset_);
        } else {
            error_ = SYSTEM_ERROR_INVALID_STATE;
        }
    }

    virtual ~ProxyInputStream() = default;

    int read(char* data, size_t size) override {
        CHECK(error_);
        if (size_ - offset_ == 0) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, size_ - offset_);
        size_t haveRead = CHECK(stream_->read(data, size));
        offset_ += haveRead;
        return haveRead;
    }

    int peek(char* data, size_t size) override {
        CHECK(error_);
        CHECK_TRUE(data, SYSTEM_ERROR_INVALID_ARGUMENT);
        if (size_ - offset_ == 0) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, size_ - offset_);
        return stream_->peek(data, size);
    }

    int skip(size_t size) override {
        CHECK(error_);
        if (size_ - offset_ == 0) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, size_ - offset_);
        size_t skipped = CHECK(stream_->skip(size));
        offset_ += skipped;
        return skipped;
    }

    int seek(size_t offset) override {
        CHECK(error_);
        //         offset, storageId_, address_, remaining_, offset_);
        CHECK_TRUE(offset <= size_, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        offset_ = CHECK(stream_->seek(baseOffset_ + offset));
        return offset_;
    }

    int availForRead() override {
        CHECK(error_);
        return std::min<int>(stream_->availForRead(), size_ - offset_);
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        CHECK(error_);

        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        int r = CHECK(stream_->waitEvent(flags, timeout));
        if ((r & InputStream::READABLE) && availForRead() == 0) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return r;
    }

private:
    InputStream* stream_;
    size_t baseOffset_;
    size_t offset_;
    size_t size_;
    int error_;
};

} // particle
