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

#include "control/common.h" // FIXME: Move to another directory

#include "filesystem.h"
#include "file_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "common/ledger.pb.h" // Common definitions
#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define DATA_FORMAT_VERSION 1

/*
    The ledger directory is organized as follows:

    /sys/ledger/
    |
    +--- device/ - Device ledgers
    |    +--- default/ - Files of the default device ledger
    |         +--- ledger.data - Ledger info
    |         +--- pages/ - Page files
    |              +--- sensors.data - Contents of the page called "sensors"
    |              +--- ...
    |
    +--- product/ - Product ledgers
    |    +--- config/ - Files of the ledger called "config"
    |    |    +--- ...
    |    +--- product_info/ - Files of the ledger called "product_info"
    |    |    +--- ...
    |    +--- ...
    |
    +--- owner/ - Owner ledgers
         +--- ...
*/
#define LEDGER_ROOT_DIR "/sys/ledger" // The root "sys" directory is expected to exist
#define DEVICE_SCOPE_DIR_NAME "device"
#define PRODUCT_SCOPE_DIR_NAME "product"
#define OWNER_SCOPE_DIR_NAME "owner"
#define LEDGER_DATA_FILE_NAME "ledger.data"
#define PAGES_DIR_NAME "pages"
#define PAGE_DATA_EXTENSION ".data"
#define DEFAULT_LEDGER_NAME "default"

#define PB_LEDGER(_name) particle_ledger_##_name
#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

static_assert(LEDGER_SCOPE_INVALID == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_INVALID) &&
        LEDGER_SCOPE_DEVICE == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_DEVICE) &&
        LEDGER_SCOPE_PRODUCT == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_PRODUCT) &&
        LEDGER_SCOPE_OWNER == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_OWNER));

namespace particle {

using control::common::EncodedString;
using control::common::DecodedCString;
using fs::FsLock;

namespace system {

namespace {

// Internal result codes
enum Result {
    RESULT_LEDGER_FILE_NOT_FOUND = 1
};

/*
    The layout of a ledger data file:

    field     | size      | description
    -----------------------------------
    version   | 4         | Format version number (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    info      | info_size | Protobuf-encoded ledger info (particle.firmware.LedgerInfo)

    All integer fields are encoded in little-endian byte order.
*/
struct LedgerDataHeader {
    uint32_t version;
    uint32_t infoSize;
} __attribute__((packed));

/*
    The layout of a page data file:

    field     | size      | description
    -----------------------------------
    version   | 4         | Format version number (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    data_size | 4         | Size of the "data" field (unsigned integer)
    info      | info_size | Protobuf-encoded page info (particle.firmware.LedgerPageInfo)
    data      | data_size | MessagePack-encoded page contents

    All integer fields are encoded in little-endian byte order.
*/
struct PageDataHeader {
    uint32_t version;
    uint32_t infoSize;
    uint32_t dataSize;
} __attribute__((packed));

int formatLedgerPath(char* buf, size_t size, const char* ledgerName, ledger_scope scope, const char* fmt = "", ...) {
    // Format the prefix part of the path
    const char* scopeName = nullptr;
    switch (scope) {
    case LEDGER_SCOPE_DEVICE: scopeName = DEVICE_SCOPE_DIR_NAME; break;
    case LEDGER_SCOPE_PRODUCT: scopeName = PRODUCT_SCOPE_DIR_NAME; break;
    case LEDGER_SCOPE_OWNER: scopeName = OWNER_SCOPE_DIR_NAME; break;
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int n = snprintf(buf, size, "%s/%s/%s/", LEDGER_ROOT_DIR, scopeName, ledgerName);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    size_t pos = n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    // Format the rest of the path
    va_list args;
    va_start(args, fmt);
    n = vsnprintf(buf + pos, size - pos, fmt, args);
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
        if (!s && src.at(i)) {
            return SYSTEM_ERROR_NO_MEMORY;
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

int Ledger::init(const char* name, ledger_scope scope, int apiVersion) {
    name_ = name;
    if (!name_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    scope_ = scope;
    apiVersion_ = apiVersion;
    int r = CHECK(loadLedgerInfo());
    if (r == RESULT_LEDGER_FILE_NOT_FOUND && scope_ == LEDGER_SCOPE_DEVICE) {
        // Create a directory for the device ledger
        CHECK(saveLedgerInfo());
    }
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

void Ledger::addRef() {
    // The ledger's reference counter is managed by LedgerManager
    LedgerManager::instance()->addLedgerRef(this);
}

void Ledger::release() {
    LedgerManager::instance()->releaseLedger(this);
}

int Ledger::loadLedgerInfo() {
    FsLock fs;
    // Open the ledger file
    char path[LFS_NAME_MAX + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), name_, scope_, "%s", LEDGER_DATA_FILE_NAME));
    lfs_file_t file = {};
    int r = lfs_file_open(fs.instance(), &file, path, LFS_O_RDONLY);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return RESULT_LEDGER_FILE_NOT_FOUND;
        }
        CHECK_FS(r); // Forward the error
    }
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the header
    LedgerDataHeader h = {};
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file, &h, sizeof(h)));
    if (n != sizeof(h)) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    h.version = littleEndianToNative(h.version);
    h.infoSize = littleEndianToNative(h.infoSize);
    if (h.version != DATA_FORMAT_VERSION) {
        LOG(ERROR, "Unsupported version of ledger data format: %u", (unsigned)h.version);
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    // Read and validate the ledger info
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    DecodedCString pbName(&pbInfo.name);
    CHECK(decodeMessageFromFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo, h.infoSize));
    if (strcmp(name_, pbName.data) != 0) {
        LOG(ERROR, "Unexpected ledger name");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (static_cast<ledger_scope>(pbInfo.scope) != scope_) {
        LOG(ERROR, "Unexpected ledger scope");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    return 0;
}

int Ledger::saveLedgerInfo() {
    FsLock fs;
    // Ensure the ledger directory exists
    char path[LFS_NAME_MAX + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), name_, scope_));
    int r = lfs_mkdir(fs.instance(), path);
    if (r < 0 && r != LFS_ERR_EXIST) {
        CHECK_FS(r); // Forward the error
    }
    // Create the ledger file
    CHECK(formatLedgerPath(path, sizeof(path), name_, scope_, "%s", LEDGER_DATA_FILE_NAME));
    lfs_file_t file = {};
    CHECK_FS(lfs_file_open(fs.instance(), &file, path, LFS_O_WRONLY | LFS_O_TRUNC));
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Write the header
    LedgerDataHeader h = {};
    h.version = nativeToLittleEndian(DATA_FORMAT_VERSION);
    CHECK_FS(lfs_file_write(fs.instance(), &file, &h, sizeof(h)));
    // Write the ledger info
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    EncodedString pbName(&pbInfo.name, name_, strlen(name_));
    pbInfo.scope = static_cast<PB_LEDGER(LedgerScope)>(scope_);
    CHECK(encodeMessageToFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo));
    return 0;
}

int LedgerManager::init() {
    // Create ledger directories
    FsLock fs;
    lfs_info entry = {};
    int r = lfs_stat(fs.instance(), LEDGER_ROOT_DIR, &entry);
    if (r == LFS_ERR_NOENT) {
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR));
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR "/" DEVICE_SCOPE_DIR_NAME));
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR "/" PRODUCT_SCOPE_DIR_NAME));
        CHECK_FS(lfs_mkdir(fs.instance(), LEDGER_ROOT_DIR "/" OWNER_SCOPE_DIR_NAME));
    } else if (r < 0 || entry.type != LFS_TYPE_DIR) {
        LOG(ERROR, "%s is not a directory", LEDGER_ROOT_DIR);
        return (r < 0) ? filesystem_to_system_error(r) : SYSTEM_ERROR_INVALID_STATE;
    }
    inited_ = true;
    return 0;
}

int LedgerManager::getLedger(const char* name, ledger_scope scope, int apiVersion, RefCountPtr<Ledger>& ledger) {
    std::lock_guard lock(mutex_);
    if (!name) {
        name = DEFAULT_LEDGER_NAME;
    } else {
        if (!*name) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (scope == LEDGER_SCOPE_DEVICE && strcmp(name, DEFAULT_LEDGER_NAME) != 0) {
            // As of now, only one device ledger is supported per device
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    if (scope == LEDGER_SCOPE_INVALID) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Check if the requested ledger is already instantiated
    for (Ledger* lr: ledgerInstances_) { // TODO: Use binary search
        if (strcmp(lr->name(), name) == 0) {
            ledger = lr;
            return 0;
        }
    }
    // Create a new instance
    auto lr = makeRefCountPtr<Ledger>();
    if (!lr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = lr->init(name, scope, apiVersion);
    if (r < 0) {
        LOG(ERROR, "Failed to initialize ledger: %d", r);
        return r;
    }
    if (!ledgerInstances_.append(lr.get())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ledger = std::move(lr);
    return 0;
}

void LedgerManager::addLedgerRef(Ledger* ledger) {
    std::lock_guard lock(mutex_);
    ++ledger->refCount_;
}

void LedgerManager::releaseLedger(Ledger* ledger) {
    std::lock_guard lock(mutex_);
    if (!--ledger->refCount_) {
        ledgerInstances_.removeOne(ledger);
        delete ledger;
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

} // namespace system

} // namespace particle
