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

#ifdef __cplusplus

#include <memory>

#include "asset_manager.h"

#include "filesystem.h"
#include "c_string.h"

#include "spark_wiring_map.h"

namespace particle::system {

class Env: public SystemAssetHandler {
public:
    static const size_t SNAPSHOT_HASH_SIZE = 32;

    Env(const Env&) = delete;
    ~Env();

    int init();

    int get(const char* name, char* buf, size_t bufSize);
    int get(const char* name, CString& val);
    int get(const char* name, int& val);
    int get(const char* name, bool& val);

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

    int clear();

    // Reimplemented from SystemAssetHandler
    int updateAsset(const Asset& asset, InputStream& data) override;
    int removeAsset(const Asset& asset) override;
    AssetStorageOption storageOptionForAsset(const Asset& asset) override;

    Env& operator=(const Env&) = delete;

    static Env& instance();

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

    Env() = default; // Use instance()

    int updateBootloaderVars();
    int readValue(const VarEntry& var, char* buf, size_t bufSize);

    static int loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged);
    static int loadVarsForSource(bool tryStaged, VarSource src, fs::File& file, Vars& vars, bool& hasStaged);
    static int loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars);
    static int readVars(VarSource src, fs::File& file, Vars& vars);
};

/**
 * Get the value of an environment variable as a string.
 *
 * Only modifies the output parameter if the variable is found. If the variable
 * is not found, the output parameter is left unchanged, allowing callers to
 * pre-set a default value.
 *
 * @param name Variable name.
 * @param[in,out] val On input, may contain a default value.
 *                    On output, the variable value if found, otherwise unchanged.
 * @return `true` if the variable is defined and was retrieved successfully,
 *         `false` if the variable is not defined or an error occurred.
 *
 * Example:
 * ```
 * CString value; // Do not assign a default value here to avoid unnecessary memory allocation
 * if (getEnv("MY_VAR", value)) {
 *     // value now contains the env var value
 * } else {
 *     value = "default";
 * }
 * ```
 */
inline bool getEnv(const char* name, CString& val) {
    return Env::instance().get(name, val) >= 0;
}

/**
 * Get the value of an environment variable into a character buffer.
 *
 * Only modifies the output buffer if the variable is found. If the variable
 * is not found, the buffer is left unchanged.
 *
 * The output is always null-terminated if the variable is found and bufSize > 0.
 * If the buffer is too small, the value is truncated but still null-terminated.
 *
 * @param name Variable name.
 * @param[out] buf Output buffer. Left unchanged if variable not found.
 * @param bufSize Size of the output buffer.
 * @return `true` if the variable is defined and was retrieved successfully,
 *         `false` if the variable is not defined or an error occurred.
 */
inline bool getEnv(const char* name, char* buf, size_t bufSize) {
    return Env::instance().get(name, buf, bufSize) >= 0;
}

/**
 * Get the value of an environment variable and validate it as a boolean.
 *
 * Validates that the environment variable contains exactly "true" or "false"
 * (case-sensitive, lowercase only). Any other value is considered invalid.
 *
 * Only modifies the output parameter if the variable is found AND contains
 * a valid boolean value. If the variable is not found or contains an invalid
 * value, the output parameter is left unchanged.
 *
 * @param name Variable name.
 * @param[in,out] val On input, may contain a default value.
 *                    On output, `true` if value is "true",
 *                    `false` if value is "false",
 *                    otherwise unchanged.
 * @return `true` if the variable is defined AND contains a valid boolean value,
 *         `false` if the variable is not defined, empty, or contains an invalid value.
 *
 * Valid values: "true", "false" (lowercase only, case-sensitive)
 * Invalid values: "TRUE", "FALSE", "True", "1", "0", "yes", "no", "", or any other string
 *
 * Example:
 * ```
 * bool enabled = true;  // default
 * if (getEnv("FEATURE_ENABLE", enabled)) {
 *     // enabled now contains the parsed boolean
 * } else {
 *     // enabled still contains true (default)
 * }
 * ```
 */
inline bool getEnv(const char* name, bool& val) {
    return Env::instance().get(name, val) == 0;
}

/**
 * Get the value of an environment variable and validate it as a 32-bit signed integer.
 *
 * Validates that the environment variable contains a valid decimal integer that
 * fits within int32_t range (-2147483648 to 2147483647). Only decimal digits
 * with an optional leading minus sign are accepted.
 *
 * Only modifies the output parameter if the variable is found AND contains
 * a valid integer value. If the variable is not found or contains an invalid
 * value, the output parameter is left unchanged.
 *
 * @param name Variable name.
 * @param[in,out] val On input, may contain a default value.
 *                    On output, the parsed integer if valid, otherwise unchanged.
 * @return `true` if the variable is defined AND contains a valid 32-bit signed integer,
 *         `false` if the variable is not defined, empty, contains non-numeric characters,
 *         or the value overflows int32_t range.
 *
 * Valid values: "0", "123", "-456", "2147483647", "-2147483648"
 * Invalid values: "", "12.34", "0x1F", "12abc", " 123", "123 ", overflow values
 *
 * Example:
 * ```
 * int timeout = 30;  // default
 * if (getEnv("TIMEOUT_SEC", timeout)) {
 *     // timeout now contains the parsed integer
 * } else {
 *     // timeout still contains 30 (default)
 * }
 * ```
 */
inline bool getEnv(const char* name, int& val) {
    return Env::instance().get(name, val) == 0;
}

/**
 * Check if an environment variable is defined.
 *
 * @param name Variable name.
 * @return `true` if the variable is defined, otherwise `false`.
 */
inline bool hasEnv(const char* name) {
    return Env::instance().has(name);
}

} // particle::system

extern "C" {
#endif // defined(__cplusplus)

#include <stdbool.h>

#define SYSTEM_ENV_NEED_RESET 1

/**
 * Get the value of an environment variable.
 *
 * The output is always null-terminated unless the size of the output buffer is 0.
 *
 * @param name Variable name.
 * @param buf Output buffer.
 * @param buf_size Size of the output buffer.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual length of the variable value, not including `\0`, otherwise an
 *         error code defined by `system_error_t`. The returned length can be greater than the size
 *         of the output buffer. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not defined.
 */
int system_get_env(const char* name, char* buf, size_t buf_size, void* reserved);

/**
 * Get the value of an environment variable and convert it to an integer.
 *
 * Only decimal digits with an optional leading minus sign are valid.
 *
 * @param name Variable name.
 * @param[out] val Output value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid integer, otherwise an error code
 *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not
 *         defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid integer or is
 *         not in the range representable by `int`.
 */
int system_get_env_int(const char* name, int* val, void* reserved);

/**
 * Get the value of an environment variable and convert it to a boolean.
 *
 * Only `true` and `false` (case-sensitive, lowercase only) are valid.
 *
 * @param name Variable name.
 * @param[out] val Output value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid boolean value, otherwise an error code
 *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not
 *         defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid boolean.
 */
int system_get_env_bool(const char* name, bool* val, void* reserved);

/**
 * List all defined environment variables.
 *
 * @param[out] names Array to store the variable names.
 * @param count Maximum number of elements that can be stored in the array.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual number of defined variables, otherwise an error code defined by
 *         `system_error_t`. The returned number of variables can be greater than the size of the
 *         output array.
 */
int system_list_env(const char* names[], size_t count, void* reserved);

/**
 * Clear all defined environment variables.
 *
 * The variables will be cleared next time the device boots.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 or `SYSTEM_ENV_NEED_RESET` on success, otherwise an error code defined by
 *         `system_error_t`. `SYSTEM_ENV_NEED_RESET` indicates that a system reset is needed for
 *         the changes to take effect.
 */
int system_clear_env(void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_PLATFORM_ENV_VARS
