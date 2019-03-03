/*
 * Copyright (c) 2019 Thorsten von Eicken.  All rights reserved.
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

#include "filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

// The following functions are straight adaptations from littlefs.
// The lfs_ prefix is dropped from the name (except for remove/rename/stat) and the first
// argument (lfs_t*) is omitted.
// All this is done for two reasons: this way we don't need to export the lfs_t filesystem to
// user-level and, mor importantly, it avoids name clashes at user-level because the littlefs
// library is linked into user-level (bad idea, seems like an expediency in order not to have to
// disentangle some dependencies in the platform stuff).

int file_remove(const char *path);
int file_rename(const char *oldpath, const char *newpath);
int file_stat(const char *path, struct lfs_info *info);
int file_open(lfs_file_t *file, const char* path, unsigned flags);
int file_close(lfs_file_t *file);
int file_sync(lfs_file_t *file);
int32_t file_read(lfs_file_t *file, void *buf, uint32_t len);
int32_t file_write(lfs_file_t *file, void *buf, uint32_t len);
int32_t file_seek(lfs_file_t *file, int32_t offset, int whence);
int file_truncate(lfs_file_t *file, uint32_t);
int32_t file_tell(lfs_file_t *file);
int file_rewind(lfs_file_t *file);
int32_t file_size(lfs_file_t *file);
int dir_mkdir(const char *path);
int dir_open(lfs_dir_t *dir, const char *path);
int dir_close(lfs_dir_t *dir);
int dir_read(lfs_dir_t *dir, struct lfs_info *info);
int dir_seek(lfs_dir_t *dir, uint32_t off);
int32_t dir_tell(lfs_dir_t *dir);
int dir_rewind(lfs_dir_t *dir);


#ifdef __cplusplus
} // extern "C"
#endif
