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
#ifndef HAL_DYNALIB_POSIX_SYSCALL_H
#define HAL_DYNALIB_POSIX_SYSCALL_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
// FIXME: this is a dirty hack in order for newlib headers to provide prototypes for us
#define _COMPILING_NEWLIB
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
// Some of the prototypes are not available even with _COMPILING_NEWLIB, so provide
// them manually here
int _write(int fd, const void* buf, size_t count);
int _read(int fd, void* buf, size_t count);
int _close(int fd);
int _isatty(int fd);
off_t _lseek(int fd, off_t offset, int whence);
int _unlink(const char* pathname);
int _link(const char* oldpath, const char* newpath);
int _rename(const char* oldpath, const char* newpath);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // DYNALIB_EXPORT

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_posix_syscall)
DYNALIB_FN(0, hal_posix_syscall, _open, int(const char* pathname, int flags, ... /* arg */))
DYNALIB_FN(1, hal_posix_syscall, _write, int(int fd, const void* buf, size_t count))
DYNALIB_FN(2, hal_posix_syscall, _read, int(int fd, void* buf, size_t count))
DYNALIB_FN(3, hal_posix_syscall, _close, int(int fd))
DYNALIB_FN(4, hal_posix_syscall, fsync, int(int fd))
DYNALIB_FN(5, hal_posix_syscall, _fstat, int(int fd, struct stat* buf))
DYNALIB_FN(6, hal_posix_syscall, _fcntl, int(int fd, int cmd, ... /* arg */))
DYNALIB_FN(7, hal_posix_syscall, _isatty, int(int fd))
DYNALIB_FN(8, hal_posix_syscall, _lseek, off_t(int fd, off_t offset, int whence))
DYNALIB_FN(9, hal_posix_syscall, stat, int(const char* pathname, struct stat* buf))
DYNALIB_FN(10, hal_posix_syscall, mkdir, int(const char* pathname, mode_t mode))
DYNALIB_FN(11, hal_posix_syscall, rmdir, int(const char* pathname))
DYNALIB_FN(12, hal_posix_syscall, _unlink, int(const char* pathname))
DYNALIB_FN(13, hal_posix_syscall, _link, int(const char* oldpath, const char* newpath))
DYNALIB_FN(14, hal_posix_syscall, _rename, int(const char* oldpath, const char* newpath))
DYNALIB_FN(15, hal_posix_syscall, opendir, DIR*(const char* name))
DYNALIB_FN(16, hal_posix_syscall, readdir, struct dirent*(DIR* pdir))
DYNALIB_FN(17, hal_posix_syscall, telldir, long(DIR* pdir))
DYNALIB_FN(18, hal_posix_syscall, seekdir, void(DIR* pdir, long loc))
DYNALIB_FN(19, hal_posix_syscall, rewinddir, void(DIR* pdir))
DYNALIB_FN(20, hal_posix_syscall, readdir_r, int(DIR* pdir, struct dirent* entry, struct dirent** out_dirent))
DYNALIB_FN(21, hal_posix_syscall, closedir, int(DIR* pdir))
DYNALIB_FN(22, hal_posix_syscall, chdir, int(const char* path))
DYNALIB_FN(23, hal_posix_syscall, fchdir, int(int fd))
DYNALIB_FN(24, hal_posix_syscall, getcwd, char*(char* buf, size_t size))
DYNALIB_FN(25, hal_posix_syscall, truncate, int(const char*, off_t))
DYNALIB_FN(26, hal_posix_syscall, ftruncate, int(int, off_t))
// DYNALIB_FN(27, hal_posix_syscall, _execve, int(const char* filename, char* const argv[], char* const envp[]))
// DYNALIB_FN(28, hal_posix_syscall, _fork, pid_t(void))
// DYNALIB_FN(29, hal_posix_syscall, _getpid, pid_t(void))
// DYNALIB_FN(30, hal_posix_syscall, _gettimeofday, int(struct timeval* tv, void* tz))
// DYNALIB_FN(31, hal_posix_syscall, _kill, int(pid_t pid, int sig))
// DYNALIB_FN(32, hal_posix_syscall, _sbrk, void*(intptr_t increment))
// DYNALIB_FN(33, hal_posix_syscall, _times, clock_t(struct tms* buf))
// DYNALIB_FN(34, hal_posix_syscall, _wait, pid_t(int* status))

DYNALIB_END(hal_posix_syscall)

#endif /* HAL_DYNALIB_POSIX_SYSCALL_H */
