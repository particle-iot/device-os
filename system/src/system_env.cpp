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

#include "system_env.h"

#if HAL_PLATFORM_ENV

#include <algorithm>
#include <charconv>
#include <limits>
#include <cstdlib>
#include <cstring>

#include <pb_decode.h>

#include "stream.h"
#include "file_util.h"
#include "nanopb_misc.h"
#include "logging.h"
#include "check.h"

#include "system/env_vars.pb.h"

#define PB_SYSTEM(_name) particle_system_##_name

namespace particle::system {

namespace {

const auto APP_FILE_CURRENT = "/sys/env_app";
const auto APP_FILE_STAGED = "/sys/env_app.staged";
const auto SNAPSHOT_FILE_CURRENT = "/sys/env_snapshot";
const auto SNAPSHOT_FILE_STAGED = "/sys/env_snapshot.staged";

const size_t MAX_VARS_FILE_SIZE = 16 * 1024;

class ValueStream: public InputStream {
public:
    ValueStream(fs::File& file, size_t offs, size_t size) :
            file_(file),
            offs_(offs),
            end_(offs + size) {
    }

    int read(char* data, size_t size) override {
        size_t n = CHECK(peek(data, size));
        offs_ += n;
        return n;
    }

    int peek(char* data, size_t size) override {
        size_t bytesToRead = std::min(size, end_ - offs_);
        if (!bytesToRead && size) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        fs::FsLock lock;
        CHECK(file_.seek(offs_));
        size_t bytesRead = CHECK(file_.read(data, bytesToRead));
        if (bytesRead != bytesToRead) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        return bytesRead;
    }

    int skip(size_t size) override {
        size_t bytesToSkip = std::min(size, end_ - offs_);
        if (!bytesToSkip && size) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        offs_ += bytesToSkip;
        return bytesToSkip;
    }

    int availForRead() override {
        return end_ - offs_;
    }

    int seek(size_t offs) override {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        if (offs_ == end_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    fs::File& file_;
    size_t offs_;
    size_t end_;
};

const char* stagedPathForAssetType(AssetType type) {
    switch (type) {
    case AssetType::ENV_VARS_APP:
        return APP_FILE_STAGED;
    case AssetType::ENV_VARS_SNAPSHOT:
        return SNAPSHOT_FILE_STAGED;
    default:
        return nullptr;
    }
}

int createEmptyFile(const char* path) {
    fs::File file;
    CHECK(file.open(path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    CHECK(file.close());
    return 0;
}

} // unnamed

Env::~Env() {
}

int Env::init() {
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
    return r; // 0 or SYSTEM_ENV_NEED_RESET
}

int Env::get(const char* name, char* buf, size_t bufSize) {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    const auto& var = it->second;
    CHECK(readValue(var, buf, bufSize));
    return var.valSize;
}

int Env::get(const char* name, CString& val) {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    const auto& var = it->second;
    auto buf = (char*)std::malloc(var.valSize + 1);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto v = CString::wrap(buf); // Takes ownership over the buffer
    CHECK(readValue(var, buf, var.valSize + 1));
    val = std::move(v);
    return var.valSize;
}

int Env::get(const char* name, int& val) {
    char buf[16] = {};
    size_t n = CHECK(get(name, buf, sizeof(buf)));
    if (n >= sizeof(buf)) {
        return SYSTEM_ERROR_ENV_INVALID_VALUE;
    }
    int v = 0;
    auto r = std::from_chars(buf, buf + n, v);
    if (r.ec != std::errc() || r.ptr != buf + n) {
        // Not a valid integer, or has trailing characters
        return SYSTEM_ERROR_ENV_INVALID_VALUE;
    }
    val = v;
    return 0;
}

int Env::get(const char* name, bool& val) {
    char buf[16] = {};
    size_t n = CHECK(get(name, buf, sizeof(buf)));
    if (n >= sizeof(buf)) {
        return SYSTEM_ERROR_ENV_INVALID_VALUE;
    }
    if (std::strcmp(buf, "true") == 0) {
        val = true;
    } else if (std::strcmp(buf, "false") == 0) {
        val = false;
    } else {
        return SYSTEM_ERROR_ENV_INVALID_VALUE;
    }
    return 0;
}

int Env::get(const char* name, std::unique_ptr<InputStream>& stream) {
    auto it = vars_.entries.find(name);
    if (it == vars_.entries.end()) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    const auto& var = it->second;
    auto& file = (var.src == VarSource::APP) ? appFile_ : snapshotFile_;
    std::unique_ptr<ValueStream> s(new(std::nothrow) ValueStream(file, var.valOffs, var.valSize));
    if (!s) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    stream = std::move(s);
    return 0;
}

int Env::clear() {
    if (vars_.entries.isEmpty()) {
        return 0;
    }
    fs::FsLock lock;
    // Create empty staged files for app and snapshot env vars
    CHECK(createEmptyFile(APP_FILE_STAGED));
    CHECK(createEmptyFile(SNAPSHOT_FILE_STAGED));
    return SYSTEM_ENV_NEED_RESET;
}

int Env::updateAsset(const Asset& asset, InputStream& data) {
    fs::FsLock lock;

    const char* path = stagedPathForAssetType(asset.type());
    if (!path) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(saveToFile(data, path));
    return 0;
}

int Env::removeAsset(const Asset& asset) {
    fs::FsLock lock;

    const char* path = stagedPathForAssetType(asset.type());
    if (!path) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Create an empty staged file
    CHECK(createEmptyFile(path));
    return 0;
}

AssetStorageOption Env::storageOptionForAsset(const Asset& asset) {
    if (asset.type() == AssetType::ENV_VARS_SNAPSHOT) {
        // Apps don't depend on snapshot env vars so those are not stored as normal assets
        return AssetStorageOption::DONT_STORE;
    }
    return AssetStorageOption::DEFAULT;
}

Env& Env::instance() {
    static Env env;
    return env;
}

int Env::updateBootloaderVars() {
    // TODO: Check if any variables used by the bootloader changed (none are defined as of now),
    // apply the changes and return SYSTEM_ENV_NEED_RESET
    return 0;
}

int Env::readValue(const VarEntry& var, char* buf, size_t bufSize) {
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

int Env::loadVars(bool tryStaged, fs::File& appFile, fs::File& snapshotFile, Vars& vars, bool& hasStaged) {
    // Load the variables bundled with the app
    CHECK(loadVarsForSource(tryStaged, VarSource::APP, appFile, vars, hasStaged));

    // Override with the variables from the snapshot
    CHECK(loadVarsForSource(tryStaged, VarSource::SNAPSHOT, snapshotFile, vars, hasStaged));
    return 0;
}

int Env::loadVarsForSource(bool tryStaged, VarSource src, fs::File& file, Vars& vars, bool& hasStaged) {
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
            int r2 = fs::remove(path);
            if (r2 < 0) {
                LOG(ERROR, "Error while removing %s: %d", path, r2);
            }
            return r;
        }
    }

    auto path = (src == VarSource::APP) ? APP_FILE_CURRENT : SNAPSHOT_FILE_CURRENT;
    int r = loadVarsFile(path, src, file, vars);
    if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
        LOG(ERROR, "Error while reading %s: %d", path, r);
        return r;
    }
    return 0;
}

int Env::loadVarsFile(const char* path, VarSource src, fs::File& file, Vars& vars) {
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

int Env::readVars(VarSource src, fs::File& file, Vars& vars) {
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

} // particle::system

using namespace particle::system;

int system_get_env(const char* name, char* buf, size_t bufSize, void* reserved) {
    return Env::instance().get(name, buf, bufSize);
}

int system_get_env_int(const char* name, int* val, void* reserved) {
    return Env::instance().get(name, *val);
}

int system_get_env_bool(const char* name, bool* val, void* reserved) {
    return Env::instance().get(name, *val);
}

int system_list_env(system_list_env_fn fn, void* arg, char* buf, size_t buf_size, void* reserved) {
    if (fn) {
        int r = Env::instance().forEach(buf, buf_size, [fn, arg](const auto& var, const char* val) -> int {
            return fn(var.name, val, var.size, arg);
        });
        if (r < 0) {
            return r;
        }
    }
    return Env::instance().count();
}

int system_clear_env(void* reserved) {
    return Env::instance().clear();
}

#endif // HAL_PLATFORM_ENV
