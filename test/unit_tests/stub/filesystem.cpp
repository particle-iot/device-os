/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "filesystem.h"

filesystem_t* filesystem_get_instance(void* reserved) {
    static filesystem_t fs;
    return &fs;
}

int filesystem_lock(filesystem_t* fs) {
    return 0;
}

int filesystem_unlock(filesystem_t* fs) {
    return 0;
}

int lfs_file_open(lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
    return 0;
}

int lfs_file_close(lfs_t* lfs, lfs_file_t* file) {
    return 0;
}

lfs_ssize_t lfs_file_read(lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size) {
    return 0;
}

lfs_ssize_t lfs_file_write(lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size) {
    return 0;
}

lfs_soff_t lfs_file_size(lfs_t* lfs, lfs_file_t* file) {
    return 0;
}

lfs_soff_t lfs_file_seek(lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence) {
    return 0;
}

lfs_soff_t lfs_file_tell(lfs_t* lfs, lfs_file_t* file) {
    return 0;
}

int lfs_file_truncate(lfs_t* lfs, lfs_file_t* file, lfs_off_t size) {
    return 0;
}

int lfs_file_sync(lfs_t* lfs, lfs_file_t* file) {
    return 0;
}

int lfs_remove(lfs_t* lfs, const char* path) {
    return 0;
}
