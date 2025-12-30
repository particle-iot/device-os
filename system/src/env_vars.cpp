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

#include "env_vars.h"

#if HAL_PLATFORM_ENV_VARS

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <cstring>

#include <pb_decode.h>

#include "nanopb_misc.h"
#include "logging.h"
#include "check.h"

#include "system/env_vars.pb.h"

#define PB_SYSTEM(_name) particle_system_##_name

namespace particle::system {

namespace {

const size_t MAX_VARS_FILE_SIZE = 16 * 1024;

} // unnamed

const char* const EnvVars::APP_FILE_CURRENT = "/sys/env_app";
const char* const EnvVars::APP_FILE_STAGED = "/sys/env_app.staged";
const char* const EnvVars::SNAPSHOT_FILE_CURRENT = "/sys/env_snapshot";
const char* const EnvVars::SNAPSHOT_FILE_STAGED = "/sys/env_snapshot.staged";

EnvVars::~EnvVars() {
}

int EnvVars::init() {
    fs::FsLock lock;
    CHECK(fs::mount());

    Vars vars;
    fs::File appFile;
    fs::File snapshotFile;

    bool hasStaged = false;
    int r = loadVars(true /* tryStaged */, appFile, snapshotFile, vars, hasStaged);
    if (r < 0) {
        if (hasStaged) {
            // Ignore the staged files as a fallback
            vars = Vars();
            appFile.close();
            snapshotFile.close();
            hasStaged = false;
            r = loadVars(false /* tryStaged */, appFile, snapshotFile, vars, hasStaged);
        }
        if (r < 0) {
            LOG(ERROR, "Error while loading env vars: %d", r);
            return r;
        }
    }

    // Clean up empty files
    if (appFile.isOpen() && appFile.size() == 0) {
        appFile.close();
        fs::remove(APP_FILE_CURRENT);
        fs::remove(APP_FILE_STAGED);
    }
    if (snapshotFile.isOpen() && snapshotFile.size() == 0) {
        snapshotFile.close();
        fs::remove(SNAPSHOT_FILE_CURRENT);
        fs::remove(SNAPSHOT_FILE_STAGED);
    }

    vars_ = std::move(vars);
    appFile_ = std::move(appFile);
    snapshotFile_ = std::move(snapshotFile);

    r = updateBootloaderVars();
    if (r < 0) {
        LOG(ERROR, "Error while updating bootloader env vars: %d", r);
        // The variables have been loaded successfully and there's not much we can do about
        // the bootloader
        r = 0;
    }
    return r; // 0 or Result::NEED_RESET
}

int EnvVars::get(const char* name, CString& val) {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
        val = CString();
        return 0;
    }
    const auto& var = it->second;
    auto buf = (char*)std::malloc(var.valSize + 1);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto s = CString::wrap(buf); // Takes ownership over the buffer
    CHECK(readValue(var, buf, var.valSize + 1));
    val = std::move(s);
    return 0;
}

int EnvVars::get(const char* name, char* buf, size_t bufSize, bool* found) {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
        if (bufSize > 0) {
            buf[0] = '\0';
        }
        if (found) {
            *found = false;
        }
        return 0;
    }
    const auto& var = it->second;
    CHECK(readValue(var, buf, bufSize));
    if (found) {
        *found = true;
    }
    return var.valSize;
}

EnvVars& EnvVars::instance() {
    static EnvVars envVars;
    return envVars;
}

int EnvVars::updateBootloaderVars() {
    // TODO: Check if any variables used by the bootloader changed (none are defined as of now),
    // apply the changes and return Result::NEED_RESET
    return 0;
}

int EnvVars::readValue(const VarEntry& var, char* buf, size_t bufSize) {
    if (!bufSize) {
        return 0;
    }
    auto& file = (var.src == VarSource::APP) ? appFile_ : snapshotFile_;
    CHECK(file.seek(var.valOffs));
    size_t bytesToRead = std::min<size_t>(var.valSize, bufSize - 1);
    size_t bytesRead = CHECK(file.read(buf, bytesToRead));
    if (bytesRead != bytesToRead) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    buf[bytesRead] = '\0';
    return 0;
}

int EnvVars::loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged) {
    // Load the variables bundled with the app
    CHECK(loadVarsForSource(tryStaged, VarSource::APP, appFile, vars, hasStaged));

    // Override with the variables from the snapshot
    CHECK(loadVarsForSource(tryStaged, VarSource::SNAPSHOT, snapshotFile, vars, hasStaged));
    return 0;
}

int EnvVars::loadVarsForSource(bool tryStaged, VarSource src, fs::File& file, Vars& vars, bool& hasStaged) {
    if (tryStaged) {
        auto path = (src == VarSource::APP) ? APP_FILE_STAGED : SNAPSHOT_FILE_STAGED;
        int r = loadVarsFile(path, src, file, vars);
        if (r >= 0) {
            hasStaged = true;
            // Rename the staged file
            CHECK(file.close());
            auto newPath = (src == VarSource::APP) ? APP_FILE_CURRENT : SNAPSHOT_FILE_CURRENT;
            CHECK(fs::rename(path, newPath));
            // Reopen the file
            CHECK(file.open(newPath, LFS_O_RDONLY));
            return 0;
        } else if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            LOG(ERROR, "Error while reading %s: %d", path, r);
            hasStaged = true;
            r = fs::remove(path);
            if (r < 0) {
                LOG(ERROR, "Error while removing %s: %d", path, r);
            }
            return r;
        }
    }

    auto path = (src == VarSource::APP) ? APP_FILE_CURRENT : SNAPSHOT_FILE_CURRENT;
    int r = loadVarsFile(path, src, file, vars);
    if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
        return r;
    }
    return 0;
}

int EnvVars::loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars) {
    fs::File f;
    CHECK(f.open(path, LFS_O_RDONLY));
    size_t size = CHECK(f.size());
    if (size > MAX_VARS_FILE_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    // As a special case, allow an app/snapshot file to be empty so that flashing it would clean up
    // the corresponding file on the device (see `init()`)
    if (size > 0) {
        CHECK(readVars(src, f, vars));
    }
    file = std::move(f); // Keep the file open
    return 0;
}

int EnvVars::readVars(VarSource src, fs::File& file, Vars& vars) {
    pb_istream_t stream = {};
    CHECK(pb_istream_from_file(&stream, file.handle(), CHECK(file.size()), nullptr /* reserved */));

    struct DecodeContext {
        fs::File& file;
        VarEntries& entries;
        VarSource src;
        size_t valOffs; // Offset of the last read variable value
        size_t valSize; // Size of the last read variable value
        unsigned varCount; // Number of variables read
        int error;
    };
    DecodeContext d = {
        .file = file,
        .entries = vars.entries,
        .src = src,
        .valOffs = 0,
        .valSize = 0,
        .varCount = 0,
        .error = 0
    };

    PB_SYSTEM(EnvVars) pbVars = {};
    pbVars.vars.arg = &d;
    pbVars.vars.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;

        PB_SYSTEM(EnvVars_Var) pbVar = {};
        pbVar.value.arg = d;
        pbVar.value.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
            auto d = (DecodeContext*)*arg;

            int r = d->file.tell();
            if (r < 0) {
                d->error = r;
                return false;
            }
            d->valOffs = r;
            d->valSize = stream->bytes_left;

            return pb_read(stream, nullptr /* buf */, d->valSize); // Skip the value bytes
        };
        if (!pb_decode(stream, &PB_SYSTEM(EnvVars_Var_msg), &pbVar)) {
            return false;
        }

        if (!pbVar.name[0]) {
            d->error = SYSTEM_ERROR_BAD_DATA; // Empty names are not allowed
            return false;
        }
        CString name(pbVar.name);
        if (!name) {
            d->error = SYSTEM_ERROR_NO_MEMORY;
            return false;
        }
        VarEntry entry = {
            .valOffs = (uint16_t)d->valOffs,
            .valSize = (uint16_t)d->valSize,
            .src = d->src
        };
        if (!d->entries.set(std::move(name), std::move(entry))) {
            d->error = SYSTEM_ERROR_NO_MEMORY;
            return false;
        }
        ++d->varCount;
        return true;
    };
    if (!pb_decode(&stream, &PB_SYSTEM(EnvVars_msg), &pbVars)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }

    if (src == VarSource::SNAPSHOT) {
        if (pbVars.hash.size != SNAPSHOT_HASH_SIZE) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        vars.snapshotHash.reset(new(std::nothrow) char[SNAPSHOT_HASH_SIZE]);
        if (!vars.snapshotHash) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        std::memcpy(vars.snapshotHash.get(), pbVars.hash.bytes, SNAPSHOT_HASH_SIZE);
    }
    return 0;
}

CString getEnv(const char* name, const char* defaultVal) {
    CString val;
    int r = EnvVars::instance().get(name, val);
    if (r < 0) {
        return defaultVal;
    }
    return val;
}

int getEnv(const char* name, int defaultVal) {
    char buf[32] = {};
    bool found = false;
    int n = EnvVars::instance().get(name, buf, sizeof(buf), &found);
    if (n < 0 || !found) {
        return defaultVal;
    }
    int val = 0;
    auto r = std::from_chars(buf, buf + n, val);
    if (r.ec != std::errc() || r.ptr != buf + n) {
        return defaultVal;
    }
    return val;
}

bool getEnv(const char* name, bool defaultVal) {
    char buf[32] = {};
    bool found = false;
    int n = EnvVars::instance().get(name, buf, sizeof(buf), &found);
    if (n < 0 || !found) {
        return defaultVal;
    }
    if (std::strcmp(buf, "true") == 0) {
        return true;
    }
    if (std::strcmp(buf, "false") == 0) {
        return false;
    }
    // Try parsing as a number
    int val = 0;
    auto r = std::from_chars(buf, buf + n, val);
    if (r.ec != std::errc() || r.ptr != buf + n) {
        return defaultVal;
    }
    return val;
}

int getEnv(const char* name, CString& val) {
    return EnvVars::instance().get(name, val);
}

int getEnv(const char* name, char* buf, size_t bufSize, bool* found) {
    return EnvVars::instance().get(name, buf, bufSize, found);
}

bool hasEnv(const char* name) {
    return EnvVars::instance().has(name);
}

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
