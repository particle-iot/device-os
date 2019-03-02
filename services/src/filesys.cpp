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

#include "filesys.h"

#include "scope_guard.h"
#include "check.h"

int file_remove(const char *path) { return -1; }

int file_rename(const char *oldpath, const char *newpath) { return -1; }

int file_stat(const char *path, struct lfs_info *info) { return -1; }

int file_open(lfs_file_t *file, const char* path, unsigned flags) {
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

int file_close(lfs_file_t *file) { return -1; }

int dir_open(lfs_dir_t *dir, const char *path) {
    //LOG(INFO, "dir_open(%s)", path);
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_open(&fs->instance, dir, path);
}

int dir_close(lfs_dir_t *dir) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_close(&fs->instance, dir);
}

int dir_read(lfs_dir_t *dir, struct lfs_info *info) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_read(&fs->instance, dir, info);
}
