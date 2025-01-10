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

#include "filesystem.h"

typedef struct pb_msgdesc_s pb_msgdesc_t;

namespace particle {

int openFile(lfs_file_t* file, const char* path, unsigned flags = LFS_O_RDWR);
int dumpFile(const char* path);

int decodeProtobufFromFile(lfs_file_t* file, const pb_msgdesc_t* desc, void* msg, int size = -1);
int encodeProtobufToFile(lfs_file_t* file, const pb_msgdesc_t* desc, const void* msg);

// TODO: Move these to filesystem.h
int rmrf(const char* path);
int mkdirp(const char* path);

/**
 * Remove the contents of a directory recursively without deleting the directory itself.
 *
 * @param path Directory path.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int clearDir(const char* path);

} // particle
