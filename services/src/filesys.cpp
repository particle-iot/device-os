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

int file_remove(const char *path) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_remove(&fs->instance, path);
}

int file_rename(const char *oldpath, const char *newpath) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_rename(&fs->instance, oldpath, newpath);
}

int file_stat(const char *path, struct lfs_info *info) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_stat(&fs->instance, path, info);
}

int file_open(lfs_file_t *file, const char* path, unsigned flags) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_open(&fs->instance, file, path, flags);
}

int file_close(lfs_file_t *file) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_close(&fs->instance, file);
}

int file_sync(lfs_file_t *file) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_sync(&fs->instance, file);
}

int32_t file_read(lfs_file_t *file, void *buf, uint32_t len) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_read(&fs->instance, file, buf, len);
}

int32_t file_write(lfs_file_t *file, void *buf, uint32_t len) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_write(&fs->instance, file, buf, len);
}

int32_t file_seek(lfs_file_t *file, int32_t offset, int whence) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_seek(&fs->instance, file, offset, whence);
}

int file_truncate(lfs_file_t *file, uint32_t size){
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_truncate(&fs->instance, file, size);
}

int32_t file_tell(lfs_file_t *file){
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_tell(&fs->instance, file);
}

int file_rewind(lfs_file_t *file) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_rewind(&fs->instance, file);
}

int32_t file_size(lfs_file_t *file){
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_file_size(&fs->instance, file);
}

int dir_mkdir(const char *path) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_mkdir(&fs->instance, path);
}

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

int dir_seek(lfs_dir_t *dir, uint32_t off) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_seek(&fs->instance, dir, off);
}

int32_t dir_tell(lfs_dir_t *dir) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_tell(&fs->instance, dir);
}

int dir_rewind(lfs_dir_t *dir) {
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    return lfs_dir_rewind(&fs->instance, dir);
}
