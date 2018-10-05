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

#include "file_util.h"

#include "nanopb_misc.h"
#include "scope_guard.h"
#include "check.h"

#include <pb_decode.h>
#include <pb_encode.h>

#include <cstdlib>
#include <cstring>

namespace particle {

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
