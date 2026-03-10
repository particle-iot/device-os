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

#if HAL_PLATFORM_ENV

#ifdef __cplusplus

#include <memory>

#include "asset_manager.h"

#include "filesystem.h"
#include "c_string.h"

#include "spark_wiring_map.h"

namespace particle {

class InputStream;

namespace system {

class Env: public SystemAssetHandler {
public:
    /**
     * Variable info.
     */
    struct VarInfo {
        /**
         * Variable name.
         */
        const char* name;
        /**
         * Size of the variable value not including '\0'.
         */
        size_t size;
        /**
         * Whether this is an application variable.
         */
        bool isApp;

        VarInfo(const char* name, size_t size, bool isApp) :
                name(name),
                size(size),
                isApp(isApp) {
        }
    };

    /**
     * Size of a snapshot hash in bytes.
     */
    static const size_t SNAPSHOT_HASH_SIZE = 32;

    Env(const Env&) = delete;
    ~Env();

    int init();

    /**
     * Get the value of an environment variable.
     *
     * The output is always null-terminated unless an the size of the output buffer is 0.
     *
     * @param name Variable name.
     * @param buf Output buffer. Only modified if the variable is defined.
     * @param buf_size Size of the output buffer.
     * @return On success, the actual length of the variable value, not including `\0`, otherwise
     *         an error code defined by `system_error_t`. The returned length can be greater than
     *         the size of the output buffer. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable
     *         is not defined.
     */
    int get(const char* name, char* buf, size_t bufSize);

    /**
     * Get the value of an environment variable as a `CString`.
     *
     * @param name Variable name.
     * @param[out] val Variable value. Only modified if the variable is defined.
     * @return On success, the length of the variable value, not including `\0`, otherwise an error
     *         code defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable
     *         is not defined.
     */
    int get(const char* name, CString& val);

    /**
     * Get the value of an environment variable and convert it to an `int`.
     *
     * Only decimal digits with an optional leading minus sign are valid.
     *
     * @param name Variable name.
     * @param[out] val Variable value. Only modified if the variable is defined and valid.
     * @return 0 if the variable is defined and contains a valid integer, otherwise an error code
     *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is
     *         not defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid integer
     *         in the range representable by `int`.
     */
    int get(const char* name, int& val);

    /**
     * Get the value of an environment variable and convert it to a `bool`.
     *
     * Only `true` and `false` (case-sensitive, lowercase only) are valid.
     *
     * @param name Variable name.
     * @param[out] val Variable value. Only modified if the variable is defined and valid.
     * @return 0 if the variable is defined and contains a valid boolean value, otherwise an error
     *         code defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable
     *         is not defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid boolean.
     */
    int get(const char* name, bool& val);

    /**
     * Get a stream for reading the value of an environment variable.
     *
     * @param name Variable name.
     * @param[out] stream Stream for reading the variable value.
     * @return 0 if the variable is defined, otherwise an error code defined by `system_error_t`.
     *         Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not defined.
     */
    int get(const char* name, std::unique_ptr<InputStream>& stream);

    /**
     * Check if an environment variable is defined.
     *
     * @param name Variable name.
     * @return `true` if the variable is defined, otherwise `false`.
     */
    bool has(const char* name) const {
        return vars_.entries.has(name);
    }

    /**
     * Get the number of defined environment variables.
     *
     * @return Number of variables.
     */
    size_t count() const {
        return vars_.entries.size();
    }

    /**
     * Invoke a callback for each defined variable.
     *
     * The callback must accept a `VarInfo` argument and return 0 on success:
     * ```
     * Env::instance().forEach([](const Env::VarInfo& var) {
     *     LOG(INFO, "%s", var.name);
     *     return 0;
     * });
     * ```
     *
     * If the callback returns a negative value, the enumeration stops and the value is returned to
     * the caller of this method.
     *
     * @param fn Callback.
     * @return On success, the number of defined variables, otherwise an error code defined by
     *         `system_error_t` or the negative value returned by the callback.
     */
    template<typename F>
    int forEach(F&& fn) {
        for (const auto& entry: vars_.entries) {
            const auto& name = entry.first;
            const auto& var = entry.second;
            int r = fn(VarInfo(name.data(), var.valSize, var.src == VarSource::APP));
            if (r < 0) {
                return r;
            }
        }
        return vars_.entries.size();
    }

    /**
     * Invoke a callback for each defined variable.
     *
     * The callback must have the signature `int(const Env::VarInfo&, const char*)` and return 0 on
     * success. The second argument is a pointer to the buffer provided by the caller where the value
     * of the variable will be stored:
     * ```
     * char val[128];
     * Env::instance().forEach(val, sizeof(val), [](const Env::VarInfo& var, const char* val) {
     *     LOG(INFO, "%s=%s", var.name, val);
     *     return 0;
     * });
     * ```
     *
     * Note that depending on the buffer size and the actual size of the variable value (`VarInfo::size`),
     * the value in the buffer can be truncated (but still terminated with `\0`).
     *
     * If the callback returns a negative value, the enumeration stops and the value is returned to
     * the caller of this method.
     *
     * @param buf Buffer for the variable value.
     * @param bufSize Buffer size.
     * @param fn Callback.
     * @return On success, the number of defined variables, otherwise an error code defined by
     *         `system_error_t` or the negative value returned by the callback.
     */
    template<typename F>
    int forEach(char* buf, size_t bufSize, F&& fn) {
        for (const auto& entry: vars_.entries) {
            const auto& name = entry.first;
            const auto& var = entry.second;
            int r = readValue(var, buf, bufSize);
            if (r < 0) {
                return r;
            }
            r = fn(VarInfo(name.data(), var.valSize, var.src == VarSource::APP), buf);
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

// Helper functions that return `true` if the variable is defined and valid, or `false` if the
// variable is not defined or an error occurs when getting the variable value
inline bool getEnv(const char* name, char* buf, size_t bufSize) {
    return Env::instance().get(name, buf, bufSize) >= 0;
}

inline bool getEnv(const char* name, CString& val) {
    return Env::instance().get(name, val) >= 0;
}

inline bool getEnv(const char* name, bool& val) {
    return Env::instance().get(name, val) == 0;
}

inline bool getEnv(const char* name, int& val) {
    return Env::instance().get(name, val) == 0;
}

inline bool hasEnv(const char* name) {
    return Env::instance().has(name);
}

} // particle::system

} // particle

extern "C" {
#endif // defined(__cplusplus)

#include <stdbool.h>

#define SYSTEM_ENV_NEED_RESET 1

/**
 * Signature of a callback for `system_for_each_env()`.
 *
 * @param name Variable name.
 * @param val Variable value. This is the `buf` pointer provided to `system_for_each_env()` by the caller.
 * @param val_size Actual length of the variable value. Can be greater than `buf_size` provided to
 *        `system_for_each_env()` by the caller.
 * @param arg Callback argument.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
typedef int (*system_for_each_env_fn)(const char* name, const char* val, size_t val_size, void* arg);

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
 * Get the value of an environment variable and convert it to an `int`.
 *
 * Only decimal digits with an optional leading minus sign are valid.
 *
 * @param name Variable name.
 * @param[out] val Output value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid integer, otherwise an error code
 *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not
 *         defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid integer in the
 *         range representable by `int`.
 */
int system_get_env_int(const char* name, int* val, void* reserved);

/**
 * Get the value of an environment variable and convert it to a `bool`.
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
 * Invoke a callback for each defined environment variable.
 *
 * @param fn Callback function.
 * @param arg Callback argument.
 * @param buf Buffer for storing the value of each variable. Can be `NULL` if `buf_size` is 0.
 * @param buf_size Size of the buffer `buf`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the number of defined variables, otherwise an error code defined by
 *         `system_error_t` or the negative value returned by the callback.
 */
int system_for_each_env(system_for_each_env_fn fn, void* arg, char* buf, size_t buf_size, void* reserved);

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

#endif // HAL_PLATFORM_ENV
