/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "system_ledger_internal.h"

#include "filesystem.h"
#include "file_util.h"
#include "scope_guard.h"
#include "check.h"
#include "system_error.h"

#include "common/ledger.pb.h" // Common definitions
#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define DATA_FORMAT_VERSION 1

#define LEDGER_ROOT_DIR "/sys/ledger"
#define LOCAL_LEDGER_DIR_NAME "local"
#define SHARED_LEDGER_DIR_NAME "shared"
#define PAGES_DIR_NAME "pages"
#define LEDGER_DATA_FILE_NAME "ledger.data"
#define PAGE_DATA_EXTENSION ".data"

#define PB_LEDGER(_name) particle_ledger_##_name
#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

namespace particle {

using fs::FsLock;

namespace system {

namespace {

int formatLedgerPath(char* buf, size_t size, const char* name, const char* fmt, ...) {
    size_t pos = 0;
    // Format the path to the ledger files
    if (name) {
        // Product or organization ledger
        int n = snprintf(buf, size, LEDGER_ROOT_DIR "/" SHARED_LEDGER_DIR_NAME "/%s/", name);
        if (n < 0) {
            return SYSTEM_ERROR_INTERNAL;
        }
        pos = n;
    } else {
        // Device ledger
        pos = strlcpy(buf, LEDGER_ROOT_DIR "/" LOCAL_LEDGER_DIR_NAME "/", size);
    }
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    // Format the suffix part of the path
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf + pos, size - pos, fmt, args);
    va_end(args);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    pos += n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    return pos;
}

int copyStringVector(const Vector<CString>& src, Vector<CString>& dest) {
    dest.clear();
    if (!dest.reserve(src.size())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    for (int i = 0; i < src.size(); ++i) {
        CString s = src.at(i);
        if (!!s != !!src.at(i)) {
            return SYSTEM_ERROR_NO_MEMORY; // strdup() failed
        }
        dest.append(std::move(s));
    }
    return 0;
}

/*
int compareStrings(const CString& s1, const CString& s2) {
    if (!s1 || !s2) {
        return !!s1 - !!s2; // bool is implicitly converted to int in arithmetic operations
    }
    return strcmp(s1, s2);
}
*/
} // namespace

Ledger::~Ledger() {
    // Destroy the application data
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
}

int Ledger::init(const char* name, int apiVersion) {
    if (name) {
        name_ = name;
        if (!name_) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    apiVersion_ = apiVersion;
    CHECK(loadLedgerInfo());
    return 0;
}

int Ledger::getLinkedPageNames(Vector<CString>& names) const {
    std::lock_guard lock(mutex_);
    CHECK(copyStringVector(linkedPageNames_, names));
    return 0;
}

void Ledger::setCallbacks(ledger_page_sync_callback pageSyncCallback, void* pageSyncArg,
        ledger_page_change_callback pageChangeCallback, void* pageChangeArg) {
    std::lock_guard lock(mutex_);
    pageSyncCallback_ = pageSyncCallback;
    pageSyncArg_ = pageSyncArg;
    pageChangeCallback_ = pageChangeCallback;
    pageChangeArg_ = pageChangeArg;
}

void Ledger::setAppData(void* data, ledger_destroy_app_data_callback destroy) {
    std::lock_guard lock(mutex_);
    appData_ = data;
    destroyAppData_ = destroy;
}

void* Ledger::appData() const {
    std::lock_guard lock(mutex_);
    return appData_;
}

ledger_scope Ledger::scope() const {
    std::lock_guard lock(mutex_);
    return scope_;
}

void Ledger::destroy() {
    LedgerManager::instance()->disposeLedger(this);
}

int Ledger::loadLedgerInfo() {
    FsLock fs;
    // Open the file with the ledger data
    char path[LFS_NAME_MAX + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), name_, LEDGER_DATA_FILE_NAME));
    lfs_file_t file = {};
    CHECK_FS(lfs_file_open(fs.instance(), &file, path, LFS_O_RDONLY));
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the version number
    uint32_t ver = 0;
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file, &ver, sizeof(ver)));
    if (n != sizeof(ver)) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (ver != DATA_FORMAT_VERSION) {
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    CHECK(decodeMessageFromFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo));
    return 0;
}

int LedgerManager::init() {
    // Create ledger directories
    FsLock fs;
    lfs_info entry = {};
    int r = lfs_stat(fs.instance(), LEDGER_ROOT_DIR, &entry);
    if (r == LFS_ERR_NOENT) {
        // /sys should already exist at this point
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR));
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR "/" LOCAL_LEDGER_DIR_NAME));
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR "/" SHARED_LEDGER_DIR_NAME));
    } else if (r < 0 || entry.type != LFS_TYPE_DIR) {
        LOG(ERROR, "%s is not a directory", LEDGER_ROOT_DIR);
        return (r < 0) ? filesystem_to_system_error(r) : SYSTEM_ERROR_INVALID_STATE;
    }
    inited_ = true;
    return 0;
}

int LedgerManager::initLedger(const char* name, int apiVersion, RefCountPtr<Ledger>& ledger) {
    std::lock_guard lock(mutex_);
    // Check if the requested ledger is already instantiated
    if (name) {
        for (Ledger* lr: sharedLedgers_) {
            if (strcmp(lr->name(), name) == 0) {
                ledger = lr;
                return 0;
            }
        }
    } else if (devLedger_) {
        ledger = devLedger_;
        return 0;
    }
    // Create a new instance
    RefCountPtr<Ledger> lr(new Ledger());
    if (!lr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(lr->init(name, apiVersion));
    if (!name) {
        devLedger_ = lr.get();
    } else if (!sharedLedgers_.append(lr.get())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ledger = std::move(lr);
    return 0;
}

void LedgerManager::disposeLedger(Ledger* ledger) {
    // FIXME: Use the same mutex to guard the reference counter and the list of instantiated ledgers
    std::lock_guard lock(mutex_);
    if (!ledger->refCount() && sharedLedgers_.removeOne(ledger)) {
        delete ledger;
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

} // namespace system

} // namespace particle
