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

#if HAL_PLATFORM_FILESYSTEM

#include "file_util.h"

#include "nanopb_misc.h"
#include "bytes2hexbuf.h"
#include "scope_guard.h"
#include "check.h"

#include <pb_decode.h>
#include <pb_encode.h>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace particle {

namespace {

const size_t DUMP_BYTES_PER_LINE = 16;

void dumpLine(const char* data, size_t size, size_t offs) {
    if (size == 0) {
        return;
    }
    if (size > DUMP_BYTES_PER_LINE) {
        size = DUMP_BYTES_PER_LINE;
    }
    char line[DUMP_BYTES_PER_LINE * 4 + 14];
    memset(line, ' ', sizeof(line));
    char* d = line;
    d += snprintf(line, sizeof(line), "%08x", (unsigned)offs);
    *d = ':';
    d += 2;
    for (size_t i = 0; i < size; ++i) {
        bytes2hexbuf_lower_case((const uint8_t*)data + i, 1, d);
        d += 3;
    }
    d = line + DUMP_BYTES_PER_LINE * 3 + 10;
    *d = '|';
    d += 2;
    for (size_t i = 0; i < size; ++i) {
        char c = data[i];
        if (!std::isprint((unsigned char)c)) {
            c = '.';
        }
        *d++ = c;
    }
    *d++ = '\r';
    *d++ = '\n';
    LOG_WRITE(TRACE, line, d - line);
}

} // unnamed

int openFile(lfs_file_t* file, const char* path, unsigned flags) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    lfs_info info = {};
    int r = lfs_stat(&fs->instance, path, &info);
    if (r != LFS_ERR_OK) {
        const auto p = strrchr(path, '/');
        if (p && p != path) {
            const auto dirPath = strndup(path, p - path);
            CHECK_TRUE(dirPath, SYSTEM_ERROR_NO_MEMORY);
            SCOPE_GUARD({
                free(dirPath);
            });
            r = lfs_mkdir(&fs->instance, dirPath);
            CHECK_TRUE(r == LFS_ERR_OK || r == LFS_ERR_EXIST, SYSTEM_ERROR_FILE);
        }
        flags |= LFS_O_CREAT;
    } else if (info.type != LFS_TYPE_REG) {
        return SYSTEM_ERROR_FILE;
    }
    r = lfs_file_open(&fs->instance, file, path, flags);
    CHECK_TRUE(r == LFS_ERR_OK, SYSTEM_ERROR_FILE);
    return 0;
}

int dumpFile(const char* path) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    const fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    lfs_file_t file = {};
    int r = lfs_file_open(&fs->instance, &file, path, LFS_O_RDONLY);
    CHECK_TRUE(r == LFS_ERR_OK, SYSTEM_ERROR_FILE);
    SCOPE_GUARD({
        lfs_file_close(&fs->instance, &file);
    });
    const auto size = lfs_file_size(&fs->instance, &file);
    CHECK_TRUE(size >= 0, SYSTEM_ERROR_FILE);
    char buf[DUMP_BYTES_PER_LINE] = {};
    size_t offs = 0;
    while (offs < (size_t)size) {
        const size_t n = std::min(sizeof(buf), (size_t)size - offs);
        const lfs_ssize_t r = lfs_file_read(&fs->instance, &file, buf, n);
        CHECK_TRUE(r == (lfs_ssize_t)n, SYSTEM_ERROR_FILE);
        dumpLine(buf, n, offs);
        offs += n;
    }
    return 0;
}

int decodeMessageFromFile(lfs_file_t* file, const pb_field_t* fields, void* msg) {
    const auto strm = pb_istream_init(nullptr);
    CHECK_TRUE(strm, SYSTEM_ERROR_NO_MEMORY);
    SCOPE_GUARD({
        pb_istream_free(strm, nullptr);
    });
    CHECK_TRUE(pb_istream_from_file(strm, file, nullptr), SYSTEM_ERROR_FILE);
    CHECK_TRUE(pb_decode(strm, fields, msg), SYSTEM_ERROR_FILE);
    return 0;
}

int encodeMessageToFile(lfs_file_t* file, const pb_field_t* fields, const void* msg) {
    const auto strm = pb_ostream_init(nullptr);
    CHECK_TRUE(strm, SYSTEM_ERROR_NO_MEMORY);
    SCOPE_GUARD({
        pb_ostream_free(strm, nullptr);
    });
    CHECK_TRUE(pb_ostream_from_file(strm, file, nullptr), SYSTEM_ERROR_FILE);
    CHECK_TRUE(pb_encode(strm, fields, msg), SYSTEM_ERROR_FILE);
    return 0;
}

} // particle

#endif // HAL_PLATFORM_FILESYSTEM
