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

namespace particle {

// Note: It is not safe to access the same file using multiple instances of this class
class SimpleFileStorage {
public:
    explicit SimpleFileStorage(const char* fileName);
    ~SimpleFileStorage();

    int load(void* data, size_t size);
    int save(const void* data, size_t size);
    int sync();

    void clear();

    void close();

    const char* fileName() const;

    static int load(const char* file, void* data, size_t size);
    static int save(const char* file, const void* data, size_t size);
    static void clear(const char* file);

private:
    lfs_file_t file_;
    const char* fileName_;
    int dataSize_;
    int openFlags_;

    int readDataSize(filesystem_t* fs);
    int open(filesystem_t* fs, bool readOnly);
    void close(filesystem_t* fs);
};

inline SimpleFileStorage::~SimpleFileStorage() {
    close();
}

inline const char* SimpleFileStorage::fileName() const {
    return fileName_;
}

inline int SimpleFileStorage::load(const char* file, void* data, size_t size) {
    return SimpleFileStorage(file).load(data, size);
}

inline int SimpleFileStorage::save(const char* file, const void* data, size_t size) {
    return SimpleFileStorage(file).save(data, size);
}

inline void SimpleFileStorage::clear(const char* file) {
    SimpleFileStorage(file).clear();
}

} // namespace particle

#endif // HAL_PLATFORM_FILESYSTEM
