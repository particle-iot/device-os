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

#include <boost/algorithm/string/split.hpp>

#include <algorithm>
#include <vector>

namespace particle {

namespace test {

namespace {

class FileError: public std::runtime_error {
public:
    explicit FileError(std::string msg, lfs_error code) :
            std::runtime_error(std::move(msg)),
            code_(code) {
    }

    lfs_error code() const {
        return code_;
    }

private:
    lfs_error code_;
};

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    boost::algorithm::split(parts, path, [](char c) { return c == '/'; });
    parts.erase(std::remove_if(parts.begin(), parts.end(), [](const std::string& p) { return p.empty(); }), parts.end());
    return parts;
}

} // namespace

Filesystem::Filesystem(MockRepository* mocks) :
        root_(std::string(), EntryType::DIR, nullptr),
        mocks_(mocks),
        lastFd_(0),
        checkOpenFiles_(true) {
    mocks_->OnCallFunc(lfs_file_open).Do([this](lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
        return this->open(lfs, file, path, flags);
    });
    mocks_->OnCallFunc(lfs_file_close).Do([this](lfs_t* lfs, lfs_file_t* file) {
        return this->close(lfs, file);
    });
    mocks_->OnCallFunc(lfs_file_read).Do([this](lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size) {
        return this->read(lfs, file, buf, size);
    });
    mocks_->OnCallFunc(lfs_file_write).Do([this](lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size) {
        return this->write(lfs, file, buf, size);
    });
    mocks_->OnCallFunc(lfs_file_size).Do([this](lfs_t* lfs, lfs_file_t* file) {
        return this->size(lfs, file);
    });
    mocks_->OnCallFunc(lfs_file_seek).Do([this](lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence) {
        return this->seek(lfs, file, offs, whence);
    });
    mocks_->OnCallFunc(lfs_file_tell).Do([this](lfs_t* lfs, lfs_file_t* file) {
        return this->tell(lfs, file);
    });
    mocks_->OnCallFunc(lfs_file_truncate).Do([this](lfs_t* lfs, lfs_file_t* file, lfs_off_t size) {
        return this->truncate(lfs, file, size);
    });
    mocks_->OnCallFunc(lfs_file_sync).Do([this](lfs_t* lfs, lfs_file_t* file) {
        return this->sync(lfs, file);
    });
    mocks_->OnCallFunc(lfs_remove).Do([this](lfs_t* lfs, const char* path) {
        return this->remove(lfs, path);
    });
}

Filesystem::~Filesystem() noexcept(false) {
    if (checkOpenFiles_ && hasOpenFiles()) {
        throw std::runtime_error("Some of the files have not been closed");
    }
}

const std::string& Filesystem::readFile(const std::string& path) {
    const auto e = findEntry(path);
    if (!e) {
        throw FileError("File not found: " + path, LFS_ERR_NOENT);
    }
    if (e->type != EntryType::FILE) {
        throw FileError("Path is not a file: " + path, LFS_ERR_ISDIR);
    }
    return e->data;
}

void Filesystem::writeFile(const std::string& path, const std::string& data) {
    auto e = findEntry(path);
    if (!e) {
        e = createEntry(path, EntryType::FILE);
    } else if (e->type != EntryType::FILE) {
        throw FileError("Path is not a file: " + path, LFS_ERR_ISDIR);
    }
    e->data = data;
}

bool Filesystem::hasFile(const std::string& path) {
    const auto e = findEntry(path);
    if (!e || e->type != EntryType::FILE) {
        return false;
    }
    return true;
}

void Filesystem::createDir(const std::string& path) {
    const auto e = findEntry(path);
    if (!e) {
        createEntry(path, EntryType::DIR);
    } else if (e->type != EntryType::DIR) {
        throw FileError("Path is not a directory: " + path, LFS_ERR_NOTDIR);
    }
}

bool Filesystem::hasDir(const std::string& path) {
    const auto e = findEntry(path);
    if (!e || e->type != EntryType::DIR) {
        return false;
    }
    return true;
}

void Filesystem::remove(const std::string& path) {
    const auto e = findEntry(path);
    if (e) {
        removeEntry(e);
    }
}

void Filesystem::clear() {
    root_.entries.clear();
    fdMap_.clear();
    lastFd_ = 0;
}

Filesystem::Entry* Filesystem::findEntry(const std::string& path) {
    const auto parts = splitPath(path);
    if (parts.empty()) {
        return nullptr;
    }
    auto e = &root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        const auto it = e->entries.find(parts.at(i));
        if (it == e->entries.end()) {
            return nullptr;
        }
        e = &it->second;
        if (e->type != EntryType::DIR) {
            return nullptr;
        }
    }
    const auto it = e->entries.find(parts.back());
    if (it == e->entries.end()) {
        return nullptr;
    }
    return &it->second;
}

Filesystem::Entry* Filesystem::createEntry(const std::string& path, EntryType type) {
    const auto parts = splitPath(path);
    if (parts.empty()) {
        throw FileError("Invalid path: " + path, LFS_ERR_NOTDIR);
    }
    auto e = &root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        const auto name = parts.at(i);
        const auto it = e->entries.find(name);
        if (it == e->entries.end()) {
            const auto r = e->entries.insert(std::make_pair(name, Entry(name, EntryType::DIR, e)));
            e = &r.first->second;
        } else {
            e = &it->second;
            if (e->type != EntryType::DIR) {
                throw FileError("Invalid path: " + path, LFS_ERR_NOTDIR);
            }
        }
    }
    const auto name = parts.back();
    const auto r = e->entries.insert(std::make_pair(name, Entry(name, type, e)));
    if (!r.second) {
        throw FileError("Path already exists: " + path, LFS_ERR_EXIST);
    }
    return &r.first->second;
}

void Filesystem::removeEntry(Entry* entry) {
    if (entry->type == EntryType::FILE) {
        if (!entry->fds.empty()) {
            throw std::runtime_error("Detected an attempt to remove an open file");
        }
    } else {
        auto it = entry->entries.begin();
        while (it != entry->entries.end()) {
            const auto e = &(it++)->second;
            removeEntry(e);
        }
    }
    if (entry->parent) {
        entry->parent->entries.erase(entry->name);
    }
}

int Filesystem::open(lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file || !path) {
            throw std::runtime_error("lfs_file_open() has been called with invalid arguments");
        }
        auto e = findEntry(path);
        if (!e) {
            if (!(flags & LFS_O_CREAT)) {
                return LFS_ERR_NOENT;
            }
            e = createEntry(path, EntryType::FILE);
        } else if (flags & LFS_O_EXCL) {
            return LFS_ERR_EXIST;
        } else if (e->type != EntryType::FILE) {
            return LFS_ERR_ISDIR;
        }
        if (flags & LFS_O_TRUNC) {
            if (!(flags & LFS_O_WRONLY)) {
                // LittleFS doesn't seem to handle this
                throw std::runtime_error("O_TRUNC flag is used without O_RDWR or O_WRONLY");
            }
            e->data = std::string();
        }
        file->fd = ++lastFd_;
        file->flags = flags;
        file->pos = 0;
        e->fds.insert(file->fd);
        fdMap_.insert(std::make_pair(file->fd, e));
        return LFS_ERR_OK;
    } catch (const FileError& e) {
        return e.code();
    }
}

int Filesystem::close(lfs_t* lfs, lfs_file_t* file) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_close() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            // Not an UB according to POSIX but LittleFS doesn't seem to handle access to closed files
            throw std::runtime_error("lfs_file_close() has been called for an already closed file");
        }
        const auto e = it->second;
        e->fds.erase(file->fd);
        fdMap_.erase(it);
        file->fd = 0;
        return LFS_ERR_OK;
    } catch (const FileError& e) {
        return e.code();
    }
}

lfs_ssize_t Filesystem::read(lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file || (size > 0 && !buf)) {
            throw std::runtime_error("lfs_file_read() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_read() has been called for a closed file");
        }
        if (!(file->flags & LFS_O_RDONLY)) {
            return LFS_ERR_BADF;
        }
        const auto e = it->second;
        auto d = e->data;
        auto pos = file->pos;
        if (pos > d.size()) {
            pos = d.size();
        }
        if (pos + size > d.size()) {
            size = d.size() - pos;
        }
        memcpy(buf, d.data() + pos, size);
        file->pos = pos + size;
        return size;
    } catch (const FileError& e) {
        return e.code();
    }
}

lfs_ssize_t Filesystem::write(lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file || (size > 0 && !buf)) {
            throw std::runtime_error("lfs_file_write() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_write() has been called for a closed file");
        }
        if (!(file->flags & LFS_O_WRONLY)) {
            return LFS_ERR_BADF;
        }
        const auto e = it->second;
        auto pos = file->pos;
        if (pos > e->data.size()) {
            e->data += std::string(pos - e->data.size(), '\0');
            pos = e->data.size();
        } else if (pos < e->data.size() && (file->flags & LFS_O_APPEND)) {
            pos = e->data.size();
        }
        e->data.replace(pos, std::min<size_t>(size, e->data.size() - pos), std::string((const char*)buf, size));
        file->pos = pos + size;
        return size;
    } catch (const FileError& e) {
        return e.code();
    }
}

lfs_soff_t Filesystem::size(lfs_t* lfs, lfs_file_t* file) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_size() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_size() has been called for a closed file");
        }
        const auto e = it->second;
        return e->data.size();
    } catch (const FileError& e) {
        return e.code();
    }
}

lfs_soff_t Filesystem::seek(lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_seek() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_seek() has been called for a closed file");
        }
        const auto e = it->second;
        lfs_soff_t pos = 0;
        switch (whence) {
        case LFS_SEEK_SET:
            pos = offs;
            break;
        case LFS_SEEK_CUR:
            pos = file->pos + offs;
            break;
        case LFS_SEEK_END:
            pos = e->data.size() + offs;
            break;
        default:
            throw std::runtime_error("lfs_file_seek() has been called with an invalid whence argument");
        };
        if (pos < 0) {
            return LFS_ERR_INVAL;
        }
        file->pos = pos;
        return pos;
    } catch (const FileError& e) {
        return e.code();
    }
}

lfs_soff_t Filesystem::tell(lfs_t* lfs, lfs_file_t* file) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_tell() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_tell() has been called for a closed file");
        }
        return file->pos;
    } catch (const FileError& e) {
        return e.code();
    }
}

int Filesystem::truncate(lfs_t* lfs, lfs_file_t* file, lfs_off_t size) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_truncate() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_truncate() has been called for a closed file");
        }
        const auto e = it->second;
        if (size > e->data.size()) {
            e->data += std::string(size - e->data.size(), '\0');
        } else {
            e->data.erase(size, e->data.size() - size);
        }
        return 0;
    } catch (const FileError& e) {
        return e.code();
    }
}

int Filesystem::sync(lfs_t* lfs, lfs_file_t* file) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !file) {
            throw std::runtime_error("lfs_file_sync() has been called with invalid arguments");
        }
        const auto it = fdMap_.find(file->fd);
        if (it == fdMap_.end()) {
            throw std::runtime_error("lfs_file_sync() has been called for a closed file");
        }
        return 0;
    } catch (const FileError& e) {
        return e.code();
    }
}

int Filesystem::remove(lfs_t* lfs, const char* path) {
    try {
        if (!lfs || lfs != &filesystem_get_instance(nullptr)->instance || !path) {
            throw std::runtime_error("lfs_file_remove() has been called with invalid arguments");
        }
        const auto e = findEntry(path);
        if (!e) {
            return LFS_ERR_NOENT;
        }
        if (e->type == EntryType::DIR && !e->entries.empty()) {
            return LFS_ERR_NOTEMPTY;
        }
        removeEntry(e);
        return 0;
    } catch (const FileError& e) {
        return e.code();
    }
}

} // namespace test

} // namespace particle
