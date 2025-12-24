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

namespace particle::system {

class EnvVars {
public:
    static const char* const APP_FILE;
    static const char* const APP_FILE_STAGED;
    static const char* const SNAPSHOT_FILE;
    static const char* const SNAPSHOT_FILE_STAGED;

    static const size_t SNAPSHOT_HASH_SIZE = 32;

    enum Result {
        NEED_RESET = 1
    };

    EnvVars(const EnvVars&) = delete;
    ~EnvVars();

    int init();

    CString get(const char* name);
    int get(const char* name, char* buf, size_t bufSize);
    bool has(const char* name) const;

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

    typedef Map<CString, VarEntry, CString::Less> VarMap;

    struct Vars {
        VarMap entries;
        std::unique_ptr<char[]> snapshotHash;
    };

    Vars vars_;
    std::unique_ptr<fs::File> appFile_;
    std::unique_ptr<fs::File> snapshotFile_;

    EnvVars();

    static int loadVarsFile(VarSource src, fs::File& file, Vars& vars);
    static int loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars);
    static int readVars(VarSource src, fs::File& file, Vars& vars);
};

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
