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
#include <cstdlib>

#include <pb_decode.h>

#include "nanopb_misc.h"
#include "scope_guard.h"
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

    std::unique_ptr<fs::File> appFile(new(std::nothrow) fs::File());
    std::unique_ptr<fs::File> snapshotFile(new(std::nothrow) fs::File());
    if (!appFile || !snapshotFile) {
        return SYSTEM_ERROR_NO_MEMORY;
    }

    Vars vars;
    bool hasStaged = false;
    int r = loadVars(true /* tryStaged */, *appFile, *snapshotFile, vars, hasStaged);
    if (r < 0) {
        if (hasStaged) {
            // Ignore the staged files as a fallback
            vars = Vars();
            r = loadVars(false /* tryStaged */, *appFile, *snapshotFile, vars, hasStaged);
        }
        if (r < 0) {
            LOG(ERROR, "Error while loading env vars: %d", r);
            return r;
        }
    }
    if (!appFile->isOpen()) {
        appFile.reset();
    }
    if (!snapshotFile->isOpen()) {
        snapshotFile.reset();
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
    return r; // 0 or Update::NEED_RESET
}

int EnvVars::get(const char* name, CString& val) const {
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

int EnvVars::get(const char* name, char* buf, size_t bufSize, bool* found) const {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
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

int EnvVars::readValue(const VarEntry& var, char* buf, size_t bufSize) const {
    if (!bufSize) {
        return 0;
    }
    auto& file = (var.src == VarSource::APP) ? *appFile_ : *snapshotFile_;
    CHECK(file.seek(var.valOffs));
    size_t bytesToRead = std::min<size_t>(var.valSize, bufSize - 1);
    size_t bytesRead = CHECK(file.read(buf, bytesToRead));
    if (bytesRead != bytesToRead) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    buf[bytesRead] = '\0';
    return 0;
}

int EnvVars::updateBootloaderVars() const {
    // TODO: Check if any variables used by the bootloader changed (none are defined as of now),
    // apply the changes and return Result::NEED_RESET
    return 0;
}

int EnvVars::loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged) {
    // Load the variables bundled with the app
    CHECK(loadVarsForSource(tryStaged, VarSource::APP, appFile, vars, hasStaged));
    NAMED_SCOPE_GUARD(closeAppFile, {
        appFile.close();
    });

    // Override with the variables set in the cloud
    CHECK(loadVarsForSource(tryStaged, VarSource::SNAPSHOT, snapshotFile, vars, hasStaged));
    closeAppFile.dismiss();
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
            hasStaged = true;
            r = fs::remove(path);
            if (r < 0) {
                LOG(ERROR, "Error while removing %s: %d", path, r);
            }
            return r;
        }
    }

    auto path = (src == VarSource::APP) ? APP_FILE_CURRENT : SNAPSHOT_FILE_CURRENT;
    CHECK(loadVarsFile(path, src, file, vars));
    return 0;
}

int EnvVars::loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars) {
    fs::File f;
    CHECK(f.open(path, LFS_O_RDONLY));
    size_t size = CHECK(f.size());
    if (size > MAX_VARS_FILE_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    CHECK(parseVars(src, f, vars));
    file = std::move(f);
    return 0;
}

int EnvVars::parseVars(VarSource src, fs::File& file, Vars& vars) {
    pb_istream_t stream = {};
    CHECK(pb_istream_from_file(&stream, file.handle(), CHECK(file.size()), nullptr /* reserved */));

    struct DecodeContext {
        fs::File& file;
        VarMap& varMap;
        VarSource varSrc;
        size_t valOffs;
        size_t valSize;
        int error;
    };
    DecodeContext d = {
        .file = file,
        .varMap = vars.entries,
        .varSrc = src,
        .valOffs = 0,
        .valSize = 0,
        .error = 0
    };

    PB_SYSTEM(EnvVars) pbVars = {};
    pbVars.vars.arg = &d;
    pbVars.vars.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;

        PB_SYSTEM(EnvVars_Var) pbVar = {};
        pbVar.value.arg = &d;
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
            .src = d->varSrc
        };
        if (!d->varMap.set(std::move(name), std::move(entry))) {
            d->error = SYSTEM_ERROR_NO_MEMORY;
            return false;
        }
        return true;
    };
    if (!pb_decode(&stream, &PB_SYSTEM(EnvVars_msg), &pbVars)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }

    if (src == VarSource::SNAPSHOT) {
        if (pbVars.hash.size != SNAPSHOT_HASH_SIZE) {
            return SYSTEM_ERROR_BAD_DATA; // Snapshot hash is missing
        }
        vars.snapshotHash.reset(new(std::nothrow) char[SNAPSHOT_HASH_SIZE]);
        if (!vars.snapshotHash) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        std::memcpy(vars.snapshotHash.get(), pbVars.hash.bytes, SNAPSHOT_HASH_SIZE);
    }

    return 0;
}

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
