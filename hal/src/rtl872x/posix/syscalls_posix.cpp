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

// FIXME: this is a dirty hack in order for newlib headers to provide prototypes for us
#define _COMPILING_NEWLIB
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
// IMPORANT: this is our own implementation header
#include <sys/dirent.h>
#include <errno.h>
#include "filesystem.h"
#include "check.h"
#include "intrusive_list.h"
#include "scope_guard.h"
#include "rtc_hal.h"
#include <sys/reent.h>

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
            if (err > 0) {
                return 0;
            }
            return err;
    }
}

lfs_whence_flags posixWhenceToLfs(int whence) {
    switch (whence) {
        case SEEK_SET:
            return LFS_SEEK_SET;
        case SEEK_CUR:
            return LFS_SEEK_CUR;
        case SEEK_END:
            return LFS_SEEK_END;
        default:
            return (lfs_whence_flags)whence;
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
    FdEntry(const char* pathname, bool isFile) {
        if (isFile) {
            file = new lfs_file_t();
        } else {
            dir = new lfs_dir_t();
        }
        if (pathname) {
            name = strdup(pathname);
        }
    }
    ~FdEntry() {
        if (file) {
            delete file;
        }
        if (dir) {
            delete dir;
        }
        if (dent) {
            delete dent;
        }
        if (name) {
            delete name;
        }
    }
    int fd = -1;

    lfs_file_t* file = nullptr;
    lfs_dir_t* dir = nullptr;
    struct dirent* dent = nullptr;

    char* name = nullptr;

    FdEntry* next = nullptr;
};

class FdMap {
public:
    FdMap() = default;
    ~FdMap() = default;

    FdEntry* get(int fd, bool isFile = true) {
        CHECK_TRUE(fd >= MINIMAL_FD && fd <= MAXIMUM_FD, nullptr);

        for (auto entry = fds_.front(); entry != nullptr; entry = entry->next) {
            if (entry->fd == fd && (isFile ? entry->file != nullptr : entry->dir != nullptr)) {
                return entry;
            }
        }

        return nullptr;
    }

    FdEntry* create(const char* pathname, bool isFile = true) {
        FdEntry* entry = new FdEntry(pathname, isFile);
        CHECK_TRUE(entry, nullptr);

        if (entry->name && (entry->file || entry->dir)) {
            entry->fd = nextFd();
            if (entry->fd >= 0) {
                fds_.pushFront(entry);
                return entry;
            }
        }

        delete entry;
        return nullptr;
    }

    bool remove(int fd) {
        CHECK_TRUE(fd >= MINIMAL_FD && fd <= MAXIMUM_FD, false);

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
    static constexpr int MINIMAL_FD = STDERR_FILENO + 1;
    static constexpr int MAXIMUM_FD = HAL_PLATFORM_FILE_MAXIMUM_FD;

    particle::IntrusiveList<FdEntry> fds_;

    int nextFd() {
        for (int i = MINIMAL_FD; i <= MAXIMUM_FD; i++) {
            if (!get(i)) {
                return i;
            }
        }
        return -1;
    }
};

FdMap s_fdMap;

} // anonymous namespace

#define CHECK_LFS_ERRNO_VAL(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            errno = lfsErrorToErrno(_ret); \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _val; \
            } \
            _ret; \
        })

#define CHECK_LFS_ERRNO(_expr) CHECK_LFS_ERRNO_VAL(_expr, -1)

extern "C" {

int _open(const char* pathname, int flags, ... /* arg */) {
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.create(pathname);
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
    return entry->fd;
}

int _write(int fd, const void* buf, size_t count) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    return CHECK_LFS_ERRNO(lfs_file_write(&lfs->instance, entry->file, buf, count));
}

int _read(int fd, void* buf, size_t count) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    return CHECK_LFS_ERRNO(lfs_file_read(&lfs->instance, entry->file, buf, count));
}

int _fstat(int fd, struct stat* buf) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    return stat(entry->name, buf);
}

int _close(int fd) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
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

int _gettimeofday(struct timeval* tv, void* tz) {
    int r = hal_rtc_get_time(tv, nullptr);
    if (r) {
        errno = EFAULT;
        return -1;
    }
    // tz argument is obsolete
    (void)tz;
    return 0;
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
#ifdef HAVE_RENAME
    // Not implemented, LittleFS doesn't support symlinks
    errno = ENOSYS;
    return -1;
#else
    // Nano versions of newlib do not support _rename, instead it's
    // implemented as _link + _unlink.

    // As a workaround we'll temporarily store the oldpath into unused
    // entry in the reentrant struct and will ignore error in _unlink
    // if it maches.
    if (_rename(oldpath, newpath)) {
        return -1;
    }

    auto r = _REENT;

    if (r) {
        if (r->_signal_buf) {
            free(r->_signal_buf);
            r->_signal_buf = nullptr;
        }
        r->_signal_buf = strdup(oldpath);
    }

    return 0;
#endif // HAVE_RENAME
}

int _fsync(int fd) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    CHECK_LFS_ERRNO(lfs_file_sync(&lfs->instance, entry->file));

    return 0;
}

off_t _lseek(int fd, off_t offset, int whence) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    return CHECK_LFS_ERRNO(lfs_file_seek(&lfs->instance, entry->file, offset, posixWhenceToLfs(whence)));
}

int _mkdir(const char* pathname, mode_t mode) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    CHECK_LFS_ERRNO(lfs_mkdir(&lfs->instance, pathname));
    return 0;
}

int rmdir(const char* pathname) {
    return _unlink(pathname);
}

void* _sbrk(intptr_t increment) {
    // Not implemented
    errno = ENOSYS;
    return nullptr;
}

int stat(const char* pathname, struct stat* buf) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    struct lfs_info info = {};
    CHECK_LFS_ERRNO(lfs_stat(&lfs->instance, pathname, &info));

    if (buf) {
        buf->st_size = info.size;
        if (info.type == LFS_TYPE_REG) {
            buf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
        } else if (info.type == LFS_TYPE_DIR) {
            buf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
        }
    }

    return 0;
}

clock_t _times(struct tms* buf) {
    // Not implemented
    errno = ENOSYS;
    return (clock_t)-1;
}

int _unlink(const char* pathname) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    int ret = lfs_remove(&lfs->instance, pathname);
    // See explanation in _link()
    if (ret == LFS_ERR_NOENT) {
        auto r = _REENT;
        if (r && r->_signal_buf && !strcmp(pathname, r->_signal_buf)) {
            free(r->_signal_buf);
            r->_signal_buf = nullptr;
            errno = 0;
            return 0;
        }
    }
    return CHECK_LFS_ERRNO(ret);
}

pid_t _wait(int* status) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

DIR* _opendir(const char* name) {
    if (!name) {
        errno = EINVAL;
        return nullptr;
    }
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.create(name, false);
    if (!entry) {
        errno = ENOMEM;
        return nullptr;
    }

    NAMED_SCOPE_GUARD(g, {
        s_fdMap.remove(entry);
    });

    CHECK_LFS_ERRNO_VAL(lfs_dir_open(&lfs->instance, entry->dir, name), nullptr);
    g.dismiss();
    // XXX: simply cast int fd to DIR*
    return (DIR*)entry->fd;
}

struct dirent* _readdir(DIR* dirp) {
    struct dirent* result = nullptr;
    readdir_r(dirp, nullptr, &result);
    return result;
}

long _telldir(DIR* pdir) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get((int)pdir, false);
    if (!entry) {
        errno = EBADF;
        return -1;
    }


    return CHECK_LFS_ERRNO(lfs_dir_tell(&lfs->instance, entry->dir));
}

void _seekdir(DIR* pdir, long loc) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get((int)pdir, false);
    if (!entry) {
        errno = EBADF;
        return;
    }

    lfs_dir_seek(&lfs->instance, entry->dir, loc);
}

void _rewinddir(DIR* pdir) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get((int)pdir, false);
    if (!entry) {
        errno = EBADF;
        return;
    }

    lfs_dir_rewind(&lfs->instance, entry->dir);
}

int _readdir_r(DIR* pdir, struct dirent* dentry, struct dirent** out_dirent) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get((int)pdir, false);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    struct lfs_info info = {};
    int r = lfs_dir_read(&lfs->instance, entry->dir, &info);
    if (r <= 0) {
        if (out_dirent) {
            *out_dirent = nullptr;
        }
        errno = lfsErrorToErrno(r);
        return -1;
    }

    size_t dentrySize = offsetof(struct dirent, d_name) + strlen(info.name) + 1;

    if (!dentry) {
        entry->dent = static_cast<struct dirent*>(realloc(entry->dent, dentrySize));
        if (!entry->dent) {
            errno = ENOMEM;
            return -1;
        }
        dentry = entry->dent;
    }
    memset(dentry, 0, dentrySize);
    if (info.type == LFS_TYPE_REG) {
        dentry->d_type = DT_REG;
    } else if (info.type == LFS_TYPE_DIR) {
        dentry->d_type = DT_DIR;
    }
    dentry->d_reclen = dentrySize;
    strcpy(dentry->d_name, info.name);
    if (out_dirent) {
        *out_dirent = dentry;
    }
    return 0;
}

int _closedir(DIR* dirp) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get((int)dirp, false);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    SCOPE_GUARD({
        s_fdMap.remove(entry);
    });

    CHECK_LFS_ERRNO(lfs_dir_close(&lfs->instance, entry->dir));

    return 0;
}

int _chdir(const char* path) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

int _fchdir(int fd) {
    // Not implemented
    errno = ENOSYS;
    return -1;
}

char* getcwd(char* buf, size_t size) {
    if (!buf || size < 2) {
        errno = ERANGE;
        return nullptr;
    }
    // XXX: chdir() is not supported, so always return '/'
    buf[0] = '/';
    buf[1] = '\0';
    return buf;
}

int _rename(const char* oldpath, const char* newpath) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    CHECK_LFS_ERRNO(lfs_rename(&lfs->instance, oldpath, newpath));
    return 0;
}

int ftruncate(int fd, off_t length) {
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    auto entry = s_fdMap.get(fd);
    if (!entry) {
        errno = EBADF;
        return -1;
    }

    return CHECK_LFS_ERRNO(lfs_file_truncate(&lfs->instance, entry->file, length));
}

int truncate(const char* path, off_t length) {
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    auto lfs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    FsLock lk(lfs);

    lfs_file_t f = {};
    CHECK_LFS_ERRNO(lfs_file_open(&lfs->instance, &f, path, LFS_O_WRONLY));
    SCOPE_GUARD({
        lfs_file_close(&lfs->instance, &f);
    });
    CHECK_LFS_ERRNO(lfs_file_truncate(&lfs->instance, &f, length));
    return 0;
}

// Current newlib doesn't implement or handle these, so we manually alias _ to non-_
DIR* opendir(const char* name) __attribute__((alias("_opendir")));
struct dirent* readdir(DIR* pdir) __attribute__((alias("_readdir")));
long telldir(DIR* pdir) __attribute__((alias("_telldir")));;
void seekdir(DIR* pdir, long loc) __attribute__((alias("_seekdir")));
void rewinddir(DIR* pdir) __attribute__((alias("_rewinddir")));
int readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent) __attribute__((alias("_readdir_r")));
int closedir(DIR* pdir) __attribute__((alias("_closedir")));
int chdir(const char* path) __attribute__((alias("_chdir")));
int fchdir(int fd) __attribute__((alias("_fchdir")));
int mkdir(const char* pathname, mode_t mode) __attribute__((alias("_mkdir")));
int fsync(int fd) __attribute__((alias("_fsync")));

} // extern "C"
