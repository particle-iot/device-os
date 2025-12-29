/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#if HAL_PLATFORM_ENV_VARS

#include <memory>

#include "filesystem.h"
#include "c_string.h"

#include "spark_wiring_map.h"
#include "spark_wiring_vector.h"

namespace particle::system {

class EnvVars {
public:
    static const char* const APP_FILE_CURRENT;
    static const char* const APP_FILE_STAGED;
    static const char* const SNAPSHOT_FILE_CURRENT;
    static const char* const SNAPSHOT_FILE_STAGED;

    static const size_t SNAPSHOT_HASH_SIZE = 32;

    enum Result {
        NEED_RESET = 1
    };

    EnvVars(const EnvVars&) = delete;
    ~EnvVars();

    int init();

    int get(const char* name, CString& val) const;
    int get(const char* name, char* buf, size_t bufSize, bool* found = nullptr) const;

    bool has(const char* name) const {
        return vars_.entries.has(name);
    }

    size_t count() const {
        return vars_.entries.size();
    }

    // Invokes a callback for each variable name and value
    template<typename F>
    int forEach(char* buf, size_t bufSize, F fn) const {
        for (const auto& entry: vars_.entries) {
            int r = get(entry.first, buf, bufSize);
            if (r < 0) {
                return r;
            }
            r = fn(entry.first, buf);
            if (r < 0) {
                return r;
            }
        }
        return vars_.entries.size();
    }

    // Invokes a callback for each variable name
    template<typename F>
    int forEach(F fn) const {
        for (const auto& entry: vars_.entries) {
            int r = fn(entry.first);
            if (r < 0) {
                return r;
            }
        }
        return vars_.entries.size();
    }

    const char* snapshotHash() const {
        return vars_.snapshotHash.get();
    }

    bool hasSnapshot() const {
        return snapshotFile_.get();
    }

    EnvVars& operator=(const EnvVars&) = delete;

    static EnvVars& instance();

private:
    enum VarSource {
        APP,
        SNAPSHOT
    };

    struct VarEntry {
        uint16_t valOffs;
        uint16_t valSize;
        VarSource src;
    };

    typedef Map<CString, VarEntry, CString::Less> VarEntries;

    struct Vars {
        VarEntries entries;
        std::unique_ptr<char[]> snapshotHash;
    };

    Vars vars_;
    std::unique_ptr<fs::File> appFile_;
    std::unique_ptr<fs::File> snapshotFile_;

    EnvVars() = default; // Use instance()

    int updateBootloaderVars() const;
    int readValue(const VarEntry& var, char* buf, size_t bufSize) const;

    static int loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged);
    static int loadVarsForSource(bool tryStaged, VarSource src, fs::File& file, Vars& vars, bool& hasStaged);
    static int loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars);
    static int readVars(VarSource src, fs::File& file, Vars& vars);
};

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
