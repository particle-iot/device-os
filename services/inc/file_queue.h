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
        if (ret>=0) {
            QueueEntry entry = { .size = uint16_t(size+sizeof(QueueEntry)), .flags = QueueEntry::ACTIVE };
            ret = file_write(&write_file_, &entry, sizeof(entry));
            ret = preserve_error(file_write(&write_file_, item, size), ret);
            ret = preserve_error(lfs_file_close(lfs(), &write_file_), ret);   // always close even if there are other errors
        }
        LOG(INFO, "add item to file queue %s, size %d, result %d", path_, size, ret);
        return ret;
    }

    /**
     * Retrieve the front queue entry in the file.
     *
     * @param entry	The entry to populate
     * @param buffer	the buffer to fill with the contents of the entry.
     * @param length	The length of the buffer. Only as much data
     * as will fit into the buffer is copied.
     * @return SYSTEM_ERROR_NOT_FOUND when there is no such entry.
     */
    int front(QueueEntry& entry, void* buffer, uint16_t length) {
        FsLock lk(fs_);
        int ret = _front(entry, true);
        if (ret>=0) {
            int remaining = entry.size-sizeof(entry);
        	if (remaining>length) {
        		LOG(ERROR,  "Buffer length %d is too small. Need at least %d", length, remaining);
        		ret = LFS_ERR_INVAL;
        	} else {
        		ret = lfs_file_read(lfs(), &read_file_, buffer, remaining);
				if (ret!=int(remaining)) {
					LOG(ERROR, "Incomplete queue record. Expected length %d but read %d", remaining, ret);
					clear();
					ret = LFS_ERR_IO;
				} else {
					ret = 0;	// no error
					LOG(INFO, "Retrieved entry from file queue, size %d", entry.size);
				}
        	}
            ret = preserve_error(lfs_file_close(lfs(), &read_file_), ret);
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
        if (!ret) {
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
				ret = lfs_file_seek(lfs(), &read_file_, entry.size-sizeof(entry), LFS_SEEK_CUR);
				if (ret>=0) {
					ret = 0;
				}
			}
			if (ret || !keepOpen) {
				ret = preserve_error(lfs_file_close(lfs(), &read_file_), ret);
			}
        }
        return ret;
    }

    /**
     * Remove the front item in the queue. This is done by clearing the ACTIVE flag in the queue entry.
     */
    int popFront() {
        FsLock lk(fs_);
        QueueEntry entry;
        int ret = _front(entry, true);
        // file position points to the start of the file data for the first ACTIVE entry.
        int offset = lfs_file_tell(lfs(), &read_file_);
        ret = lfs_file_close(lfs(), &read_file_);

        bool isLast = false;
        if (ret>=0) {
            int ret = lfs_file_open(lfs(), &read_file_, path_, LFS_O_RDWR);
            if (!ret) {
                offset -= sizeof(entry);	// offset is at the start of the entry
                ret = lfs_file_seek(lfs(), &read_file_, offset, LFS_SEEK_SET);
                if (ret>=0) ret = 0;
                entry.flags &= ~QueueEntry::ACTIVE;
                if (!ret) {
                	ret = lfs_file_write(lfs(), &read_file_, &entry, sizeof(entry));
                	if (ret==sizeof(entry))
                		ret = 0;
                }
                lfs_soff_t length = lfs_file_size(lfs(), &read_file_);
                isLast = (length>=0 && length==(offset+entry.size));
                ret = preserve_error(lfs_file_close(lfs(), &read_file_), ret);
            }
        }
        if (isLast && !ret) {
        	LOG(INFO, "Removed last entry from file queue, deleting file %s", path_);
            // when the last entry has been cleared remove the file
            ret = clear();
        }
        return ret;
    }

    int clear() {
    	FsLock lk(fs_);
    	return lfs_remove(lfs(), path_);
    }

private:

    int file_write(lfs_file* file, void* data, uint16_t size) {
		int ret = lfs_file_write(lfs(), file, data, size);
		if (ret<0) {
			LOG(ERROR, "Error writing %d bytes to file %s: error %d", size, path_, ret);
		}
		else if (ret!=size) {
			LOG(ERROR, "wrote only %d bytes to file %s, expected %d bytes to be written", ret, path_, size);
			ret = LFS_ERR_IO;
		}
		else {
			ret = 0;
		}
		return ret;
	}

    lfs_t* lfs() {
        _open();
        return &fs_->instance;
    }

    static int preserve_error(int first, int second) {
    	return first<0 ? first : second;
    }

    filesystem_t* fs_ = nullptr;
    lfs_file_t read_file_ = {};
    lfs_file_t write_file_ = {};
    const char* path_;
};

} // fs
} // particle

#endif
