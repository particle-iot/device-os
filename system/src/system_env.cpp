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

const size_t MAX_ENV_FILE_SIZE = 48 * 1024;

const size_t HASH_PRIME_SIZES[] = { 5, 11, 23, 37, 53, 97, 127, 193, 283, 389, 577, 769, 1153 };
const unsigned HASH_LOAD_FACTOR_N = 1;
const unsigned HASH_LOAD_FACTOR_D = 1;

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

uint32_t strHash(const char* s) { // djb2
    uint32_t h = 5381;
    for (; *s; ++s) {
        h = ((h << 5) + h) + (unsigned char)*s;
    }
    return h;
}

} // unnamed

Env::Var::Var() :
        next(nullptr),
        hash(0),
        valOffs(0),
        valSize(0),
        nameOffs(0),
        nameSize(0),
        src(0) {
}

Env::Vars::Vars() :
        firstVar(nullptr),
        varCount(0) {
}

Env::Vars::Vars(Vars&& vars) :
        snapshotFile(std::move(vars.snapshotFile)),
        appFile(std::move(vars.appFile)),
        snapshotHash(std::move(vars.snapshotHash)),
        buckets(std::move(vars.buckets)),
        firstVar(vars.firstVar),
        varCount(vars.varCount) {
    vars.firstVar = nullptr;
    vars.varCount = 0;
}

Env::Vars::~Vars() {
    auto v = firstVar;
    while (v) {
        auto next = v->next;
        delete v;
        v = next;
    }
}

int Env::Vars::add(const char* name, Var newVar) {
    auto hash = strHash(name);
    size_t bucketCount = buckets.size();
    if (bucketCount) {
        char nameBuf[MAX_ENV_NAME_LEN + 1];
        auto var = buckets.at(hash % bucketCount);
        while (var) {
            CHECK(readName(*var, nameBuf, sizeof(nameBuf)));
            if (std::strcmp(nameBuf, name) == 0) {
                // Replace the existing entry
                newVar.hash = hash;
                newVar.next = var->next;
                *var = std::move(newVar);
                return 0;
            }
            var = var->next;
            if (!var || var->hash != hash) {
                break;
            }
        }
    }
    if (!bucketCount || (varCount + 1) * HASH_LOAD_FACTOR_D > bucketCount * HASH_LOAD_FACTOR_N) {
        // Rehash
        size_t newBucketCount = 0;
        for (unsigned i = 0; i < sizeof(HASH_PRIME_SIZES) / sizeof(HASH_PRIME_SIZES[0]); ++i) {
            if (HASH_PRIME_SIZES[i] > bucketCount) {
                newBucketCount = HASH_PRIME_SIZES[i];
                break;
            }
        }
        if (!newBucketCount) {
            newBucketCount = bucketCount * 2 - 1;
        }
        Vector<Var*> newBuckets;
        if (!newBuckets.resize(newBucketCount)) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        auto var = firstVar;
        while (var) {
            auto next = var->next;
            auto index = var->hash % newBucketCount;
            auto var2 = newBuckets.at(index);
            if (var2) {
                var->next = var2;
            }
            newBuckets[index] = var;
            var = next;
        }
        buckets = std::move(newBuckets);
        bucketCount = newBucketCount;
    }
    // Add a new entry
    std::unique_ptr<Var> var(new(std::nothrow) Var(newVar));
    if (!var) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto index = hash % bucketCount;
    auto var2 = buckets.at(index);
    if (var2) {
        var->next = var2;
    }
    buckets[index] = var.release();
    return 0;
}

const Env::Var* Env::Vars::find(const char* name) {
    if (buckets.isEmpty()) {
        return nullptr;
    }
    auto hash = strHash(name);
    auto var = buckets.at(hash % buckets.size());
    if (!var) {
        return nullptr;
    }
    char nameBuf[MAX_ENV_NAME_LEN + 1];
    for (;;) {
        int r = readName(*var, nameBuf, sizeof(nameBuf));
        if (r < 0) {
            return nullptr;
        }
        if (std::strcmp(nameBuf, name) == 0) {
            return var;
        }
        var = var->next;
        if (!var || var->hash != hash) {
            return nullptr;
        }
    }
}

int Env::Vars::readValue(const Var& var, char* buf, size_t bufSize) {
    if (!bufSize) {
        return 0;
    }
    fs::FsLock lock;
    auto& file = (var.src == VarSource::APP) ? appFile : snapshotFile;
    CHECK(file.seek(var.valOffs));
    size_t bytesToRead = std::min<size_t>(var.valSize, bufSize - 1);
    size_t bytesRead = CHECK(file.read(buf, bytesToRead));
    if (bytesRead != bytesToRead) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    buf[bytesRead] = '\0';
    return var.valSize;
}

int Env::Vars::readName(const Var& var, char* buf, size_t bufSize) {
    if (!bufSize) {
        return 0;
    }
    fs::FsLock lock;
    auto& file = (var.src == VarSource::APP) ? appFile : snapshotFile;
    CHECK(file.seek(var.nameOffs));
    size_t bytesToRead = std::min<size_t>(var.nameSize, bufSize - 1);
    size_t bytesRead = CHECK(file.read(buf, bytesToRead));
    if (bytesRead != bytesToRead) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    buf[bytesRead] = '\0';
    return var.nameSize;
}

Env::Vars& Env::Vars::operator=(Vars&& vars) {
    snapshotFile = std::move(vars.snapshotFile);
    appFile = std::move(vars.appFile);
    snapshotHash = std::move(vars.snapshotHash);
    buckets = std::move(vars.buckets);
    firstVar = vars.firstVar;
    varCount = vars.varCount;
    vars.firstVar = nullptr;
    vars.varCount = 0;
    return *this;
}

Env::~Env() {
}

int Env::init() {
    fs::FsLock lock;
    CHECK(fs::mount());

    Vars vars;

    bool hasStaged = false;
    int r = loadVars(vars, hasStaged, true /* tryStaged */);
    if (r < 0) {
        if (hasStaged) {
            // Ignore the staged files as a fallback
            vars = Vars();
            hasStaged = false;
            r = loadVars(vars, hasStaged, false /* tryStaged */);
        }
        if (r < 0) {
            LOG(ERROR, "Error while loading env vars: %d", r);
            return r;
        }
    }

    // Clean up empty files
    if (vars.appFile.isOpen() && vars.appFile.size() == 0) {
        vars.appFile.close();
        fs::remove(APP_FILE_CURRENT);
        fs::remove(APP_FILE_STAGED);
    }
    if (vars.snapshotFile.isOpen() && vars.snapshotFile.size() == 0) {
        vars.snapshotFile.close();
        fs::remove(SNAPSHOT_FILE_CURRENT);
        fs::remove(SNAPSHOT_FILE_STAGED);
    }

    vars_ = std::move(vars);

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
    auto var = vars_.find(name);
    if (!var) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    CHECK(vars_.readValue(*var, buf, bufSize));
    return var->valSize;
}

int Env::get(const char* name, CString& val) {
    auto var = vars_.find(name);
    if (!var) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    auto buf = (char*)std::malloc(var->valSize + 1);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto v = CString::wrap(buf); // Takes ownership over the buffer
    CHECK(vars_.readValue(*var, buf, var->valSize + 1));
    val = std::move(v);
    return var->valSize;
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
    auto var = vars_.find(name);
    if (!var) {
        return SYSTEM_ERROR_ENV_NOT_FOUND;
    }
    auto& file = (var->src == VarSource::APP) ? vars_.appFile : vars_.snapshotFile;
    std::unique_ptr<ValueStream> s(new(std::nothrow) ValueStream(file, var->valOffs, var->valSize));
    if (!s) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    stream = std::move(s);
    return 0;
}

int Env::clear() {
    if (!vars_.varCount) {
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

int Env::loadVars(Vars& vars, bool& hasStaged, bool tryStaged) {
    // Load the variables bundled with the app
    CHECK(loadVarsForSource(vars, hasStaged, tryStaged, VarSource::APP));

    // Override with the variables from the snapshot
    CHECK(loadVarsForSource(vars, hasStaged, tryStaged, VarSource::SNAPSHOT));
    return 0;
}

int Env::loadVarsForSource(Vars& vars, bool& hasStaged, bool tryStaged, VarSource src) {
    auto& file = (src == VarSource::APP) ? vars.appFile : vars.snapshotFile;

    if (tryStaged) {
        auto path = (src == VarSource::APP) ? APP_FILE_STAGED : SNAPSHOT_FILE_STAGED;
        int r = loadVarsFile(vars, file, path, src);
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
    int r = loadVarsFile(vars, file, path, src);
    if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
        LOG(ERROR, "Error while reading %s: %d", path, r);
        return r;
    }
    return 0;
}

int Env::loadVarsFile(Vars& vars, fs::File& file, const char* path, VarSource src) {
    fs::File f;
    CHECK(f.open(path, LFS_O_RDONLY));
    size_t size = CHECK(f.size());
    if (size > MAX_ENV_FILE_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    // As a special case, allow an app/snapshot file to be empty so that flashing it would clean up
    // the corresponding file on the device (see `init()`)
    if (size > 0) {
        CHECK(readVars(vars, f, src));
    }
    file = std::move(f); // Keep the file open
    return 0;
}

int Env::readVars(Vars& vars, fs::File& file, VarSource src) {
    pb_istream_t stream = {};
    CHECK(pb_istream_from_file(&stream, file.handle(), CHECK(file.size()), nullptr /* reserved */));

    char nameBuf[MAX_ENV_NAME_LEN + 1];

    struct DecodeContext {
        Vars& vars;
        fs::File& file;
        VarSource src;
        char* name; // Last read variable name
        size_t nameOffs; // Offset/size of the last read variable name
        size_t nameSize;
        size_t valOffs; // Offset/size of the last read variable value
        size_t valSize;
        int error;
    };
    DecodeContext d = {
        .vars = vars,
        .file = file,
        .src = src,
        .name = nameBuf,
        .nameOffs = 0,
        .nameSize = 0,
        .valOffs = 0,
        .valSize = 0,
        .error = 0
    };

    PB_SYSTEM(EnvVars) pbVars = {};
    pbVars.vars.arg = &d;
    pbVars.vars.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;

        PB_SYSTEM(EnvVars_Var) pbVar = {};
        pbVar.name.arg = d;
        pbVar.name.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
            auto d = (DecodeContext*)*arg;

            int r = d->file.tell();
            if (r < 0) {
                d->error = r;
                return false;
            }
            d->nameOffs = r;
            d->nameSize = stream->bytes_left;
            if (!d->nameSize || d->nameSize > MAX_ENV_NAME_LEN) {
                d->error = SYSTEM_ERROR_BAD_DATA;
                return false;
            }

            bool ok = pb_read(stream, (pb_byte_t*)d->name, d->nameSize);
            if (!ok) {
                return false;
            }
            d->name[d->nameSize] = '\0';
            return true;
        };

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

        Var var;
        var.valOffs = d->valOffs;
        var.valSize = d->valSize;
        var.nameOffs = d->nameOffs;
        var.nameSize = d->nameSize;
        var.src = d->src;

        int r = d->vars.add(d->name, std::move(var));
        if (r < 0) {
            d->error = r;
            return false;
        }
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
