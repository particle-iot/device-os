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

#include "hal_platform.h"

#if (defined(HAL_PLATFORM_FILESYSTEM) && HAL_PLATFORM_FILESYSTEM == 1)

#include <stdint.h>
#include "service_debug.h"
#include "system_error.h"
#include "filesystem.h"

namespace particle {

namespace fs {

/**
 * Implements a queue on top of a file.
 */
class FileQueue {

public:

    struct __attribute__((__packed__)) QueueEntry {
        enum Flags {
            ACTIVE = 1<<0,		// when this bit is set the entry is active. When the bit is reset, the entry is not valid and can ce ignored.
        };

        uint16_t size;
        uint16_t flags;

    };

    FileQueue(const char* path) : path_(path)
    {
    }

    ~FileQueue() {

    }

    int _open() {
        auto fs = filesystem_get_instance(nullptr);
        SPARK_ASSERT(fs);

        FsLock lk(fs);
        fs_ = fs;

        SPARK_ASSERT(!filesystem_mount(fs_));
        return 0;
    }


    /**
     * Add an entry to the back of the queue.
     */
    int pushBack(void* item, uint16_t size) {
        // append a new item to the file
        FsLock lk(fs_);
        _open();
        int ret = lfs_file_open(lfs(), &write_file_, path_, LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
        if (!ret) {
            QueueEntry entry = { .size = size, .flags = QueueEntry::ACTIVE };
            ret = lfs_file_write(lfs(), &write_file_, &entry, sizeof(entry));
            ret = ret || lfs_file_write(lfs(), &write_file_, item, size);
            ret = lfs_file_close(lfs(), &write_file_) || ret;   // always close even if there are other errors
        }
        return ret;
    }

    /**
     * Retrieve the front queue entry in the file.
     *
     * @param entry	The entry to populate
     * @param buffer	When non-zero, represents a buffer to fill
     * with the contents of the entry.
     * @param buffer	The length of the buffer. Only as much data
     * as will fit into the buffer is copied.
     * @return SYSTEM_ERROR_NOT_FOUND when there is no such entry.
     */
    int front(QueueEntry& entry, void* buffer, size_t length) {
        FsLock lk(fs_);
        int ret = _front(entry, true);
        if (!ret) {
            ret = lfs_file_read(lfs(), &read_file_, buffer, length);
            if (ret!=int(length)) {
                ret = LFS_ERR_IO;
            }
            ret = lfs_file_close(lfs(), &read_file_) || ret;
        }
        return ret;
    }

    /**
     * Locate the offset in the file of the first non-expired entry
     * and retrieve the entry.
     *
     * @return >=0 when the entry was successfully retrieved.
     * <0 for an error condition.
     */
    int _front(QueueEntry& entry, bool keepOpen) {
        _open();
        int ret = lfs_file_open(lfs(), &read_file_, path_, LFS_O_RDONLY);
        int pos = 0;
        while (!ret) {
            ret = lfs_file_read(lfs(), &read_file_, &entry, sizeof(entry));
            if (ret==sizeof(QueueEntry)) {
                ret = 0; 	// all good
                if (entry.flags & QueueEntry::ACTIVE) {
                    break;
                }
            } else {
                ret = LFS_ERR_IO;
                break;	// problem reading
            }
            pos += sizeof(QueueEntry);
            pos += entry.size;
            ret = lfs_file_seek(lfs(), &read_file_, entry.size, LFS_SEEK_CUR);
            if (ret>=0)
                ret = 0;
        }
        if (ret || !keepOpen) {
            ret = lfs_file_close(lfs(), &read_file_) || ret;
        }
        return ret || pos;
    }

    /**
     * Remove the front item in the queue.
     */
    int popFront() {
        FsLock lk(fs_);
        QueueEntry entry;
        int ret = _front(entry, false);
        bool isLast = false;
        if (!ret) {
            int offset = ret;
            int ret = lfs_file_open(lfs(), &read_file_, path_, LFS_O_WRONLY);
            if (!ret) {
                offset += offsetof(QueueEntry, flags);
                ret = lfs_file_seek(lfs(), &read_file_, offset, LFS_SEEK_SET);
                if (ret>=0) ret = 0;
                entry.flags &= ~QueueEntry::ACTIVE;
                ret = lfs_file_write(lfs(), &read_file_, &entry.flags, sizeof(entry.flags)) || ret;
                lfs_soff_t length = lfs_file_size(lfs(), &read_file_);
                isLast = (length>=0 && length==(offset+int(sizeof(QueueEntry::flags))+entry.size));
                ret = lfs_file_close(lfs(), &read_file_) || ret;
            }
        }
        if (isLast && !ret) {
            // when the last entry has been cleared remove the file
            ret = lfs_remove(lfs(), path_);
        }
        return ret;
    }

private:

    lfs_t* lfs() {
        return &fs_->instance;
    }

    filesystem_t* fs_ = nullptr;
    lfs_file_t read_file_ = {};
    lfs_file_t write_file_ = {};
    const char* path_;
};

} // fs
} // particle

#endif
