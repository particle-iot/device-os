/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
// IMPORANT: this is our own implementation header
#include <sys/dirent.h>
#include <errno.h>
#include "littlefs/filesystem.h"
#include "check.h"
#include "intrusive_list.h"
#include "scope_guard.h"

using namespace particle::fs;

namespace {

int lfsErrorToErrno(int err) {
    switch (err) {
        case LFS_ERR_OK:
            return 0;
        case LFS_ERR_IO:
            return EIO;
        case LFS_ERR_NOENT:
            return ENOENT;
        case LFS_ERR_EXIST:
            return EEXIST;
        case LFS_ERR_NOTDIR:
            return ENOTDIR;
        case LFS_ERR_ISDIR:
            return EISDIR;
        case LFS_ERR_INVAL:
            return EINVAL;
        case LFS_ERR_NOSPC:
            return ENOSPC;
        case LFS_ERR_NOMEM:
            return ENOMEM;
        case LFS_ERR_CORRUPT:
            return EILSEQ;
        default:
            return err;
    }
}

int posixOpenFlagsToLfs(int flags) {
    // Returns -1 if unknown flags are encountered

    int lfsFlags = 0;
    if ((flags & O_ACCMODE) == O_RDONLY) {
        lfsFlags |= LFS_O_RDONLY;
    } else if ((flags & O_ACCMODE) == O_WRONLY) {
        lfsFlags |= LFS_O_WRONLY;
    } else if ((flags & O_ACCMODE) == O_RDWR) {
        lfsFlags |= LFS_O_RDWR;
    }

    flags &= ~(O_ACCMODE);

    if (flags & O_CREAT) {
        flags &= ~(O_CREAT);
        lfsFlags |= LFS_O_CREAT;
    }
    if (flags & O_EXCL) {
        flags &= ~(O_EXCL);
        lfsFlags |= LFS_O_EXCL;
    }
    if (flags & O_TRUNC) {
        flags &= ~(O_TRUNC);
        lfsFlags |= LFS_O_TRUNC;
    }
    if (flags & O_APPEND) {
        flags &= ~(O_APPEND);
        lfsFlags |= LFS_O_APPEND;
    }

    // Unknown flags
    if (flags != 0) {
        return -1;
    }

    return lfsFlags;
}

struct FdEntry {
    int fd = -1;

    lfs_file_t* file;
    lfs_dir_t* dir;

    FdEntry* next = nullptr;
};

class FdMap {
public:
    FdMap() = default;
    ~FdMap() = default;

    FdEntry* get(int fd, bool isFile = true) {
        if (fd < MINIMAL_FD) {
            return nullptr;
        }

        for (auto entry = fds_.front(); entry != nullptr; entry = entry->next) {
            if (entry->fd == fd && (isFile ? entry->file != nullptr : entry->dir != nullptr)) {
                return entry;
            }
        }

        return nullptr;
    }

    FdEntry* create(bool isFile = true) {
        FdEntry* entry = new FdEntry();
        CHECK_TRUE(entry, nullptr);

        if (isFile) {
            entry->file = new lfs_file_t();
        } else {
            entry->dir = new lfs_dir_t();
        }
        if (entry->file || entry->dir) {
            entry->fd = nextFd();
            fds_.pushFront(entry);
            return entry;
        }

        delete entry;
        return nullptr;
    }

    bool remove(int fd) {
        if (fd < MINIMAL_FD) {
            return false;
        }

        for (auto entry = fds_.front(), prev = (FdEntry*)nullptr; entry != nullptr; prev = entry, entry = entry->next) {
            if (entry->fd == fd) {
                fds_.pop(entry, prev);
                delete entry;
                return true;
            }
        }

        return false;
    }

    bool remove(FdEntry* entry) {
        auto popped = fds_.pop(entry);
        if (popped) {
            delete popped;
            return true;
        }
        return false;
    }

private:
    static constexpr const int MINIMAL_FD = STDERR_FILENO + 1;

    particle::IntrusiveList<FdEntry> fds_;
    int nextFd_ = MINIMAL_FD;

    int nextFd() {
        while (get(nextFd_)) {
            ++nextFd_;
            if (nextFd_ < MINIMAL_FD) {
                nextFd_ = MINIMAL_FD;
            }
        }
        return nextFd_;
    }
};

FdMap s_fdMap;

} // anonymous namespace

#define CHECK_LFS_ERRNO(_expr) \
        ({ \
            const auto _ret = _expr; \
            errno = lfsErrorToErrno(_ret); \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return -1; \
            } \
            _ret; \
        })

extern "C" {

int _open(const char* pathname, int flags, ... /* arg */) {
    auto lfs = filesystem_get_instance(nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.create();
    if (!entry) {
        errno = ENOMEM;
        return -1;
    }

    NAMED_SCOPE_GUARD(g, {
        s_fdMap.remove(entry);
    });

    int lfsFlags = posixOpenFlagsToLfs(flags);
    if (lfsFlags < 0) {
        errno = EINVAL;
        return -1;
    }

    CHECK_LFS_ERRNO(lfs_file_open(&lfs->instance, entry->file, pathname, lfsFlags));
    g.dismiss();
    return 0;
}

int _write(int fd, const void* buf, size_t count) {
    auto lfs = filesystem_get_instance(nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    CHECK_LFS_ERRNO(lfs_file_write(&lfs->instance, entry->file, buf, count));
    return 0;
}

int _read(int fd, void* buf, size_t count) {
    auto lfs = filesystem_get_instance(nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    CHECK_LFS_ERRNO(lfs_file_read(&lfs->instance, entry->file, buf, count));
    return 0;
}

int _fstat(int fd, struct stat* buf) {
    return -1;
}

int _close(int fd) {
    auto lfs = filesystem_get_instance(nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    SCOPE_GUARD({
        s_fdMap.remove(entry);
    });

    CHECK_LFS_ERRNO(lfs_file_close(&lfs->instance, entry->file));

    return 0;
}

int _execve(const char* filename, char* const argv[], char* const envp[]) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

int _fcntl(int fd, int cmd, ... /* arg */) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

pid_t _fork(void) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

pid_t _getpid(void) {
    // Always return PID = 1
    return 1;
}

int _gettimeofday(struct timeval* tv, struct timezone* tz) {
    return -1;
}

int _isatty(int fd) {
    // We won't have any file descriptors referring to a terminal
    errno = ENOTTY;
    return 0;
}

int _kill(pid_t pid, int sig) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

int _link(const char* oldpath, const char* newpath) {
    return -1;
}

int _lseek(int fd, off_t offset, int whence) {
    return -1;
}

int _mkdir(const char* pathname, mode_t mode) {
    return -1;
}

void* _sbrk(intptr_t increment) {
    // Not implemented
    errno = ENOSYS;
    return nullptr;
}

int _stat(const char* pathname, struct stat* buf) {
    return -1;
}

clock_t _times(struct tms* buf) {
    return (clock_t)-1;
}

int _unlink(const char* pathname) {
    return -1;
}

pid_t _wait(int* status) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

DIR* _opendir(const char* name) {
    return nullptr;
}

struct dirent* _readdir(DIR* dirp) {
    return nullptr;
}

long _telldir(DIR* pdir) {
    return -1;
}

void _seekdir(DIR* pdir, long loc) {
}

void _rewinddir(DIR* pdir) {
}

int _readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent) {
    return -1;
}

int _closedir(DIR* dirp) {
    return -1;
}

// Current newlib doesn't implement or handle these, so we manually alias _ to non-_
DIR* opendir(const char* name) __attribute__((alias("_opendir")));
struct dirent* readdir(DIR* pdir) __attribute__((alias("_readdir")));
long telldir(DIR* pdir) __attribute__((alias("_telldir")));;
void seekdir(DIR* pdir, long loc) __attribute__((alias("_seekdir")));
void rewinddir(DIR* pdir) __attribute__((alias("_rewinddir")));
int readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent) __attribute__((alias("_readdir_r")));
int closedir(DIR* pdir) __attribute__((alias("_closedir")));

} // extern "C"
