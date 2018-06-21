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

#ifndef SERVICES_TLV_FILE_H
#define SERVICES_TLV_FILE_H

#include "filesystem.h"
#include <stdio.h>

namespace particle { namespace services { namespace settings {

static constexpr uint32_t TLV_FILE_MAGICK = 0x714f11e5;
static constexpr uint32_t TLV_HEADER_MAGICK = 0x4ead;

class TlvFile {
public:
    TlvFile(const char* path);
    ~TlvFile();

    int init();
    int deInit();

    int purge();
    int sync();

    uint16_t currentVersion() const;
    int fileVersion();

    ssize_t size();

    ssize_t get(uint16_t key, uint8_t* value, uint16_t length, int index = 0);
    int set(uint16_t key, const uint8_t* value, uint16_t length, int index = -1);
    int add(uint16_t key, const uint8_t* value, uint16_t length);
    int del(uint16_t key, int index = -1);

private:
    struct FileFooter {
        uint32_t reserved;  /* CRC32? */
        uint32_t size;
        uint16_t reserved1;
        uint16_t version;
        uint32_t magick;
    } __attribute__((__packed__));
    static_assert(sizeof(FileFooter) == sizeof(uint32_t) * 4, "sizeof(FileFooter) != 16");

    struct TlvHeader {
        uint16_t magick;
        uint16_t key;
        uint16_t length;
        uint16_t reserved;
    } __attribute__((__packed__));
    static_assert(sizeof(TlvHeader) == sizeof(uint32_t) * 2, "sizeof(TlvHeader) != 8");

private:
    lfs_t* lfs();

    int open();
    int validate();
    int close();

    int mkdir(char* dir);

    ssize_t find(uint16_t key, int index, uint16_t* dataSize);
    int readFooter(FileFooter& footer);

    ssize_t seek(ssize_t offset, int whence = SEEK_SET);
    ssize_t read(uint8_t* buf, size_t length);
    ssize_t write(const uint8_t* buf, size_t length);

private:
    char* path_;

    bool open_ = false;
    filesystem_t* fs_ = nullptr;
    lfs_file_t file_ = {};
};

} } } /* namespace particle::services::settings */

#endif /* SERVICES_TLV_FILE_H */
