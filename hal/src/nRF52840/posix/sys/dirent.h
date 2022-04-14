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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/syslimits.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {

} DIR;

struct dirent {
    /* The only fields in the dirent structure that are mandated by POSIX.1
     * are d_name and d_ino.  The other fields are unstandardized, and not
     * present on all systems.
     */
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[NAME_MAX + 1];
};

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

DIR* opendir(const char* name);

struct dirent* readdir(DIR* pdir);

long telldir(DIR* pdir);

void seekdir(DIR* pdir, long loc);

void rewinddir(DIR* pdir);

int readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent);

int closedir(DIR* pdir);

#ifdef __cplusplus
}
#endif // __cplusplus