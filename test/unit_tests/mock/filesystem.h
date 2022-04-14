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

#include "stub/filesystem.h"

#include <hippomocks.h>

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace particle {

namespace test {

class Filesystem {
public:
    explicit Filesystem(MockRepository* mocks);
    ~Filesystem() noexcept(false);

    const std::string& readFile(const std::string& path);
    void writeFile(const std::string& path, const std::string& data);
    bool hasFile(const std::string& path);

    void createDir(const std::string& path);
    bool hasDir(const std::string& path);

    void remove(const std::string& path);

    void clear();

    bool hasOpenFiles() const;

    void autoCheckOpenFiles(bool enabled);
    bool autoCheckOpenFiles() const;

    int lastFileDesc() const;

private:
    enum EntryType {
        DIR,
        FILE
    };

    struct Entry {
        std::map<std::string, Entry> entries;
        std::unordered_set<int> fds;
        std::string name;
        std::string data;
        Entry* parent;
        EntryType type;

        Entry(std::string name, EntryType type, Entry* parent) :
                name(std::move(name)),
                parent(parent),
                type(type) {
        }
    };

    Entry root_;
    std::unordered_map<int, Entry*> fdMap_;
    MockRepository* mocks_;
    int lastFd_;
    bool checkOpenFiles_;

    Entry* findEntry(const std::string& path);
    Entry* createEntry(const std::string& path, EntryType type);
    void removeEntry(Entry* entry);

    int open(lfs_t* lfs, lfs_file_t* file, const char* path, int flags);
    int close(lfs_t* lfs, lfs_file_t* file);
    lfs_ssize_t read(lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size);
    lfs_ssize_t write(lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size);
    lfs_soff_t size(lfs_t* lfs, lfs_file_t* file);
    lfs_soff_t seek(lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence);
    lfs_soff_t tell(lfs_t* lfs, lfs_file_t* file);
    int truncate(lfs_t* lfs, lfs_file_t* file, lfs_off_t size);
    int sync(lfs_t* lfs, lfs_file_t* file);
    int remove(lfs_t* lfs, const char* path);
};

inline bool Filesystem::hasOpenFiles() const {
    return !fdMap_.empty();
}

inline void Filesystem::autoCheckOpenFiles(bool enabled) {
    checkOpenFiles_ = enabled;
}

inline bool Filesystem::autoCheckOpenFiles() const {
    return checkOpenFiles_;
}

inline int Filesystem::lastFileDesc() const {
    return lastFd_;
}

} // namespace test

} // namespace particle
