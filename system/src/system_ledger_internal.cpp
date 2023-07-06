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
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "system_ledger_internal.h"

#include "control/common.h" // FIXME: Move Protobuf utilities to another directory
#include "rtc_hal.h"

#include "file_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "common/ledger.pb.h" // Common definitions
#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define PB_LEDGER(_name) particle_ledger_##_name
#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

static_assert(LEDGER_SCOPE_UNKNOWN == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_UNKNOWN) &&
        LEDGER_SCOPE_DEVICE == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_DEVICE) &&
        LEDGER_SCOPE_PRODUCT == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_PRODUCT) &&
        LEDGER_SCOPE_OWNER == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_OWNER));

static_assert(LEDGER_SYNC_DIRECTION_UNKNOWN == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_UNKNOWN) &&
        LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_DEVICE_TO_CLOUD) &&
        LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_CLOUD_TO_DEVICE));

namespace particle {

using control::common::EncodedString;
using control::common::DecodedCString;
using fs::FsLock;

namespace system {

namespace {

/*
    The ledger directory is organized as follows:

    /usr/ledger/
    |
    +--- my-ledger/ - Files of the ledger called "my-ledger"
    |    |
    |    +--- staged/ - Fully written ledger data files
    |    |    +--- 0001
    |    |    +--- 0002
    |    |    +--- ...
    |    |
    |    +--- temp/ - Partially written ledger data files
    |    |    +--- 0003
    |    |    +--- 0004
    |    |    +--- ...
    |    |
    |    +--- current - Current ledger data
    |
    +--- ...

    When a ledger is opened for writing, a temporary file for the new ledger data is created in the
    "temp" directory. When writing is finished, if there are active readers of the current ledger
    data ("current"), the temporary file is moved to the "staged" directory, otherwise the file is
    moved to "current" to replace the current ledger data.

    When a ledger is opened for reading, it is checked whether the actual ledger data is stored in
    "current" or a file in the "staged" directory. The ledger data is then read from the respective
    file.

    The ledger instance tracks the readers of the current and staged data to ensure that the relevant
    files are modified or removed only when the last reader accessing them is closed. When all readers
    are closed, the most recent staged data is moved to "current" and all other files in "staged" are
    removed.
*/
const auto LEDGER_ROOT_DIR = "/usr/ledger";
const auto TEMP_DATA_DIR_NAME = "temp";
const auto STAGED_DATA_DIR_NAME = "staged";
const auto CURRENT_DATA_FILE_NAME = "current";

const unsigned DATA_FORMAT_VERSION = 1;

const size_t MAX_LEDGER_NAME_LEN = 32; // Must match the maximum length in ledger.proto
const size_t MAX_PATH_LEN = 127;

// Internal result codes
enum Result {
    RESULT_CURRENT_DATA_NOT_FOUND = 1,
    RESULT_STAGED_DATA_FOUND = 2
};

/*
    The layout of a ledger data file:

    Field     | Size      | Description
    ----------+-----------+------------
    version   | 4         | Format version number (unsigned integer)
    data_size | 4         | Size of the "data" field (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    data      | data_size | Contents of the ledger in an application-specific format
    info      | info_size | Protobuf-encoded ledger info (particle.firmware.LedgerInfo)

    All integer fields are encoded in little-endian byte order.
*/
struct LedgerDataHeader {
    uint32_t version;
    uint32_t dataSize;
    uint32_t infoSize;
} __attribute__((packed));

int readDataHeader(lfs_t* fs, lfs_file_t* file, size_t* dataSize = nullptr, size_t* infoSize = nullptr, int* version = nullptr) {
    LedgerDataHeader h = {};
    size_t n = CHECK_FS(lfs_file_read(fs, file, &h, sizeof(h)));
    if (n != sizeof(h)) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (version) {
        *version = littleEndianToNative(h.version);
    }
    if (dataSize) {
        *dataSize = littleEndianToNative(h.dataSize);
    }
    if (infoSize) {
        *infoSize = littleEndianToNative(h.infoSize);
    }
    return n;
}

int writeDataHeader(lfs_t* fs, lfs_file_t* file, size_t dataSize, size_t infoSize, int version = DATA_FORMAT_VERSION) {
    LedgerDataHeader h = {};
    h.version = nativeToLittleEndian(version);
    h.dataSize = nativeToLittleEndian(dataSize);
    h.infoSize = nativeToLittleEndian(infoSize);
    size_t n = CHECK_FS(lfs_file_write(fs, file, &h, sizeof(h)));
    return n;
}

int writeLedgerInfo(lfs_t* fs, lfs_file_t* file, const char* ledgerName, const LedgerInfo& info) {
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    size_t n = strlcpy(pbInfo.name, ledgerName, sizeof(pbInfo.name));
    if (n >= sizeof(pbInfo.name)) {
        return SYSTEM_ERROR_INTERNAL; // The name is longer than specified in ledger.proto
    }
    pbInfo.scope = static_cast<PB_LEDGER(LedgerScope)>(info.scope());
    pbInfo.sync_direction = static_cast<PB_LEDGER(SyncDirection)>(info.syncDirection());
    pbInfo.last_updated = info.lastUpdated();
    pbInfo.last_synced = info.lastSynced();
    pbInfo.sync_pending = info.syncPending();
    n = CHECK(encodeProtobufToFile(file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo));
    return n;
}

int formatLedgerPath(char* buf, size_t size, const char* ledgerName, const char* fmt = nullptr, ...) {
    // Format the prefix part of the path
    int n = snprintf(buf, size, "%s/%s/", LEDGER_ROOT_DIR, ledgerName);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    size_t pos = n;
    if (pos >= size) {
        return SYSTEM_ERROR_PATH_TOO_LONG;
    }
    if (fmt) {
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
            return SYSTEM_ERROR_PATH_TOO_LONG;
        }
    }
    return pos;
}

inline int getLedgerDirPath(char* buf, size_t size, const char* ledgerName) {
    CHECK(formatLedgerPath(buf, size, ledgerName));
    return 0;
}

inline int getTempDirPath(char* buf, size_t size, const char* ledgerName) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/", TEMP_DATA_DIR_NAME));
    return 0;
}

inline int getTempFilePath(char* buf, size_t size, const char* ledgerName, int seqNum) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/%04d", TEMP_DATA_DIR_NAME, seqNum));
    return 0;
}

inline int getTempFilePath(char* buf, size_t size, const char* ledgerName, const char* fileName) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/%s", TEMP_DATA_DIR_NAME, fileName));
    return 0;
}

inline int getStagedDirPath(char* buf, size_t size, const char* ledgerName) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/", STAGED_DATA_DIR_NAME));
    return 0;
}

inline int getStagedFilePath(char* buf, size_t size, const char* ledgerName, int seqNum) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/%04d", STAGED_DATA_DIR_NAME, seqNum));
    return 0;
}

inline int getStagedFilePath(char* buf, size_t size, const char* ledgerName, const char* fileName) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s/%s", STAGED_DATA_DIR_NAME, fileName));
    return 0;
}

inline int getCurrentFilePath(char* buf, size_t size, const char* ledgerName) {
    CHECK(formatLedgerPath(buf, size, ledgerName, "%s", CURRENT_DATA_FILE_NAME));
    return 0;
}

// Helper functions that transform LittleFS errors to a system error
inline int removeFile(lfs_t* fs, const char* path) {
    CHECK_FS(lfs_remove(fs, path));
    return 0;
}

inline int closeFile(lfs_t* fs, lfs_file_t* file) {
    CHECK_FS(lfs_file_close(fs, file));
    return 0;
}

inline int closeDir(lfs_t* fs, lfs_dir_t* dir) {
    CHECK_FS(lfs_dir_close(fs, dir));
    return 0;
}

bool isLedgerNameValid(const char* name) {
    size_t len = 0;
    char c = 0;
    while ((c = *name++)) {
        if (!(++len <= MAX_LEDGER_NAME_LEN && ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))) {
            return false;
        }
    }
    if (len == 0) {
        return false;
    }
    return true;
}

} // namespace

int LedgerManager::getLedger(const char* name, RefCountPtr<Ledger>& ledger) {
    std::lock_guard lock(mutex_);
    // Check if the requested ledger is already instantiated
    auto found = findLedger(name);
    if (found.second) {
        ledger = *found.first;
        return 0;
    }
    // Create a new instance
    auto lr = makeRefCountPtr<Ledger>();
    if (!lr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = lr->init(name);
    if (r < 0) {
        LOG(ERROR, "Failed to initialize ledger: %d", r);
        return r;
    }
    if (!ledgers_.insert(found.first, lr.get())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ledger = std::move(lr);
    return 0;
}

int LedgerManager::removeLedgerData(const char* name) {
    if (!*name) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto found = findLedger(name);
    if (found.second) {
        return SYSTEM_ERROR_LEDGER_IN_USE;
    }
    char path[MAX_PATH_LEN + 1];
    CHECK(getLedgerDirPath(path, sizeof(path), name));
    CHECK(rmrf(path));
    return 0;
}

int LedgerManager::removeAllData() {
    if (!ledgers_.isEmpty()) {
        return SYSTEM_ERROR_LEDGER_IN_USE;
    }
    CHECK(rmrf(LEDGER_ROOT_DIR));
    return 0;
}

std::pair<Vector<Ledger*>::ConstIterator, bool> LedgerManager::findLedger(const char* name) const {
    bool found = false;
    auto it = std::lower_bound(ledgers_.begin(), ledgers_.end(), name, [&found](Ledger* lr, const char* name) {
        auto r = std::strcmp(lr->name(), name);
        if (r == 0) {
            found = true;
        }
        return r < 0;
    });
    return std::make_pair(it, found);
}

void LedgerManager::addLedgerRef(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    ++ledger->refCount_;
}

void LedgerManager::releaseLedger(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    if (--ledger->refCount_ == 0) {
        ledgers_.removeOne(const_cast<Ledger*>(static_cast<const Ledger*>(ledger)));
        delete ledger;
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

Ledger::Ledger() :
        lastSeqNum_(0),
        stagedSeqNum_(0),
        curReaderCount_(0),
        stagedReaderCount_(0),
        stagedFileCount_(0),
        lastUpdated_(0),
        lastSynced_(0),
        dataSize_(0),
        syncPending_(false),
        syncCallback_(nullptr),
        destroyAppData_(nullptr),
        appData_(nullptr),
        scope_(LEDGER_SCOPE_UNKNOWN),
        syncDir_(LEDGER_SYNC_DIRECTION_UNKNOWN) {
}

Ledger::~Ledger() {
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
}

int Ledger::init(const char* name) {
    if (!isLedgerNameValid(name)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    name_ = name;
    if (!name_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    FsLock fs;
    int r = CHECK(loadLedgerInfo(fs.instance()));
    if (r == RESULT_CURRENT_DATA_NOT_FOUND) {
        // Initialize the ledger directory
        char path[MAX_PATH_LEN + 1];
        CHECK(getTempDirPath(path, sizeof(path), name_));
        CHECK(mkdirp(path));
        CHECK(getStagedDirPath(path, sizeof(path), name_));
        CHECK(mkdirp(path));
        // Create a file for the current ledger data
        CHECK(initCurrentData(fs.instance()));
    } else {
        // Clean up and recover staged data
        CHECK(removeTempData(fs.instance()));
        r = CHECK(flushStagedData(fs.instance()));
        if (r == RESULT_STAGED_DATA_FOUND) {
            // Reload the ledger info
            CHECK(loadLedgerInfo(fs.instance()));
        }
    }
    return 0;
}

int Ledger::initReader(LedgerReader& reader) {
    std::lock_guard lock(*this);
    CHECK(reader.init(stagedSeqNum_, this));
    if (stagedSeqNum_ > 0) {
        ++stagedReaderCount_;
    } else {
        ++curReaderCount_;
    }
    return 0;
}

int Ledger::initWriter(LedgerWriteSource src, LedgerWriter& writer) {
    std::lock_guard lock(*this);
    // It's allowed to write to a ledger while its sync direction is unknown
    if (src == LedgerWriteSource::USER && syncDir_ != LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD &&
            syncDir_ != LEDGER_SYNC_DIRECTION_UNKNOWN) {
        return SYSTEM_ERROR_LEDGER_READ_ONLY;
    }
    CHECK(writer.init(src, ++lastSeqNum_, this));
    return 0;
}

LedgerInfo Ledger::info() const {
    std::lock_guard lock(*this);
    return LedgerInfo()
            .scope(scope_)
            .syncDirection(syncDir_)
            .dataSize(dataSize_)
            .lastUpdated(lastUpdated_)
            .lastSynced(lastSynced_)
            .syncPending(syncPending_);
}

void Ledger::setAppData(void* data, ledger_destroy_app_data_callback destroy) {
    std::lock_guard lock(*this);
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
    appData_ = data;
    destroyAppData_ = destroy;
}

int Ledger::readerClosed(bool staged) {
    std::lock_guard lock(*this);
    if (staged) {
        --stagedReaderCount_;
    } else {
        --curReaderCount_;
    }
    // Check if there's staged data and if it can be flushed
    if (stagedSeqNum_ > 0 && curReaderCount_ == 0 && stagedReaderCount_ == 0) {
        // To reduce RAM usage, we don't count how many readers are open for each individual file,
        // which may result in creation of multiple files with outdated staged data. Typically,
        // however, there will be at most one file with staged data when the last reader is closed
        // as the application API never keeps ledger streams open for long. The only slow reader
        // is the system which streams ledger data to the server
        FsLock fs;
        if (stagedFileCount_ == 1) {
            // Fast track: there's one file with staged data. Move it to "current"
            char srcPath[MAX_PATH_LEN + 1];
            char destPath[MAX_PATH_LEN + 1];
            CHECK(getStagedFilePath(srcPath, sizeof(srcPath), name_, stagedSeqNum_));
            CHECK(getCurrentFilePath(destPath, sizeof(destPath), name_));
            CHECK_FS(lfs_rename(fs.instance(), srcPath, destPath));
        } else {
            // The staged directory contains outdated files. Clean those up and move the most recent
            // file to "current"
            CHECK(flushStagedData(fs.instance()));
        }
        stagedSeqNum_ = 0;
        stagedFileCount_ = 0;
    }
    return 0;
}

int Ledger::writerClosed(const LedgerInfo& info, LedgerWriteSource src, int tempSeqNum) {
    std::lock_guard lock(*this);
    FsLock fs;
    // Move the temporary file where appropriate
    bool newStagedFile = false;
    char srcPath[MAX_PATH_LEN + 1];
    char destPath[MAX_PATH_LEN + 1];
    CHECK(getTempFilePath(srcPath, sizeof(srcPath), name_, tempSeqNum));
    if (curReaderCount_ == 0 && stagedReaderCount_ == 0) {
        // Nobody is reading anything. Move to "current"
        CHECK(getCurrentFilePath(destPath, sizeof(destPath), name_));
    } else if (stagedSeqNum_ > 0 && stagedReaderCount_ == 0) {
        // There's staged data and nobody is reading it. Replace it
        CHECK(getStagedFilePath(destPath, sizeof(destPath), name_, stagedSeqNum_));
    } else {
        // Create a new file in "staged". Note that in this case, "current" can't be replaced with
        // the new data even if nobody is reading it, as otherwise it would get overwritten with
        // outdated data if the device resets before the staged directory is cleaned up
        CHECK(getStagedDirPath(destPath, sizeof(destPath), name_));
        newStagedFile = true;
    }
    CHECK_FS(lfs_rename(fs.instance(), srcPath, destPath));
    if (newStagedFile) {
        stagedSeqNum_ = tempSeqNum;
        ++stagedFileCount_;
    }
    updateLedgerInfo(info);
    return 0;
}

int Ledger::loadLedgerInfo(lfs_t* fs) {
    // Open the ledger file
    char path[MAX_PATH_LEN + 1];
    CHECK(getCurrentFilePath(path, sizeof(path), name_));
    lfs_file_t file = {};
    int r = lfs_file_open(fs, &file, path, LFS_O_RDONLY);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return RESULT_CURRENT_DATA_NOT_FOUND;
        }
        CHECK_FS(r); // Forward the error
    }
    SCOPE_GUARD({
        int r = closeFile(fs, &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the header
    size_t dataSize = 0;
    size_t infoSize = 0;
    int version = 0;
    CHECK(readDataHeader(fs, &file, &dataSize, &infoSize, &version));
    if (version != DATA_FORMAT_VERSION) {
        LOG(ERROR, "Unsupported version of ledger data format: %d", version);
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    size_t fileSize = CHECK_FS(lfs_file_size(fs, &file));
    if (fileSize != sizeof(LedgerDataHeader) + dataSize + infoSize) {
        LOG(ERROR, "Unexpected size of ledger data file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    // Skip the data section
    CHECK_FS(lfs_file_seek(fs, &file, dataSize, LFS_SEEK_CUR));
    // Read the info section
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    CHECK(decodeProtobufFromFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo, infoSize));
    if (strcmp(pbInfo.name, name_) != 0) {
        LOG(ERROR, "Unexpected ledger name");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    scope_ = static_cast<ledger_scope>(pbInfo.scope);
    syncDir_ = static_cast<ledger_sync_direction>(pbInfo.sync_direction);
    lastUpdated_ = pbInfo.last_updated;
    lastSynced_ = pbInfo.last_synced;
    syncPending_ = pbInfo.sync_pending;
    return 0;
}

void Ledger::updateLedgerInfo(const LedgerInfo& info) {
    if (info.hasScope()) {
        scope_ = info.scope();
    }
    if (info.hasSyncDirection()) {
        syncDir_ = info.syncDirection();
    }
    if (info.hasDataSize()) {
        dataSize_ = info.dataSize();
    }
    if (info.hasLastUpdated()) {
        lastUpdated_ = info.lastUpdated();
    }
    if (info.hasLastSynced()) {
        lastSynced_ = info.lastSynced();
    }
    if (info.hasSyncPending()) {
        syncPending_ = info.syncPending();
    }
}

int Ledger::initCurrentData(lfs_t* fs) {
    // Create a ledger file
    char path[MAX_PATH_LEN + 1];
    CHECK(getCurrentFilePath(path, sizeof(path), name_));
    lfs_file_t file = {};
    CHECK_FS(lfs_file_open(fs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL));
    SCOPE_GUARD({
        int r = closeFile(fs, &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Reserve space for the header
    CHECK_FS(lfs_file_seek(fs, &file, sizeof(LedgerDataHeader), LFS_SEEK_SET));
    // Write the info section
    size_t infoSize = CHECK(writeLedgerInfo(fs, &file, name_, info()));
    // Write the header
    CHECK_FS(lfs_file_seek(fs, &file, 0, LFS_SEEK_SET));
    CHECK(writeDataHeader(fs, &file, 0 /* dataSize */, infoSize));
    return 0;
}

int Ledger::flushStagedData(lfs_t* fs) {
    char path[MAX_PATH_LEN + 1];
    CHECK(getStagedDirPath(path, sizeof(path), name_));
    lfs_dir_t dir = {};
    CHECK_FS(lfs_dir_open(fs, &dir, path));
    SCOPE_GUARD({
        int r = closeDir(fs, &dir);
        if (r < 0) {
            LOG(ERROR, "Failed to close directory handle: %d", r);
        }
    });
    // Find the latest staged data
    char fileName[32] = {};
    int maxSeq = 0;
    lfs_info entry = {};
    int r = 0;
    while ((r = lfs_dir_read(fs, &dir, &entry)) == 1) {
        if (entry.type != LFS_TYPE_REG) {
            LOG(WARN, "Found unexpected entry in ledger directory");
            continue;
        }
        char* end = nullptr;
        int seq = strtol(entry.name, &end, 10);
        if (end != entry.name + std::strlen(entry.name)) {
            LOG(WARN, "Found unexpected entry in ledger directory");
            continue;
        }
        if (seq > maxSeq) {
            if (maxSeq > 0) {
                // Older staged files must be removed before the most recent file is moved to
                // "current", otherwise "current" would get overwritten with outdated data if
                // the device resets before the staged directory is cleaned up
                CHECK(getStagedFilePath(path, sizeof(path), name_, fileName));
                CHECK_FS(lfs_remove(fs, path));
            }
            size_t n = strlcpy(fileName, entry.name, sizeof(fileName));
            if (n >= sizeof(fileName)) {
                return SYSTEM_ERROR_INTERNAL;
            }
            maxSeq = seq;
        } else {
            CHECK(getStagedFilePath(path, sizeof(path), name_, entry.name));
            CHECK_FS(lfs_remove(fs, path));
        }
    }
    CHECK_FS(r);
    if (maxSeq > 0) {
        // Replace "current" with the found file
        char destPath[MAX_PATH_LEN + 1];
        CHECK(getCurrentFilePath(destPath, sizeof(destPath), name_));
        CHECK(getStagedFilePath(path, sizeof(path), name_, fileName));
        CHECK_FS(lfs_rename(fs, path, destPath));
        return RESULT_STAGED_DATA_FOUND;
    }
    return 0;
}

int Ledger::removeTempData(lfs_t* fs) {
    char path[MAX_PATH_LEN + 1];
    CHECK(getTempDirPath(path, sizeof(path), name_));
    lfs_dir_t dir = {};
    CHECK_FS(lfs_dir_open(fs, &dir, path));
    SCOPE_GUARD({
        int r = closeDir(fs, &dir);
        if (r < 0) {
            LOG(ERROR, "Failed to close directory handle: %d", r);
        }
    });
    lfs_info entry = {};
    int r = 0;
    while ((r = lfs_dir_read(fs, &dir, &entry)) == 1) {
        if (entry.type != LFS_TYPE_REG) {
            LOG(WARN, "Found unexpected entry in ledger directory");
            continue;
        }
        CHECK(getTempFilePath(path, sizeof(path), name_, entry.name));
        CHECK_FS(lfs_remove(fs, path));
    }
    CHECK_FS(r);
    return 0;
}

LedgerInfo& LedgerInfo::update(const LedgerInfo& info) {
    if (info.hasScope()) {
        scope_ = info.scope();
    }
    if (info.hasSyncDirection()) {
        syncDir_ = info.syncDirection();
    }
    if (info.hasDataSize()) {
        dataSize_ = info.dataSize();
    }
    if (info.hasLastUpdated()) {
        lastUpdated_ = info.lastUpdated();
    }
    if (info.hasLastSynced()) {
        lastSynced_ = info.lastSynced();
    }
    if (info.hasSyncPending()) {
        syncPending_ = info.syncPending();
    }
    return *this;
}

int LedgerReader::init(int stagedSeqNum, Ledger* ledger) {
    char path[MAX_PATH_LEN + 1];
    if (stagedSeqNum > 0) {
        // The most recent data is staged
        CHECK(getStagedFilePath(path, sizeof(path), ledger->name(), stagedSeqNum));
        staged_ = true;
    } else {
        CHECK(getCurrentFilePath(path, sizeof(path), ledger->name()));
    }
    // Open the respective file
    FsLock fs;
    CHECK_FS(lfs_file_open(fs.instance(), &file_, path, LFS_O_RDONLY));
    NAMED_SCOPE_GUARD(closeFileGuard, {
        int r = closeFile(fs.instance(), &file_);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the header. The header data has already been validated at this point
    CHECK(readDataHeader(fs.instance(), &file_, &dataSize_));
    ledger_ = ledger;
    open_ = true;
    closeFileGuard.dismiss();
    return 0;
}

int LedgerReader::read(char* data, size_t size) {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t bytesToRead = std::min(size, dataSize_ - dataOffs_);
    if (size > 0 && bytesToRead == 0) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    FsLock fs;
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file_, data, bytesToRead));
    if (n != bytesToRead) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT; // Unexpected end of file
    }
    dataOffs_ += n;
    return n;
}

int LedgerReader::close(bool /* discard */) {
    if (!open_) {
        return 0;
    }
    // Consider the reader closed regardless if any of the operations below fails
    open_ = false;
    FsLock fs;
    int result = closeFile(fs.instance(), &file_);
    if (result < 0) {
        LOG(ERROR, "Error while closing file: %d", result);
    }
    // Let the ledger flush any data
    int r = ledger_->readerClosed(staged_);
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger data: %d", r);
        return r;
    }
    return result;
}

int LedgerWriter::init(LedgerWriteSource src, int tempSeqNum, Ledger* ledger) {
    // Create a temporary file
    char path[MAX_PATH_LEN + 1];
    CHECK(getTempFilePath(path, sizeof(path), ledger->name(), tempSeqNum));
    FsLock fs;
    CHECK_FS(lfs_file_open(fs.instance(), &file_, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL));
    NAMED_SCOPE_GUARD(closeFileGuard, {
        int r = closeFile(fs.instance(), &file_);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Reserve space for the header
    CHECK_FS(lfs_file_seek(fs.instance(), &file_, sizeof(LedgerDataHeader), LFS_SEEK_SET));
    ledger_ = ledger;
    src_ = src;
    tempSeqNum_ = tempSeqNum;
    open_ = true;
    closeFileGuard.dismiss();
    return 0;
}

int LedgerWriter::write(const char* data, size_t size) {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (src_ == LedgerWriteSource::USER && dataSize_ + size > LEDGER_MAX_DATA_SIZE) {
        LOG(ERROR, "Ledger data is too long");
        return SYSTEM_ERROR_LEDGER_TOO_LARGE;
    }
    FsLock fs;
    size_t n = CHECK_FS(lfs_file_write(fs.instance(), &file_, data, size));
    if (n != size) {
        LOG(ERROR, "Unexpected number of bytes written");
        return SYSTEM_ERROR_FILESYSTEM;
    }
    dataSize_ += n;
    return n;
}

int LedgerWriter::close(bool discard) {
    if (!open_) {
        return 0;
    }
    // Consider the writer closed regardless if any of the operations below fails
    open_ = false;
    FsLock fs;
    if (discard) {
        // Remove the temporary file
        char path[MAX_PATH_LEN + 1];
        CHECK(getTempFilePath(path, sizeof(path), ledger_->name(), tempSeqNum_));
        int r = closeFile(fs.instance(), &file_);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
        CHECK_FS(lfs_remove(fs.instance(), path));
        return r;
    }
    NAMED_SCOPE_GUARD(removeFileGuard, {
        char path[MAX_PATH_LEN + 1];
        int r = getTempFilePath(path, sizeof(path), ledger_->name(), tempSeqNum_);
        if (r >= 0) {
            r = removeFile(fs.instance(), path);
        }
        if (r < 0) {
            LOG(ERROR, "Failed to remove file: %d", r);
        }
    });
    NAMED_SCOPE_GUARD(closeFileGuard, { // Will run before removeFileGuard
        int r = closeFile(fs.instance(), &file_);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Update the ledger info
    std::lock_guard lock(*ledger_);
    auto newInfo = ledger_->info().update(ledgerInfo_);
    newInfo.dataSize(dataSize_); // Can't be overridden
    if (!ledgerInfo_.hasLastUpdated()) {
        int64_t time = 0; // Time is unknown
        if (hal_rtc_time_is_valid(nullptr)) {
            timeval tv = {};
            int r = hal_rtc_get_time(&tv, nullptr);
            if (r < 0) {
                LOG(ERROR, "Failed to get current time: %d", r);
            } else {
                time = tv.tv_sec * 1000ll + tv.tv_usec / 1000;
            }
        }
        newInfo.lastUpdated(time);
    }
    if (!ledgerInfo_.hasSyncPending() && src_ == LedgerWriteSource::USER) {
        newInfo.syncPending(true);
    }
    // Write the info section
    size_t infoSize = CHECK(writeLedgerInfo(fs.instance(), &file_, ledger_->name(), newInfo));
    // Write the header
    CHECK_FS(lfs_file_seek(fs.instance(), &file_, 0, LFS_SEEK_SET));
    CHECK(writeDataHeader(fs.instance(), &file_, dataSize_, infoSize));
    closeFileGuard.dismiss();
    CHECK_FS(lfs_file_close(fs.instance(), &file_));
    // Let the ledger flush the data
    int r = ledger_->writerClosed(newInfo, src_, tempSeqNum_);
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger data: %d", r);
        return r;
    }
    removeFileGuard.dismiss();
    return 0;
}

} // namespace system

} // namespace particle
