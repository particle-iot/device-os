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

#include "application.h"
#include "unit-test/unit-test.h"
#include <dirent.h>
#include <cstdio>
#include <sys/stat.h>
#include "check.h"
#include "c_string.h"
#include <limits.h>
#include <algorithm>
#include <fcntl.h>
#include "random.h"

#if HAL_PLATFORM_FILESYSTEM

namespace {

using namespace particle;
using namespace spark;

#define USR_DIR "/usr"
#define TEST_DIR USR_DIR "/wiring_filesystem"

constexpr mode_t DEFAULT_MODE = 0755;

bool dirExists(const char* path) {
    struct stat st;
    if (!stat(path, &st)) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
    }

    return false;
}

bool fileExists(const char* path) {
    struct stat st;
    if (!stat(path, &st)) {
        if (S_ISREG(st.st_mode)) {
            return true;
        }
    }

    return false;
}

struct FsEntryHelper {
    explicit FsEntryHelper(const char* n, int p = -1)
            : FsEntryHelper(n, dirExists(n), p) {
    }
    explicit FsEntryHelper(const char* n, bool d, int p = -1)
            : parent{p},
              name{n},
              dir{d} {
    }
    ~FsEntryHelper() = default;

    CString fullPath(Vector<FsEntryHelper>& vec) const {
        // Make pass over all the parents to find the length required
        size_t len = 1;
        for (auto el = this; el != nullptr; el = el->parent >= 0 ? &vec[el->parent] : nullptr) {
            len += strlen(el->name) + 1;
        }

        auto path = std::make_unique<char[]>(len);
        if (path) {
            memset(path.get(), 0, len);
            size_t pos = 0;
            for (auto el = this; el != nullptr && pos != len; el = el->parent >= 0 ? &vec[el->parent] : nullptr) {
                if (el != this && (len - pos >= 1) && strcmp(el->name, "/")) {
                    path.get()[pos++] = '/';
                }
                size_t toCopy = strlen(el->name);
                if (toCopy >= (len - pos)) {
                    break;
                }
                strncpy(path.get() + pos, el->name, toCopy);
                std::reverse(path.get() + pos, path.get() + pos + toCopy);
                pos += toCopy;
            }

            std::reverse(path.get(), path.get() + strlen(path.get()));
            auto r = CString::wrap(path.get());
            path.release();
            return r;
        }

        return CString();
    }

    bool isDir() const {
        return dir;
    }

    int parent;
    CString name;
    bool dir;
};

// FIXME: use list
int rmDir(const char* path) {
    if (!dirExists(path)) {
        return -1;
    }

    spark::Vector<FsEntryHelper> unlinkList;
    unlinkList.append(FsEntryHelper(path));

    int index = 0;

    for (; index < unlinkList.size(); index++) {
        auto& curDir = unlinkList[index];
        if (!curDir.dir) {
            continue;
        }
        auto curPath = curDir.fullPath(unlinkList);
        auto d = opendir(curPath);
        if (!d) {
            return -1;
        }

        struct dirent* ent = nullptr;
        while ((ent = readdir(d)) != nullptr) {
            // Just in case skip "." and ".."
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                continue;
            }

            unlinkList.append(FsEntryHelper(ent->d_name, ent->d_type == DT_DIR, index));
        }

        closedir(d);
    }

    for (int i = unlinkList.size() - 1; i >= 0; i--) {
        auto& ent = unlinkList[i];
        auto fullPath = ent.fullPath(unlinkList);
        if (!ent.dir) {
            unlink(fullPath);
        } else {
            rmdir(fullPath);
        }
    }
    return 0;
}

void listFsContents(const char* path = "/") {
    // List everything
    spark::Vector<FsEntryHelper> dirs;
    dirs.append(FsEntryHelper(path, true));

    for (int i = 0; i < dirs.size(); i++) {
        auto& curDir = dirs[i];
        auto curPath = curDir.fullPath(dirs);
        auto d = opendir(curPath);
        if (d == nullptr) {
            return;
        }

        Serial.println();
        Serial.printlnf("%s:", (const char*)curPath);

        struct dirent* ent = nullptr;
        while ((ent = readdir(d)) != nullptr) {
            bool special = false;
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                special = true;
            }

            FsEntryHelper entH(ent->d_name, true, i);

            struct stat st;
            if (stat(entH.fullPath(dirs), &st)) {
                return;
            }

            Serial.printlnf("%crw-rw-rw- %8lu %s", ent->d_type != DT_DIR ? '-' : 'd', st.st_size, ent->d_name);

            if (ent->d_type == DT_DIR && !special) {
                dirs.append(entH);
            }
        }

        closedir(d);
    }

    Serial.println();
}

Vector<CString> generateRandomFilenames(const char* basePath, int num) {
    Vector<CString> files(num);
    particle::Random rand;
    for (int i = 0; i < num; i++) {
        char tmp[NAME_MAX + 1] = {};
        snprintf(tmp, sizeof(tmp), "%s/", basePath);
        size_t l = random(NAME_MAX / 2, NAME_MAX - strlen(tmp));
        rand.genBase32(tmp + strlen(tmp), l);
        files[i] = CString(tmp);
    }

    return files;
}

} // anonymous

test(FS_POSIX_00_Prepare) {
    if (!dirExists(TEST_DIR)) {
        assertEqual(0, mkdir(TEST_DIR, DEFAULT_MODE));
    }

    listFsContents();
}

test(FS_POSIX_01_Directories) {
    constexpr int num = 10;
    
    // Create 'num' directories
    auto dirs = generateRandomFilenames(TEST_DIR, num);

    for (int i = 0; i < num; i++) {
        assertEqual(0, mkdir(dirs[i], DEFAULT_MODE));
    }

    for (int i = 0; i < num; i++) {
        assertTrue(dirExists(dirs[i]));
    }

    auto dirMove = generateRandomFilenames(TEST_DIR, num);
    for (int i = 0; i < num; i++) {
        assertEqual(0, rename(dirs[i], dirMove[i]));
        assertEqual(0, errno);
        assertTrue(dirExists(dirMove[i]));
        assertFalse(dirExists(dirs[i]));
    }

    listFsContents();

    for (int i = 0; i < num; i++) {
        assertEqual(0, rmdir(dirMove[i]));
    }
}

test(FS_POSIX_02_File) {
    constexpr int num = 10;
    constexpr size_t dataSize = 8192;

    auto files = generateRandomFilenames(TEST_DIR, num);
    static char randomData[dataSize] = {};
    static char tmpData[dataSize] = {};
    particle::Random rand;
    rand.genBase32(randomData, sizeof(randomData) - 1);
    
    spark::Vector<int> fds(num);

    // Create and write only
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_CREAT | O_WRONLY);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        ssize_t r = write(fd, randomData, sizeof(randomData));
        assertEqual(r, sizeof(randomData));
    }

    // Verify that read fails
    for (int i = 0; i < num; i++) {
        int r = read(fds[i], tmpData, sizeof(tmpData));
        assertEqual(r, -1);
    }

    // Close
    for (int i = 0; i < num; i++) {
        assertEqual(close(fds[i]), 0);
    }

    // Validate file size with stat
    for (int i = 0; i < num; i++) {
        struct stat st;
        assertEqual(0, stat(files[i], &st));
        assertEqual(st.st_size, sizeof(randomData));
    }

    // Open in read only and validate
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_RDONLY);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        memset(tmpData, 0, sizeof(tmpData));
        ssize_t r = read(fd, tmpData, sizeof(tmpData));
        assertEqual(r, sizeof(tmpData));

        assertEqual(0, strncmp(tmpData, randomData, sizeof(tmpData)));
    }

    // Verify that write fails
    for (int i = 0; i < num; i++) {
        int r = write(fds[i], randomData, sizeof(randomData));
        assertEqual(r, -1);
    }

    // Close
    for (int i = 0; i < num; i++) {
        assertEqual(close(fds[i]), 0);
    }

    rand.genBase32(randomData, sizeof(randomData) - 1);
    // Truncate, overwrite, seek, read
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_CREAT | O_RDWR | O_TRUNC);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        ssize_t r = write(fd, randomData, sizeof(randomData));
        assertEqual(r, sizeof(randomData));

        assertEqual(sizeof(randomData), lseek(fd, 0, SEEK_CUR));
        assertEqual(0, lseek(fd, -sizeof(randomData), SEEK_CUR));

        memset(tmpData, 0, sizeof(tmpData));
        r = read(fd, tmpData, sizeof(tmpData));
        assertEqual(r, sizeof(tmpData));

        assertEqual(0, strncmp(tmpData, randomData, sizeof(tmpData)));
    }

    // Close and validate file size with stat
    for (int i = 0; i < num; i++) {
        assertEqual(close(fds[i]), 0);
        struct stat st;
        assertEqual(0, stat(files[i], &st));
        assertEqual(st.st_size, sizeof(randomData));
    }

    rand.genBase32(randomData + sizeof(randomData) / 2, sizeof(randomData) / 2 - 1);
    // Open, seek, overwrite, read
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_CREAT | O_RDWR);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        assertEqual(sizeof(randomData) / 2, lseek(fd, sizeof(randomData) / 2, SEEK_CUR));

        ssize_t r = write(fd, randomData + sizeof(randomData) / 2, sizeof(randomData) / 2);
        assertEqual(r, sizeof(randomData) / 2);

        assertEqual(sizeof(randomData), lseek(fd, 0, SEEK_CUR));
        assertEqual(0, lseek(fd, -sizeof(randomData), SEEK_CUR));

        memset(tmpData, 0, sizeof(tmpData));
        r = read(fd, tmpData, sizeof(tmpData));
        assertEqual(r, sizeof(tmpData));

        assertEqual(0, strncmp(tmpData, randomData, sizeof(tmpData)));

        close(fd);
    }

    rand.genBase32(randomData, sizeof(randomData) - 1);

    // Open with truncate, write, close
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_CREAT | O_RDWR | O_TRUNC);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        ssize_t r = write(fd, randomData, sizeof(randomData) / 2);
        assertEqual(r, sizeof(randomData) / 2);

        close(fd);
    }

    // Open with append, write, seek, read, validate
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_APPEND | O_RDWR);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        assertEqual(sizeof(randomData) / 2, lseek(fd, 0, SEEK_END));

        ssize_t r = write(fd, randomData + sizeof(randomData) / 2, sizeof(randomData) / 2);
        assertEqual(r, sizeof(randomData) / 2);

        assertEqual(0, lseek(fd, 0, SEEK_SET));

        memset(tmpData, 0, sizeof(tmpData));
        r = read(fd, tmpData, sizeof(tmpData));
        assertEqual(r, sizeof(tmpData));

        assertEqual(0, strncmp(tmpData, randomData, sizeof(tmpData)));

        close(fd);
    }

    auto fileMove = generateRandomFilenames(TEST_DIR, num);
    for (int i = 0; i < num; i++) {
        assertEqual(0, rename(files[i], fileMove[i]));
        assertEqual(0, errno);
        assertTrue(fileExists(fileMove[i]));
        assertFalse(fileExists(files[i]));
    }
}

test(FS_POSIX_03_Truncate) {
    constexpr int num = 10;
    constexpr size_t dataSize = 8192;

    auto files = generateRandomFilenames(TEST_DIR, num);

    static char randomData[dataSize] = {};
    particle::Random rand;
    rand.genBase32(randomData, sizeof(randomData) - 1);

    spark::Vector<int> fds(num);

    // Create and write only
    for (int i = 0; i < num; i++) {
        int fd = open(files[i], O_CREAT | O_WRONLY);
        assertMoreOrEqual(fd, 0);
        fds[i] = fd;

        ssize_t r = write(fd, randomData, sizeof(randomData));
        assertEqual(r, sizeof(randomData));
    }

    // Sync
    for (int i = 0; i < num; i++) {
        assertEqual(fsync(fds[i]), 0);
    }

    // Validate file size with stat
    for (int i = 0; i < num; i++) {
        struct stat st;
        assertEqual(0, stat(files[i], &st));
        assertEqual(st.st_size, sizeof(randomData));
    }

    // ftruncate
    for (int i = 0; i < num; i++) {
        assertEqual(ftruncate(fds[i], dataSize / 2), 0);
    }

    // Close
    for (int i = 0; i < num; i++) {
        assertEqual(close(fds[i]), 0);
    }

    // Validate file size with stat
    for (int i = 0; i < num; i++) {
        struct stat st;
        assertEqual(0, stat(files[i], &st));
        assertEqual(st.st_size, dataSize / 2);
    }

    // truncate
    for (int i = 0; i < num; i++) {
        truncate(files[i], 0);
    }

    // Validate file size with stat
    for (int i = 0; i < num; i++) {
        struct stat st;
        assertEqual(0, stat(files[i], &st));
        assertEqual(st.st_size, 0);
    }

    // Unlink
    for (int i = 0; i < num; i++) {
        assertEqual(0, unlink(files[i]));
        struct stat st;
        assertNotEqual(0, stat(files[i], &st));
    }

    // truncate
    for (int i = 0; i < num; i++) {
        assertNotEqual(0, truncate(files[i], 0));
    }
}

test(FS_POSIX_03_TruncateStress) {
    // https://github.com/littlefs-project/littlefs/issues/268
    const int num = 10;
    const size_t dataSize = 8192;
    const size_t truncateSize = 8192 / 32;

    auto files = generateRandomFilenames(TEST_DIR, num);

    static char randomData[dataSize] = {};
    particle::Random rand;
    rand.gen(randomData, sizeof(randomData));

    spark::Vector<int> fds(num);

    const int iterations = 20;

    for (int iter = 0; iter < iterations; iter++) {
        // Serial.printlnf("iter=%d", iter);
        // Create and write
        for (int i = 0; i < num; i++) {
            int fd = open(files[i], O_CREAT | O_RDWR | O_APPEND);
            assertMoreOrEqual(fd, 0);
            fds[i] = fd;

            ssize_t r = write(fd, randomData, sizeof(randomData));
            assertEqual(r, sizeof(randomData));
        }

        // Sync
        for (int i = 0; i < num; i++) {
            assertEqual(fsync(fds[i]), 0);
        }

        // Validate file size with stat
        for (int i = 0; i < num; i++) {
            struct stat st;
            assertEqual(0, stat(files[i], &st));
            assertEqual(st.st_size, sizeof(randomData));
        }

        ssize_t expectedSize = sizeof(randomData);
        while (expectedSize > 0) {
            for (int i = 0; i < num; i++) {
                // Validate file size with seek
                assertEqual(expectedSize, lseek(fds[i], 0, SEEK_END));
                // Validate data
                char chunk[1024] = {};
                assertEqual(0, lseek(fds[i], 0, SEEK_SET));
                for (ssize_t pos = 0; pos < expectedSize; pos += sizeof(chunk)) {
                    size_t toRead = std::min<size_t>(expectedSize - pos, sizeof(chunk));
                    // Serial.printlnf("[%d] pos=%u/%u toRead=%u", i, pos, expectedSize, toRead);
                    assertEqual(toRead, read(fds[i], chunk, toRead));
                    assertEqual(0, memcmp(chunk, randomData + pos, toRead));
                }
            }

            expectedSize -= truncateSize;
            if (expectedSize >= 0) {
                // ftruncate
                for (int i = 0; i < num; i++) {
                    // Serial.printlnf("[%d] ftruncate %d", i, expectedSize);
                    assertEqual(ftruncate(fds[i], expectedSize), 0);
                }
            }
        }

        // Close
        for (int i = 0; i < num; i++) {
            assertEqual(close(fds[i]), 0);
        }

        // Validate file size with stat
        for (int i = 0; i < num; i++) {
            struct stat st;
            assertEqual(0, stat(files[i], &st));
            assertEqual(st.st_size, 0);
        }

        // Unlink
        for (int i = 0; i < num; i++) {
            assertEqual(0, unlink(files[i]));
            struct stat st;
            assertNotEqual(0, stat(files[i], &st));
        }

        // truncate
        for (int i = 0; i < num; i++) {
            assertNotEqual(0, truncate(files[i], 0));
        }
    }
}


test(FS_POSIX_99_Cleanup) {
    assertTrue(dirExists(TEST_DIR));
    assertEqual(0, rmDir(TEST_DIR));
}

#endif // HAL_PLATFORM_FILESYSTEM
