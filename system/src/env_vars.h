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

#include "asset_manager.h"

#include "filesystem.h"
#include "c_string.h"

#include "spark_wiring_map.h"
#include "spark_wiring_vector.h"

namespace particle::system {

class EnvVars: public SystemAssetHandler {
public:
    static const size_t SNAPSHOT_HASH_SIZE = 32;

    enum Result {
        NEED_RESET = 1
    };

    EnvVars(const EnvVars&) = delete;
    ~EnvVars();

    int init();

    int get(const char* name, CString& val);
    int get(const char* name, char* buf, size_t bufSize, bool* found = nullptr);

    bool has(const char* name) const {
        return vars_.entries.has(name);
    }

    size_t count() const {
        return vars_.entries.size();
    }

    // Invokes a callback for each variable name and value
    template<typename F>
    int forEach(char* buf, size_t bufSize, F fn) {
        for (const auto& entry: vars_.entries) {
            int r = get(entry.first, buf, bufSize);
            if (r < 0) {
                return r;
            }
            r = fn(entry.first.data(), buf);
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
            int r = fn(entry.first.data());
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
        return snapshotFile_.isOpen();
    }

    int handleAsset(const Asset& asset, InputStream& data) override;

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
    fs::File appFile_;
    fs::File snapshotFile_;

    EnvVars() = default; // Use instance()

    int updateBootloaderVars();
    int readValue(const VarEntry& var, char* buf, size_t bufSize);

    static int loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged);
    static int loadVarsForSource(bool tryStaged, VarSource src, fs::File& file, Vars& vars, bool& hasStaged);
    static int loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars);
    static int readVars(VarSource src, fs::File& file, Vars& vars);
};

/**
 * Get the value of an environment variable.
 *
 * @param name Variable name.
 * @param defaultVal Default value.
 * @return Variable value if defined, or the default value.
 */
CString getEnv(const char* name, const char* defaultVal);

/**
 * Get the value of an environment variable and convert it to `int`.
 *
 * @param name Variable name.
 * @param defaultVal Default value.
 * @return Variable value if defined, or the default value.
 */
int getEnv(const char* name, int defaultVal);

/**
 * Get the value of an environment variable and convert it to `bool`.
 *
 * @param name Variable name.
 * @param defaultVal Default value.
 * @return Variable value if defined, or the default value.
 */
bool getEnv(const char* name, bool defaultVal);

/**
 * Get the value of an environment variable.
 *
 * Alias for `EnvVars::get(const char* name, CString& val)`.
 *
 * @param name Variable name.
 * @param[out] val Variable value. Will be set to a null-initialized string if the variable is
 *             not defined.
 * @return 0 on success, or an error code defined by `system_error_t`.
 */
int getEnv(const char* name, CString& val);

/**
 * Get the value of an environment variable.
 *
 * The output is always null-terminated unless the size of the output buffer is 0.
 *
 * Alias for `EnvVars::get(const char* name, char* buf, size_t bufSize, bool* found)`.
 *
 * @param name Variable name.
 * @param buf Output buffer.
 * @param bufSize Size of the output buffer.
 * @param[out] found Will be set to `true` if the variable is defined, otherwise to `false`.
 * @return On success, the actual length of the variable value, not including `\0`, or 0 if the
 *         variable is not defined. On failure, an error code defined by `system_error_t`. The
 *         returned length can be greater than the size of the output buffer.
 */
int getEnv(const char* name, char* buf, size_t bufSize, bool* found = nullptr);

/**
 * Check if an environment variable is defined.
 *
 * Alias for `EnvVars::has(const char* name)`.
 *
 * @param name Variable name.
 * @return `true` if the variable is defined, otherwise `false`.
 */
bool hasEnv(const char* name);

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
