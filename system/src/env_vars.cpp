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

#include <pb_decode.h>

#include "nanopb_misc.h"
#include "scope_guard.h"
#include "check.h"
#include "logging.h"

#include "system/env_vars.pb.h"

#define PB_SYSTEM(_name) particle_system_##_name

namespace particle::system {

namespace {

const auto APP_VARS_FILE = "/sys/env_app";
const auto APP_VARS_FILE_STAGED = "/sys/env_app.staged";
const auto SNAPSHOT_VARS_FILE = "/sys/env_snapshot";
const auto SNAPSHOT_VARS_FILE_STAGED = "/sys/env_snapshot.staged";

const size_t MAX_VARS_FILE_SIZE = 16 * 1024;

static_assert(EnvVars::MAX_NAME_LEN + 1 == sizeof(PB_SYSTEM(EnvVars_Var::name)));

} // unnamed

EnvVars::EnvVars() {
}

EnvVars::~EnvVars() {
}

int EnvVars::init() {
    auto fsInstance = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr /* reserved */);
    if (!fsInstance) {
        return SYSTEM_ERROR_FILESYSTEM;
    }
    fs::FsLock lock(fsInstance);
    CHECK(filesystem_mount(fsInstance));

    Vars vars;

    // Load the variables bundled with the app
    std::unique_ptr<fs::File> appFile(new(std::nothrow) fs::File(fsInstance));
    if (!appFile) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(openVarsFile(VarSource::APP, *appFile, vars));
    if (!appFile->isOpen()) {
        appFile.reset();
    }

    // Override with the variables set in the cloud
    std::unique_ptr<fs::File> snapshotFile(new(std::nothrow) fs::File(fsInstance));
    if (!snapshotFile) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(openVarsFile(VarSource::SNAPSHOT, *snapshotFile, vars));
    if (!snapshotFile->isOpen()) {
        snapshotFile.reset();
    }

    vars_ = std::move(vars);
    appFile_ = std::move(appFile);
    snapshotFile_ = std::move(snapshotFile);

    return 0;
}

EnvVars& EnvVars::instance() {
    static EnvVars envVars;
    return envVars;
}

int EnvVars::openVarsFile(VarSource src, fs::File& file, Vars& vars) {
    const char* path = nullptr;
    bool tryNormal = true;
    bool tryStaged = true;
    bool isStaged = false;
    bool loaded = false;
    int error = 0;

    for (;;) {
        isStaged = tryStaged;
        if (tryStaged) {
            path = (src == VarSource::APP) ? APP_VARS_FILE_STAGED : SNAPSHOT_VARS_FILE_STAGED;
            tryStaged = false;
        } else if (tryNormal) {
            path = (src == VarSource::APP) ? APP_VARS_FILE : SNAPSHOT_VARS_FILE;
            tryNormal = false;
        } else {
            break;
        }
        int r = file.open(path, LFS_O_RDONLY);
        if (r < 0) {
            if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
                LOG(ERROR, "Error while opening %s: %d", path, r);
                if (!error) {
                    error = r;
                }
            }
            continue;
        }
        r = file.size();
        if (r < 0) {
            if (!error) {
                error = r;
            }
            continue;
        }
        if (r > (int)MAX_VARS_FILE_SIZE) {
            if (!error) {
                error = SYSTEM_ERROR_TOO_LARGE;
            }
            continue;
        }
        r = readVars(src, file, vars);
        if (r < 0) {
            LOG(ERROR, "Error while reading %s: %d", path, r);
            if (!error) {
                error = r;
            }
            r = file.close();
            if (r < 0) {
                LOG(ERROR, "Error while closing %s: %d", path, r);
            }
            // Delete the staged file but not the normal one as this might be an intermittent IO error
            if (isStaged) {
                r = fs::remove(file.lfs(), path);
                if (r < 0) {
                    LOG(ERROR, "Error while removing %s: %d", path, r);
                }
            }
            continue;
        }
        error = 0;
        loaded = true;
        break;
    }
    if (error < 0) {
        return error;
    }
    if (loaded && isStaged) {
        // Rename the staged file
        CHECK(file.close());
        auto newPath = (src == VarSource::APP) ? APP_VARS_FILE : SNAPSHOT_VARS_FILE;
        CHECK(fs::rename(file.lfs(), path, newPath));
        CHECK(file.open(newPath, LFS_O_RDONLY));
    }
    return 0;
}

int EnvVars::readVars(VarSource src, fs::File& file, Vars& vars) {
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
        auto [it, inserted] = d->varMap.insert(std::move(name), entry);
        if (it == d->varMap.end()) {
            d->error = SYSTEM_ERROR_NO_MEMORY;
            return false;
        }
        if (!inserted) {
            it->second = entry;
        }

        return true;
    };
    if (!pb_decode(&stream, &PB_SYSTEM(EnvVars_msg), &pbVars)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }

    if (pbVars.hash.size > 0) {
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

} // particle::system

#endif // HAL_PLATFORM_ENV_VARS
