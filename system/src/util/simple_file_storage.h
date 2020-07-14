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

#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_FILESYSTEM

#include "filesystem.h"

namespace particle::system {

class SimpleFileStorage {
public:
    explicit SimpleFileStorage(const char* file);
    ~SimpleFileStorage();

    int load(char* data, size_t size);
    int save(const char* data, size_t size);
    int sync();

    void close();

    void clear();

    static int load(const char* file, char* data, size_t size);
    static int save(const char* file, const char* data, size_t size);
    static void clear(const char* file);

private:
    lfs_file_t file_;
    const char* const fileName_;
    int openFlags_;

    int open(filesystem_t* fs, int flags);
    void close(filesystem_t* fs);
};

inline SimpleFileStorage::~SimpleFileStorage() {
    close();
}

inline int SimpleFileStorage::load(const char* file, char* data, size_t size) {
    return SimpleFileStorage(file).load(data, size);
}

inline int SimpleFileStorage::save(const char* file, const char* data, size_t size) {
    return SimpleFileStorage(file).save(data, size);
}

inline void SimpleFileStorage::clear(const char* file) {
    SimpleFileStorage(file).clear();
}

} // namespace particle::system

#endif // HAL_PLATFORM_FILESYSTEM
