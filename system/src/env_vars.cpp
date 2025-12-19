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

#include "scope_guard.h"
#include "check.h"
#include "logging.h"

namespace particle {

namespace system {

namespace {

const auto APP_VARS_FILE = "/sys/env_app";
const auto APP_VARS_FILE_STAGED = "/sys/env_app.staged";
const auto SNAPSHOT_VARS_FILE = "/sys/env_snapshot";
const auto SNAPSHOT_VARS_FILE_STAGED = "/sys/env_snapshot.staged";

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
    return 0;
}

} // system

} // particle

#endif // HAL_PLATFORM_ENV_VARS
