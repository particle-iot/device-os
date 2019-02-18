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

#include "hal_platform.h"

#if (defined(HAL_PLATFORM_FILESYSTEM) && HAL_PLATFORM_FILESYSTEM == 1)

#include "tlv_file.h"
#include <cstring>
#include "service_debug.h"
#include "system_error.h"
#include <algorithm>

/* FIXME: once filesystem interface is finalized, convert the implementation not to use
 * LittleFS API.
 */

using namespace particle::services::settings;
using namespace particle::fs;

TlvFile::TlvFile(const char* path) {
    SPARK_ASSERT(path != nullptr);
    path_ = strdup(path);
    SPARK_ASSERT(path_ != nullptr);
}

TlvFile::~TlvFile() {
    if (path_) {
        free(path_);
        path_ = nullptr;
    }
}

int TlvFile::init() {
    auto fs = filesystem_get_instance(nullptr);
    SPARK_ASSERT(fs);

    FsLock lk(fs);

    fs_ = fs;

    SPARK_ASSERT(!filesystem_mount(fs_));

    char* p = strrchr(path_, '/');
    int ret = 0;
    if (p != nullptr && p != path_) {
        *p = '\0';
        ret = mkdir(path_);
        *p = '/';
    }

    if (!ret) {
        ret = open();
    }

    return ret;
}

int TlvFile::deInit() {
    return close();
}

int TlvFile::purge() {
    FsLock lk(fs_);

    close();

    return lfs_remove(lfs(), path_);
}

int TlvFile::sync() {
    FsLock lk(fs_);

    if (open_) {
        int r = lfs_file_sync(lfs(), &file_);
        if (r) {
            /* Reopen just in case */
            close();
            open();
        }
        return r;
    }

    return SYSTEM_ERROR_INVALID_STATE;
}

ssize_t TlvFile::size() {
    FsLock lk(fs_);

    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    return lfs_file_size(lfs(), &file_);
}

ssize_t TlvFile::get(uint16_t key, uint8_t* value, uint16_t length, int index) {
    FsLock lk(fs_);

    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (value == nullptr && length != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint16_t dataSize;
    ssize_t ret = SYSTEM_ERROR_NOT_FOUND;
    ssize_t pos = find(key, index, &dataSize);
    if (pos >= 0) {
        /* Found it */
        const size_t toRead = std::min(length, dataSize);
        if (toRead) {
            ret = seek(pos + sizeof(TlvHeader));
            if (ret >= 0) {
                ret = read(value, toRead);
            }
        }
    }

    return ret;
}

int TlvFile::set(uint16_t key, const uint8_t* value, uint16_t length, int index) {
    FsLock lk(fs_);

    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    /* Delete previous entry */
    int ret = del(key, index);
    if (!(ret == 0 || ret == SYSTEM_ERROR_NOT_FOUND)) {
        return ret;
    }

    return add(key, value, length);
}

int TlvFile::add(uint16_t key, const uint8_t* value, uint16_t length) {
    FsLock lk(fs_);

    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    FileFooter footer;
    int ret = readFooter(footer);
    if (ret < 0) {
        return ret;
    }
    ssize_t pos = footer.size;

    if (pos < 0) {
        return pos;
    }

    ret = seek(pos);
    if (ret < 0) {
        return ret;
    }

    TlvHeader header = {};
    header.magick = TLV_HEADER_MAGICK;
    header.key = key;
    header.length = length;

    /* Write entry header */
    ret = write((const uint8_t*)&header, sizeof(header));
    if (ret < 0) {
        return ret;
    }
    /* Write data */
    ret = write((const uint8_t*)value, length);
    if (ret < 0) {
        return ret;
    }
    /* Write file footer */
    footer.magick = TLV_FILE_MAGICK;
    footer.size += sizeof(header) + length;
    ret = write((const uint8_t*)&footer, sizeof(footer));
    if (ret < 0) {
        return ret;
    }

    return sync();
}

int TlvFile::del(uint16_t key, int index) {
    FsLock lk(fs_);

    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int ret = 0;

    for (;;) {
        uint16_t dataSize = 0;
        ssize_t pos = find(key, index, &dataSize);
        if (pos < 0) {
            return pos;
        }

        /* FIXME: this will only work on LittleFS. Provide a different implementation later
         * when filesystem API is finalized and TlvFile implementation is refactored not to use
         * LittleFS API.
         */
        const ssize_t fileSize = size();

        FileFooter footer;
        ret = readFooter(footer);

        while (ret == 0) {
            lfs_file_t f;
            ret = lfs_file_open(lfs(), &f, path_, LFS_O_RDONLY);
            if (ret) {
                break;
            }

            size_t wpos = pos;
            size_t rpos = wpos + sizeof(TlvHeader) + dataSize;

            ret = seek(wpos);
            if (ret < 0) {
                break;
            }

            ret = lfs_file_seek(lfs(), &f, rpos, LFS_SEEK_SET);
            if (ret < 0) {
                break;
            }

            for (; rpos < footer.size && ret > 0; ++wpos, ++rpos) {
                uint8_t tmp;
                ret = lfs_file_read(lfs(), &f, &tmp, sizeof(tmp));
                if (ret < 0) {
                    break;
                }

                ret = write(&tmp, sizeof(tmp));
                if (ret < 0) {
                    break;
                }
            }

            if (ret > 0) {
                /* Write footer */
                footer.size -= sizeof(TlvHeader) + dataSize;
                ret = write((const uint8_t*)&footer, sizeof(footer));
                if (ret < 0) {
                    break;
                }
                /* Truncate */
                ret = lfs_file_truncate(lfs(), &file_, fileSize - (sizeof(TlvHeader) + dataSize));
                if (ret < 0) {
                    break;
                }
                ret = sync();
            }

            lfs_file_close(lfs(), &f);

            break;
        }


        if (ret || index >= 0) {
            break;
        }
    }

    return ret;
}

lfs_t* TlvFile::lfs() {
    SPARK_ASSERT(fs_);
    return &fs_->instance;
}

int TlvFile::open() {
    FsLock lk(fs_);

    if (open_) {
        return 0;
    }

    /* Open */
    int r = lfs_file_open(lfs(), &file_, path_, LFS_O_CREAT | LFS_O_RDWR);
    if (r) {
        return r;
    }

    open_ = true;
    FileFooter footer = {};

    if (!validate()) {
        goto open_done;
    }

    footer.magick = TLV_FILE_MAGICK;
    footer.size = 0;

    /* Validation failed, create anew */
    r = lfs_file_truncate(lfs(), &file_, 0);
    if (r < 0) {
        goto open_done;
    }

    r = seek(0);
    if (r < 0) {
        goto open_done;
    }

    r = lfs_file_write(lfs(), &file_, &footer, sizeof(footer));
    if (r < 0) {
        goto open_done;
    }

    r = sync();

open_done:
    if (r) {
        lfs_file_close(lfs(), &file_);
        open_ = false;
    }
    return r;
}

int TlvFile::validate() {
    FileFooter footer = {};
    int ret = readFooter(footer);
    if (!ret) {
        if (footer.magick != TLV_FILE_MAGICK) {
            ret = SYSTEM_ERROR_BAD_DATA;
        }
    }

    return ret;
}

int TlvFile::close() {
    FsLock lk(fs_);

    if (!open_) {
        return 0;
    }

    /* Close */

    open_ = false;

    return lfs_file_close(lfs(), &file_);
}

int TlvFile::mkdir(char* dir) {
    int ret = SYSTEM_ERROR_NONE;

    const size_t sz = strlen(dir);

    if (dir == nullptr || *dir == '\0' || (*dir == '/' && sz == 1)) {
        return ret;
    }

    for (char* p = strchrnul(dir + 1, '/'); p != nullptr && (size_t)(p - dir) <= sz && ret == 0; p = strchrnul(p + 1, '/')) {
        *p = '\0';

        struct lfs_info info;
        ret = lfs_stat(lfs(), dir, &info);

        if (ret < 0) {
            /* Directory does not exist */
            ret = lfs_mkdir(lfs(), dir);
        } else {
            /* Check if a directory */
            if (info.type != LFS_TYPE_DIR) {
                ret = SYSTEM_ERROR_INVALID_STATE;
            }
        }

        *p = '/';
    }

    return ret;
}

ssize_t TlvFile::seek(ssize_t offset, int whence) {
    return lfs_file_seek(lfs(), &file_, offset, whence);
}

ssize_t TlvFile::read(uint8_t* buf, size_t length) {
    return lfs_file_read(lfs(), &file_, buf, length);
}

ssize_t TlvFile::write(const uint8_t* buf, size_t length) {
    ssize_t r = lfs_file_write(lfs(), &file_, buf, length);
    if (r < 0) {
        /* Write operation failed. sync() should discard this cached write */
        if (sync()) {
            /* If sync fails, try to reopen the file */
            close();
            open();
        }
    }

    return r;
}

int TlvFile::readFooter(FileFooter& footer) {
    ssize_t sz = size();
    if (sz < 0) {
        return sz;
    }

    /* Validate */
    if (sz >= (ssize_t)sizeof(footer)) {
        ssize_t pos = seek(sz - sizeof(footer));
        if (pos >= 0) {
            ssize_t rd = read((uint8_t*)&footer, sizeof(footer));
            if (rd == sizeof(footer)) {
                return 0;
            }
        }
    }

    return SYSTEM_ERROR_BAD_DATA;
}

ssize_t TlvFile::find(uint16_t key, int index, uint16_t* dataSize) {
    FileFooter footer;
    int r = readFooter(footer);
    if (r) {
        return r;
    }

    ssize_t candidatePos = -1;
    int candidateIdx = -1;
    uint16_t candidateSize = 0;

    TlvHeader header;
    for (ssize_t pos = 0; pos >= 0 && (pos + sizeof(TlvHeader)) <= footer.size;) {
        r = seek(pos);
        if (r < 0) {
            return r;
        }

        ssize_t rd = read((uint8_t*)&header, sizeof(header));
        if (rd < (ssize_t)sizeof(TlvHeader)) {
            return SYSTEM_ERROR_BAD_DATA;
        }

        if (header.magick != TLV_HEADER_MAGICK) {
            /* Attempt to recover */
            pos += sizeof(uint16_t);
            continue;
        }

        if (header.key == key) {
            candidatePos = pos;
            ++candidateIdx;
            candidateSize = header.length;
            if (index >= 0 && candidateIdx >= index) {
                break;
            }
        }

        pos += sizeof(TlvHeader) + header.length;
    }

    if ((index >= 0 && candidateIdx == index) || (index < 0 && candidateIdx >= 0)) {
        if (dataSize) {
            *dataSize = candidateSize;
        }
        return candidatePos;
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

#endif /* HAL_PLATFORM_FILESYSTEM == 1 */
