/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "Particle.h"

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace particle {
namespace test {

const char* const WIFI_SUITE_BACKUP = "wifi_suite_wifi_config";
const char* const TEST_RUN_BACKUP = "test_run_wifi_config";

namespace detail {

const char* const WIFI_CONFIG_FILE = "/sys/wifi_config.bin";
const char* const WIFI_CONFIG_RESTORE_FILE = "/sys/wifi_config.bin.restore";
const system_tick_t WIFI_OFF_TIMEOUT = 60000;

} // namespace detail

class WifiConfigBackup {
public:
    explicit WifiConfigBackup(const char* name) :
            path_(String::format("/usr/%s.bin", name)),
            noConfigPath_(String::format("/usr/%s.none", name)),
            tmpPath_(String::format("/usr/%s.tmp", name)) {
    }

    bool exists() const {
        return fileExists(path_.c_str()) || fileExists(noConfigPath_.c_str());
    }

    bool save() {
        discard();
        if (!fileExists(detail::WIFI_CONFIG_FILE)) {
            return createFile(noConfigPath_.c_str());
        }
        if (!copyFile(detail::WIFI_CONFIG_FILE, tmpPath_.c_str()) || rename(tmpPath_.c_str(), path_.c_str()) != 0) {
            unlink(tmpPath_.c_str());
            return false;
        }
        return true;
    }

    bool restore() {
        if (!exists()) {
            return true;
        }
        if (!matchesConfig()) {
            const bool wasOn = !WiFi.isOff();
            WiFi.off();
            const bool ok = System.waitCondition([]() { return WiFi.isOff(); }, detail::WIFI_OFF_TIMEOUT) && writeConfig();
            if (wasOn) {
                WiFi.on();
            }
            if (!ok) {
                return false;
            }
        }
        discard();
        return true;
    }

    void discard() {
        unlink(path_.c_str());
        unlink(noConfigPath_.c_str());
        unlink(tmpPath_.c_str());
    }

private:
    String path_;
    String noConfigPath_;
    String tmpPath_;

    bool matchesConfig() const {
        if (!fileExists(path_.c_str())) {
            return !fileExists(detail::WIFI_CONFIG_FILE);
        }
        return sameContents(path_.c_str(), detail::WIFI_CONFIG_FILE);
    }

    bool writeConfig() const {
        if (!fileExists(path_.c_str())) {
            unlink(detail::WIFI_CONFIG_FILE);
            return !fileExists(detail::WIFI_CONFIG_FILE);
        }
        if (!copyFile(path_.c_str(), detail::WIFI_CONFIG_RESTORE_FILE) || rename(detail::WIFI_CONFIG_RESTORE_FILE, detail::WIFI_CONFIG_FILE) != 0) {
            unlink(detail::WIFI_CONFIG_RESTORE_FILE);
            return false;
        }
        return true;
    }

    static bool fileExists(const char* path) {
        struct stat st = {};
        return stat(path, &st) == 0;
    }

    static bool createFile(const char* path) {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            return false;
        }
        close(fd);
        return true;
    }

    static bool copyFile(const char* src, const char* dst) {
        int in = open(src, O_RDONLY);
        if (in < 0) {
            return false;
        }
        int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out < 0) {
            close(in);
            return false;
        }
        bool ok = true;
        char buf[128];
        for (;;) {
            ssize_t n = read(in, buf, sizeof(buf));
            if (n <= 0) {
                ok = (n == 0);
                break;
            }
            if (write(out, buf, n) != n) {
                ok = false;
                break;
            }
        }
        close(in);
        if (close(out) != 0) {
            ok = false;
        }
        return ok;
    }

    static bool sameContents(const char* a, const char* b) {
        int fa = open(a, O_RDONLY);
        if (fa < 0) {
            return false;
        }
        int fb = open(b, O_RDONLY);
        if (fb < 0) {
            close(fa);
            return false;
        }
        bool same = true;
        char bufA[64];
        char bufB[64];
        for (;;) {
            ssize_t na = read(fa, bufA, sizeof(bufA));
            ssize_t nb = read(fb, bufB, sizeof(bufB));
            if (na != nb || na < 0 || memcmp(bufA, bufB, na) != 0) {
                same = false;
                break;
            }
            if (na == 0) {
                break;
            }
        }
        close(fa);
        close(fb);
        return same;
    }
};

} // namespace test
} // namespace particle

#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
