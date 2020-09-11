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

#include "simple_file_storage.h"
#include "system_error.h"

#include "mock/filesystem.h"
#include "util/random.h"

#include <catch2/catch.hpp>
#include <hippomocks.h>

#include <string>
#include <unordered_map>

using namespace particle;

TEST_CASE("SimpleFileStorage") {
    MockRepository mocks;
    SECTION("SimpleFileStorage()") {
        SECTION("constructs a file storage instance") {
            SimpleFileStorage s("path/to/file");
            CHECK(strcmp(s.fileName(), "path/to/file") == 0);
        }
        SECTION("doesn't open the file") {
            mocks.NeverCallFunc(lfs_file_open);
            SimpleFileStorage s("path/to/file");
            CHECK(!s.isOpen());
        }
    }
    SECTION("~SimpleFileStorage()") {
        SECTION("closes the file") {
            test::Filesystem fs(&mocks);
            fs.writeFile("file", std::string("\x04\x00\x00\x00""abcd", 8));
            {
                SimpleFileStorage s("file");
                char buf[10] = {};
                s.load(buf, sizeof(buf));
            }
            CHECK(!fs.hasOpenFiles());
        }
    }
    SECTION("load()") {
        SECTION("opens the file only for reading") {
            SimpleFileStorage s("file");
            mocks.ExpectCallFunc(lfs_file_open).Do([](lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
                CHECK(lfs == &filesystem_get_instance(nullptr)->instance);
                CHECK(strcmp(path, "file") == 0);
                CHECK((flags & LFS_O_RDONLY));
                CHECK(!(flags & LFS_O_WRONLY));
                return 0;
            });
            char buf[10] = {};
            s.load(buf, sizeof(buf));
            CHECK(s.isOpen());
        }
        test::Filesystem fs(&mocks);
        SimpleFileStorage s("file");
        SECTION("reads the record data from the file") {
            fs.writeFile("file", std::string("\x04\x00\x00\x00""abcd", 8));
            char buf[10] = {};
            CHECK(s.load(buf, sizeof(buf)) == 4);
            CHECK(memcmp(buf, "abcd", 4) == 0);
        }
        SECTION("can read an empty record from the file") {
            fs.writeFile("file", std::string("\x00\x00\x00\x00", 4));
            char buf[10] = {};
            CHECK(s.load(buf, sizeof(buf)) == 0);
        }
        SECTION("always reads the last record from the file") {
            fs.writeFile("file", std::string("\x04\x00\x00\x00""abcdefgh", 12));
            char buf[10] = {};
            CHECK(s.load(buf, sizeof(buf)) == 4);
            CHECK(memcmp(buf, "efgh", 4) == 0);
        }
        SECTION("can read fewer bytes than the record size") {
            fs.writeFile("file", std::string("\x04\x00\x00\x00""abcdefgh", 12));
            char buf[10] = {};
            CHECK(s.load(buf, 2) == 2);
            CHECK(memcmp(buf, "ef", 2) == 0);
            CHECK(buf[2] == 0);
        }
        SECTION("can read a record larger than the filesystem block") {
            size_t n = FILESYSTEM_BLOCK_SIZE * 2;
            auto d = test::randString(n);
            std::string h(4, '\0');
            h[0] = n & 0xff;
            h[1] = (n >> 8) & 0xff;
            h[2] = (n >> 16) & 0xff;
            h[3] = (n >> 24) & 0xff;
            fs.writeFile("file", h + d);
            char buf[n] = {};
            CHECK(s.load(buf, n) == n);
            CHECK(memcmp(buf, d.data(), n) == 0);
        }
        SECTION("fails if the file doesn't exist") {
            char buf[10] = {};
            CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_NOT_FOUND);
        }
        SECTION("fails if the file size is invalid") {
            char buf[10] = {};
            SECTION("empty file") {
                fs.writeFile("file", std::string());
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_BAD_DATA);
            }
            SECTION("incomplete header") {
                fs.writeFile("file", std::string("\x04\x00\x00", 3));
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_BAD_DATA);
            }
            SECTION("incomplete data") {
                fs.writeFile("file", std::string("\x04\x00\x00\x00""abc", 7));
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_BAD_DATA);
            }
            SECTION("extra data at the end of the file") {
                fs.writeFile("file", std::string("\x00\x00\x00\x00""abcd", 8));
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_BAD_DATA);
            }
        }
        SECTION("fails if the filesystem API returns an error") {
            fs.writeFile("file", "abcd");
            char buf[10] = {};
            SECTION("lfs_file_open()") {
                mocks.OnCallFunc(lfs_file_open).Do([](lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
                    return LFS_ERR_IO;
                });
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_FILE);
            }
            SECTION("lfs_file_seek()") {
                mocks.OnCallFunc(lfs_file_seek).Do([](lfs_t* lfs, lfs_file_t* file, lfs_soff_t offs, int whence) {
                    return LFS_ERR_IO;
                });
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_FILE);
            }
            SECTION("lfs_file_read()") {
                mocks.OnCallFunc(lfs_file_read).Do([](lfs_t* lfs, lfs_file_t* file, void* buf, lfs_size_t size) {
                    return LFS_ERR_IO;
                });
                CHECK(s.load(buf, sizeof(buf)) == SYSTEM_ERROR_FILE);
            }
        }
    }
    SECTION("save()") {
        test::Filesystem fs(&mocks);
        SimpleFileStorage s("file");
        SECTION("creates the file if it doesn't exist") {
            s.save("abcd", 4);
            CHECK(fs.readFile("file") == std::string("\x04\x00\x00\x00""abcd", 8));
            CHECK(s.isOpen());
        }
        SECTION("appends new records to the file") {
            s.save("abcd", 4);
            s.save("efgh", 4);
            CHECK(fs.readFile("file") == std::string("\x04\x00\x00\x00""abcdefgh", 12));
        }
        SECTION("truncates the file when its size exceeds the size of the filesystem block") {
            for (size_t i = 0; i < FILESYSTEM_BLOCK_SIZE - 4; ++i) {
                s.save("a", 1);
            }
            CHECK(fs.readFile("file") == std::string("\x01\x00\x00\x00", 4) + std::string(FILESYSTEM_BLOCK_SIZE - 4, 'a'));
            s.save("a", 1);
            CHECK(fs.readFile("file") == std::string("\x01\x00\x00\x00""a", 5));
        }
        SECTION("can write a record larger than the filesystem block") {
            size_t n = FILESYSTEM_BLOCK_SIZE * 2;
            auto d = test::randString(n);
            std::string h(4, '\0');
            h[0] = n & 0xff;
            h[1] = (n >> 8) & 0xff;
            h[2] = (n >> 16) & 0xff;
            h[3] = (n >> 24) & 0xff;
            s.save(d.data(), d.size());
            CHECK(fs.readFile("file") == h + d);
        }
        SECTION("truncates the file when the size of the record changes") {
            s.save("abc", 3);
            CHECK(fs.readFile("file") == std::string("\x03\x00\x00\x00""abc", 7));
            s.save("defg", 4);
            CHECK(fs.readFile("file") == std::string("\x04\x00\x00\x00""defg", 8));
        }
        SECTION("doesn't mix reads and writes when updating the file") {
            fs.writeFile("file", std::string("\x04\x00\x00\x00""abcd", 8));
            // Reading is acceptable when saving the first record to an existing file, but not subsequent records
            s.save("efgh", 4);
            mocks.NeverCallFunc(lfs_file_read);
            s.save("ijkl", 4);
        }
        SECTION("fails if the filesystem API returns an error") {
            SECTION("lfs_file_open()") {
                mocks.OnCallFunc(lfs_file_open).Do([](lfs_t* lfs, lfs_file_t* file, const char* path, int flags) {
                    return LFS_ERR_IO;
                });
                CHECK(s.save("abcd", 4) == SYSTEM_ERROR_FILE);
            }
            SECTION("lfs_file_write()") {
                mocks.OnCallFunc(lfs_file_write).Do([](lfs_t* lfs, lfs_file_t* file, const void* buf, lfs_size_t size) {
                    return LFS_ERR_IO;
                });
                CHECK(s.save("abcd", 4) == SYSTEM_ERROR_FILE);
            }
        }
    }
    SECTION("sync()") {
        test::Filesystem fs(&mocks);
        SimpleFileStorage s("file");
        SECTION("calls lfs_file_sync() to write all pending modifications to the file") {
            s.save("abcd", 4);
            mocks.ExpectCallFunc(lfs_file_sync).Do([&fs](lfs_t* lfs, lfs_file_t* file) {
                CHECK(lfs == &filesystem_get_instance(nullptr)->instance);
                CHECK(file->fd == fs.lastFileDesc());
                return 0;
            });
            s.sync();
        }
        SECTION("doesn't call lfs_file_sync() if the file is closed") {
            s.save("abcd", 4);
            s.close();
            mocks.NeverCallFunc(lfs_file_sync);
            s.sync();
        }
    }
    SECTION("close()") {
        test::Filesystem fs(&mocks);
        SimpleFileStorage s("file");
        SECTION("closes the file") {
            fs.autoCheckOpenFiles(false); // This test intercepts calls to lfs_file_close()
            s.save("abcd", 4);
            mocks.ExpectCallFunc(lfs_file_close).Do([&fs](lfs_t* lfs, lfs_file_t* file) {
                CHECK(lfs == &filesystem_get_instance(nullptr)->instance);
                CHECK(file->fd == fs.lastFileDesc());
                return 0;
            });
            s.close();
            CHECK(!s.isOpen());
        }
        SECTION("doesn't attempt to close an already closed file") {
            fs.autoCheckOpenFiles(false);
            s.save("abcd", 4);
            auto count = 0;
            mocks.ExpectCallFunc(lfs_file_close).Do([&count](lfs_t* lfs, lfs_file_t* file) {
                ++count;
                return 0;
            });
            s.close();
            s.close();
            CHECK(count == 1);
        }
    }
    SECTION("clear()") {
        test::Filesystem fs(&mocks);
        SimpleFileStorage s("file");
        SECTION("removes the file") {
            s.save("abcd", 4);
            s.clear();
            CHECK(!fs.hasFile("file"));
        }
    }
}
