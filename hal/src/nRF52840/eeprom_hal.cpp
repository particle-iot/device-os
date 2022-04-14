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

#include "eeprom_hal.h"
#include "eeprom_hal_impl.h"
#include "filesystem.h"
#include "static_recursive_mutex.h"

#include <algorithm>
#include <mutex>
#include <cstring>

namespace {

using namespace particle::fs;

class EepromFile {
public:
    EepromFile() {
        init();
    }

    ~EepromFile() {
        deinit();
    }

    ssize_t read(size_t offset, uint8_t* buffer, size_t size) {
        FsLock lk(fs_);
        open(LFS_O_RDONLY);
        ssize_t r = seek(offset);
        if (r < 0) {
            close();
            return r;
        }
        r = lfs_file_read(lfs(), &file_, buffer, size);
        close();
        return r;
    }

    ssize_t write(size_t offset, const uint8_t* buffer, size_t size) {
        FsLock lk(fs_);

        open(LFS_O_WRONLY);

        ssize_t r = seek(offset);
        if (r < 0) {
            close();
            return r;
        }

        r = lfs_file_write(lfs(), &file_, buffer, size);
        if (r < 0) {
            /* Error */
            LOG_DEBUG(ERROR, "Failed to write to EEPROM: %d", r);
        }

        close();

        return r;
    }

    ssize_t recreate(void) {
        FsLock lk(fs_);

        struct lfs_info info;
        int r = lfs_stat(lfs(), path_, &info);
        if (r == 0) {
            r = lfs_remove(lfs(), EEPROM_FILE_PATH);
            if (r < 0) {
                /* Error */
                LOG_DEBUG(ERROR, "Failed to remove EEPROM file: %d", r);
                return r;
            }
        }

        int flags = LFS_O_CREAT | LFS_O_RDWR;

        SPARK_ASSERT(open(flags));

        LOG_DEBUG(INFO, "Initializing empty EEPROM");
        /* Fill with 0xff for compatibility with raw flash EEPROM */
        uint8_t tmp = 0xff;
        for (unsigned offset = 0; offset < EEPROM_FILE_SIZE;) {
            r = lfs_file_write(lfs(), &file_, &tmp, sizeof(tmp));
            SPARK_ASSERT(r > 0);
            offset += r;
        }

        SPARK_ASSERT(close());

        return r;
    }


private:

    bool open(int flags) {
        open_ = lfs_file_open(lfs(), &file_, path_, flags) == 0;
        return open_;
    }

    bool close() {
        if (!open_) {
            return false;
        }
        open_ = false;
        return lfs_file_close(lfs(), &file_) == 0;
    }

    ssize_t seek(size_t offset) {
        return lfs_file_seek(lfs(), &file_, offset, LFS_SEEK_SET);
    }

    void init() {
        fs_ = filesystem_get_instance(nullptr);
        SPARK_ASSERT(fs_);

        FsLock lk(fs_);
        SPARK_ASSERT(!filesystem_mount(fs_));

        LOG_DEBUG(INFO, "Filesystem mounted");

        int r = lfs_mkdir(lfs(), EEPROM_DIR_PATH);
        SPARK_ASSERT((r == 0 || r == LFS_ERR_EXIST));

        /* Check that /sys/eeprom.bin exists */
        struct lfs_info info;
        r = lfs_stat(lfs(), path_, &info);

        if (r) {
            recreate();
        }
    }

    void deinit() {
        FsLock lk(fs_);

        lfs_file_close(lfs(), &file_);
        filesystem_unmount(fs_);
    }

    lfs_t* lfs() {
        return &fs_->instance;
    }

    filesystem_t* fs_;
    lfs_file_t file_;
    bool open_ = false;
    static constexpr const char* path_ = EEPROM_FILE_PATH;
};

EepromFile& eeprom() {
    static EepromFile eeprom;
    return eeprom;
}

} // namespace

void HAL_EEPROM_Init(void) {
    // eeprom file is created in constructor of EepromFile
}

uint8_t HAL_EEPROM_Read(uint32_t index) {
    uint8_t data;
    HAL_EEPROM_Get(index, &data, 1);
    return data;
}

void HAL_EEPROM_Write(uint32_t index, uint8_t data) {
    HAL_EEPROM_Put(index, &data, 1);
}

size_t HAL_EEPROM_Length() {
    return EEPROM_FILE_SIZE;
}

void HAL_EEPROM_Get(uint32_t index, void *data, size_t length) {
    // std::memset(data, FLASH_ERASED_VALUE, length);

    if (index >= EEPROM_FILE_SIZE) {
        return;
    }

    eeprom().read(index, (uint8_t *)data, length);
}

void HAL_EEPROM_Put(uint32_t index, const void *data, size_t length) {
    if (index >= EEPROM_FILE_SIZE) {
        return;
    }

    eeprom().write(index, (const uint8_t *)data, length);
}

void HAL_EEPROM_Clear() {
    eeprom().recreate();
}

bool HAL_EEPROM_Has_Pending_Erase() {
    return false;
}

void HAL_EEPROM_Perform_Pending_Erase() {

}
