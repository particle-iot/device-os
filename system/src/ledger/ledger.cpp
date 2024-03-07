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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include "logging.h"
LOG_SOURCE_CATEGORY("system.ledger");

#include <algorithm>
#include <cstring>
#include <cassert>

#include "ledger.h"
#include "ledger_manager.h"
#include "ledger_util.h"

#include "file_util.h"
#include "time_util.h"
#include "endian_util.h"
#include "str_compat.h"
#include "scope_guard.h"
#include "check.h"

#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define PB_LEDGER(_name) particle_cloud_ledger_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

namespace particle {

using fs::FsLock;

namespace system {

static_assert(LEDGER_SCOPE_UNKNOWN == (int)PB_LEDGER(ScopeType_SCOPE_TYPE_UNKNOWN) &&
        LEDGER_SCOPE_DEVICE == (int)PB_LEDGER(ScopeType_SCOPE_TYPE_DEVICE) &&
        LEDGER_SCOPE_PRODUCT == (int)PB_LEDGER(ScopeType_SCOPE_TYPE_PRODUCT) &&
        LEDGER_SCOPE_OWNER == (int)PB_LEDGER(ScopeType_SCOPE_TYPE_OWNER));

static_assert(LEDGER_SYNC_DIRECTION_UNKNOWN == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_UNKNOWN) &&
        LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_DEVICE_TO_CLOUD) &&
        LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE == (int)PB_LEDGER(SyncDirection_SYNC_DIRECTION_CLOUD_TO_DEVICE));

static_assert(LEDGER_MAX_NAME_LENGTH + 1 == sizeof(PB_INTERNAL(LedgerInfo::name)) &&
        LEDGER_MAX_NAME_LENGTH + 1 == sizeof(PB_LEDGER(GetInfoResponse_Ledger::name)));

static_assert(MAX_LEDGER_SCOPE_ID_SIZE == sizeof(PB_INTERNAL(LedgerInfo_scope_id_t::bytes)) &&
        MAX_LEDGER_SCOPE_ID_SIZE == sizeof(PB_LEDGER(GetInfoResponse_Ledger_scope_id_t::bytes)));

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
    "temp" directory. When writing is finished, if there are active readers of the ledger data, the
    temporary file is moved to the "staged" directory, otherwise the file is moved to "current" to
    replace the current ledger data.

    When a ledger is opened for reading, it is checked whether the most recent ledger data is stored
    in "current" or a file in the "staged" directory. The ledger data is then read from the respective
    file.

    The ledger instance tracks the readers of the current and staged data to ensure that the relevant
    files are modified or removed only when the last reader accessing them is closed. When all readers
    are closed, the most recent staged data is moved to "current" and all other files in "staged" are
    removed.
*/
const auto TEMP_DATA_DIR_NAME = "temp";
const auto STAGED_DATA_DIR_NAME = "staged";
const auto CURRENT_DATA_FILE_NAME = "current";

const unsigned DATA_FORMAT_VERSION = 1;

const size_t MAX_PATH_LEN = 127;

// Internal result codes
enum Result {
    CURRENT_DATA_NOT_FOUND = 1
};

/*
    The layout of a ledger data file:

    Field     | Size      | Description
    ----------+-----------+------------
    data      | data_size | Contents of the ledger in an application-specific format
    info      | info_size | Protobuf-encoded ledger info (particle.firmware.LedgerInfo)
    data_size | 4         | Size of the "data" field (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    version   | 4         | Format version number (unsigned integer)

    All integer fields are encoded in little-endian byte order.
*/
struct LedgerDataFooter {
    uint32_t dataSize;
    uint32_t infoSize;
    uint32_t version;
} __attribute__((packed));

int readFooter(lfs_t* fs, lfs_file_t* file, size_t* dataSize = nullptr, size_t* infoSize = nullptr, int* version = nullptr) {
    LedgerDataFooter f = {};
    size_t n = CHECK_FS(lfs_file_read(fs, file, &f, sizeof(f)));
    if (n != sizeof(f)) {
        LOG(ERROR, "Unexpected end of ledger data file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (dataSize) {
        *dataSize = littleEndianToNative(f.dataSize);
    }
    if (infoSize) {
        *infoSize = littleEndianToNative(f.infoSize);
    }
    if (version) {
        *version = littleEndianToNative(f.version);
    }
    return n;
}

int writeFooter(lfs_t* fs, lfs_file_t* file, size_t dataSize, size_t infoSize, int version = DATA_FORMAT_VERSION) {
    LedgerDataFooter f = {};
    f.dataSize = nativeToLittleEndian(dataSize);
    f.infoSize = nativeToLittleEndian(infoSize);
    f.version = nativeToLittleEndian(version);
    size_t n = CHECK_FS(lfs_file_write(fs, file, &f, sizeof(f)));
    if (n != sizeof(f)) {
        LOG(ERROR, "Unexpected number of bytes written");
        return SYSTEM_ERROR_FILESYSTEM;
    }
    return n;
}

int writeLedgerInfo(lfs_t* fs, lfs_file_t* file, const char* ledgerName, const LedgerInfo& info) {
    // All fields must be set
    assert(info.isScopeTypeSet() && info.isScopeIdSet() && info.isSyncDirectionSet() && info.isDataSizeSet() &&
            info.isLastUpdatedSet() && info.isLastSyncedSet() && info.isUpdateCountSet() && info.isSyncPendingSet());
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    size_t n = strlcpy(pbInfo.name, ledgerName, sizeof(pbInfo.name));
    if (n >= sizeof(pbInfo.name)) {
        return SYSTEM_ERROR_INTERNAL; // Name is longer than the maximum size specified in ledger.proto
    }
    auto& scopeId = info.scopeId();
    assert(scopeId.size <= sizeof(pbInfo.scope_id.bytes));
    std::memcpy(pbInfo.scope_id.bytes, scopeId.data, scopeId.size);
    pbInfo.scope_id.size = scopeId.size;
    pbInfo.scope_type = static_cast<PB_LEDGER(ScopeType)>(info.scopeType());
    pbInfo.sync_direction = static_cast<PB_LEDGER(SyncDirection)>(info.syncDirection());
    if (info.lastUpdated()) {
        pbInfo.last_updated = info.lastUpdated();
        pbInfo.has_last_updated = true;
    }
    if (info.lastSynced()) {
        pbInfo.last_synced = info.lastSynced();
        pbInfo.has_last_synced = true;
    }
    pbInfo.update_count = info.updateCount();
    pbInfo.sync_pending = info.syncPending();
    n = CHECK(encodeProtobufToFile(file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo));
    return n;
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

bool isLedgerNameValid(const char* name) {
    size_t len = 0;
    char c = 0;
    while ((c = *name++)) {
        if (!(++len <= LEDGER_MAX_NAME_LENGTH && ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))) {
            return false;
        }
    }
    if (len == 0) {
        return false;
    }
    return true;
}

// Helper functions that transform LittleFS errors to a system error
inline int renameFile(lfs_t* fs, const char* oldPath, const char* newPath) {
    CHECK_FS(lfs_rename(fs, oldPath, newPath));
    return 0;
}

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

} // namespace

Ledger::Ledger(detail::LedgerSyncContext* ctx) :
        LedgerBase(ctx),
        lastSeqNum_(0),
        stagedSeqNum_(0),
        curReaderCount_(0),
        stagedReaderCount_(0),
        stagedFileCount_(0),
        lastUpdated_(0),
        lastSynced_(0),
        dataSize_(0),
        updateCount_(0),
        syncPending_(false),
        syncCallback_(nullptr),
        destroyAppData_(nullptr),
        appData_(nullptr),
        name_(""),
        scopeId_(EMPTY_LEDGER_SCOPE_ID),
        scopeType_(LEDGER_SCOPE_UNKNOWN),
        syncDir_(LEDGER_SYNC_DIRECTION_UNKNOWN),
        inited_(false) {
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
    FsLock fs;
    int r = CHECK(loadLedgerInfo(fs.instance()));
    if (r == Result::CURRENT_DATA_NOT_FOUND) {
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
        if (r > 0) {
            // Found staged data. Reload the ledger info
            CHECK(loadLedgerInfo(fs.instance()));
        }
    }
    inited_ = true;
    return 0;
}

int Ledger::initReader(LedgerReader& reader) {
    // Reference counter of this ledger instance is managed by the LedgerManager so if it needs
    // to be incremented it's important to do so before acquiring a lock on the ledger instance
    // to avoid a deadlock
    RefCountPtr<Ledger> ledgerPtr(this);
    std::lock_guard lock(*this);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(reader.init(info(), stagedSeqNum_, std::move(ledgerPtr)));
    if (stagedSeqNum_ > 0) {
        ++stagedReaderCount_;
    } else {
        ++curReaderCount_;
    }
    return 0;
}

int Ledger::initWriter(LedgerWriter& writer, LedgerWriteSource src) {
    RefCountPtr<Ledger> ledgerPtr(this); // See initReader()
    std::lock_guard lock(*this);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // It's allowed to write to a ledger while its sync direction is unknown
    if (src == LedgerWriteSource::USER && syncDir_ != LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD &&
            syncDir_ != LEDGER_SYNC_DIRECTION_UNKNOWN) {
        return SYSTEM_ERROR_LEDGER_READ_ONLY;
    }
    CHECK(writer.init(src, ++lastSeqNum_, std::move(ledgerPtr)));
    return 0;
}

LedgerInfo Ledger::info() const {
    std::lock_guard lock(*this);
    return LedgerInfo()
            .scopeType(scopeType_)
            .scopeId(scopeId_)
            .syncDirection(syncDir_)
            .dataSize(dataSize_)
            .lastUpdated(lastUpdated_)
            .lastSynced(lastSynced_)
            .updateCount(updateCount_)
            .syncPending(syncPending_);
}

int Ledger::updateInfo(const LedgerInfo& info) {
    std::lock_guard lock(*this);
    // Open the file with the current ledger data
    char path[MAX_PATH_LEN + 1];
    CHECK(getCurrentFilePath(path, sizeof(path), name_));
    FsLock fs;
    lfs_file_t file = {};
    // TODO: Rewriting parts of a file is inefficient with LittleFS. Consider using an append-only
    // structure for storing ledger data
    CHECK_FS(lfs_file_open(fs.instance(), &file, path, LFS_O_RDWR));
    NAMED_SCOPE_GUARD(closeFileGuard, {
        int r = closeFile(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the footer
    size_t dataSize = 0;
    size_t infoSize = 0;
    CHECK_FS(lfs_file_seek(fs.instance(), &file, -(int)sizeof(LedgerDataFooter), LFS_SEEK_END));
    CHECK(readFooter(fs.instance(), &file, &dataSize, &infoSize));
    // Write the info section
    CHECK_FS(lfs_file_seek(fs.instance(), &file, -(int)(infoSize + sizeof(LedgerDataFooter)), LFS_SEEK_END));
    auto newInfo = this->info().update(info);
    size_t newInfoSize = CHECK(writeLedgerInfo(fs.instance(), &file, name_, newInfo));
    // Write the footer
    if (newInfoSize != infoSize) {
        CHECK(writeFooter(fs.instance(), &file, dataSize, newInfoSize));
        size_t newFileSize = CHECK_FS(lfs_file_tell(fs.instance(), &file));
        CHECK_FS(lfs_file_truncate(fs.instance(), &file, newFileSize));
    }
    closeFileGuard.dismiss();
    CHECK_FS(lfs_file_close(fs.instance(), &file));
    // Update the instance
    setLedgerInfo(newInfo);
    return 0;
}

void Ledger::notifySynced() {
    std::lock_guard lock(*this);
    if (syncCallback_) {
        syncCallback_(reinterpret_cast<ledger_instance*>(this), appData_);
    }
}

int Ledger::notifyReaderClosed(bool staged) {
    std::lock_guard lock(*this);
    if (staged) {
        --stagedReaderCount_;
    } else {
        --curReaderCount_;
    }
    // Check if there's staged data and if it can be flushed
    int result = 0;
    if (stagedSeqNum_ > 0 && curReaderCount_ == 0 && stagedReaderCount_ == 0) {
        // To save on RAM usage, we don't count how many readers are open for each individual file,
        // which may result in the staged directory containing multiple unused files with outdated
        // ledger data. Typically, however, when the last reader is closed, there will be at most
        // one file with staged data as the application never keeps ledger streams open for long.
        // The only slow reader is the system which streams ledger data to the server
        FsLock fs;
        if (stagedFileCount_ == 1) {
            // Fast track: there's one file with staged data. Move it to "current"
            char srcPath[MAX_PATH_LEN + 1];
            char destPath[MAX_PATH_LEN + 1];
            CHECK(getStagedFilePath(srcPath, sizeof(srcPath), name_, stagedSeqNum_));
            CHECK(getCurrentFilePath(destPath, sizeof(destPath), name_));
            result = renameFile(fs.instance(), srcPath, destPath);
            if (result < 0) {
                // The filesystem is full or broken, or somebody messed up the staged directory
                LOG(ERROR, "Failed to rename file: %d", result);
                int r = removeFile(fs.instance(), srcPath);
                if (r < 0) {
                    LOG(WARN, "Failed to remove file: %d", r);
                }
            }
        } else {
            // The staged directory contains outdated files. Clean those up and move the most recent
            // file to "current"
            int r = flushStagedData(fs.instance());
            if (r < 0) {
                result = r;
            } else if (r != stagedSeqNum_) {
                // The file moved wasn't the most recent one for some reason
                LOG(ERROR, "Staged ledger data not found");
                result = SYSTEM_ERROR_LEDGER_INCONSISTENT_STATE;
            }
        }
        if (result < 0) {
            // Try recovering some known consistent state
            int r = loadLedgerInfo(fs.instance());
            if (r < 0) {
                LOG(ERROR, "Failed to recover ledger state");
            }
        }
        stagedSeqNum_ = 0;
        stagedFileCount_ = 0;
    }
    return result;
}

int Ledger::notifyWriterClosed(const LedgerInfo& info, int tempSeqNum) {
    std::unique_lock lock(*this);
    FsLock fs;
    // Move the file where appropriate
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
        CHECK(getStagedFilePath(destPath, sizeof(destPath), name_, tempSeqNum));
        newStagedFile = true;
    }
    CHECK_FS(lfs_rename(fs.instance(), srcPath, destPath));
    if (newStagedFile) {
        stagedSeqNum_ = tempSeqNum;
        ++stagedFileCount_;
    }
    setLedgerInfo(this->info().update(info));
    return 0;
}

int Ledger::loadLedgerInfo(lfs_t* fs) {
    // Open the file with the current ledger data
    char path[MAX_PATH_LEN + 1];
    CHECK(getCurrentFilePath(path, sizeof(path), name_));
    lfs_file_t file = {};
    int r = lfs_file_open(fs, &file, path, LFS_O_RDONLY);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return Result::CURRENT_DATA_NOT_FOUND;
        }
        CHECK_FS(r); // Forward the error
    }
    SCOPE_GUARD({
        int r = closeFile(fs, &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the footer
    size_t dataSize = 0;
    size_t infoSize = 0;
    int version = 0;
    CHECK_FS(lfs_file_seek(fs, &file, -(int)sizeof(LedgerDataFooter), LFS_SEEK_END));
    CHECK(readFooter(fs, &file, &dataSize, &infoSize, &version));
    if (version != DATA_FORMAT_VERSION) {
        LOG(ERROR, "Unsupported version of ledger data format: %d", version);
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    size_t fileSize = CHECK_FS(lfs_file_size(fs, &file));
    if (fileSize != dataSize + infoSize + sizeof(LedgerDataFooter)) {
        LOG(ERROR, "Unexpected size of ledger data file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    // Skip the data section
    CHECK_FS(lfs_file_seek(fs, &file, dataSize, LFS_SEEK_SET));
    // Read the info section
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    r = decodeProtobufFromFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo, infoSize);
    if (r < 0) {
        LOG(ERROR, "Failed to parse ledger info: %d", r);
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (std::strcmp(pbInfo.name, name_) != 0) {
        LOG(ERROR, "Unexpected ledger name");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    assert(pbInfo.scope_id.size <= sizeof(scopeId_.data));
    std::memcpy(scopeId_.data, pbInfo.scope_id.bytes, pbInfo.scope_id.size);
    scopeId_.size = pbInfo.scope_id.size;
    scopeType_ = static_cast<ledger_scope>(pbInfo.scope_type);
    syncDir_ = static_cast<ledger_sync_direction>(pbInfo.sync_direction);
    dataSize_ = dataSize;
    lastUpdated_ = pbInfo.has_last_updated ? pbInfo.last_updated : 0;
    lastSynced_ = pbInfo.has_last_synced ? pbInfo.last_synced : 0;
    updateCount_ = pbInfo.update_count;
    syncPending_ = pbInfo.sync_pending;
    return 0;
}

void Ledger::setLedgerInfo(const LedgerInfo& info) {
    // All fields must be set
    assert(info.isScopeTypeSet() && info.isScopeIdSet() && info.isSyncDirectionSet() && info.isDataSizeSet() &&
            info.isLastUpdatedSet() && info.isLastSyncedSet() && info.isUpdateCountSet() && info.isSyncPendingSet());
    scopeType_ = info.scopeType();
    scopeId_ = info.scopeId();
    syncDir_ = info.syncDirection();
    dataSize_ = info.dataSize();
    lastUpdated_ = info.lastUpdated();
    lastSynced_ = info.lastSynced();
    updateCount_ = info.updateCount();
    syncPending_ = info.syncPending();
}

int Ledger::initCurrentData(lfs_t* fs) {
    // Create a file for the current ledger data
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
    // Write the info section
    size_t infoSize = CHECK(writeLedgerInfo(fs, &file, name_, info()));
    // Write the footer
    CHECK(writeFooter(fs, &file, 0 /* dataSize */, infoSize));
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
    int maxSeqNum = 0;
    lfs_info entry = {};
    int r = 0;
    while ((r = lfs_dir_read(fs, &dir, &entry)) == 1) {
        if (entry.type != LFS_TYPE_REG) {
            if (entry.type != LFS_TYPE_DIR || (std::strcmp(entry.name, ".") != 0 && std::strcmp(entry.name, "..") != 0)) {
                LOG(WARN, "Found unexpected entry in ledger directory");
            }
            continue;
        }
        char* end = nullptr;
        int seqNum = std::strtol(entry.name, &end, 10);
        if (end != entry.name + std::strlen(entry.name)) {
            LOG(WARN, "Found unexpected entry in ledger directory");
            continue;
        }
        if (seqNum > maxSeqNum) {
            if (maxSeqNum > 0) {
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
            maxSeqNum = seqNum;
        } else {
            CHECK(getStagedFilePath(path, sizeof(path), name_, entry.name));
            CHECK_FS(lfs_remove(fs, path));
        }
    }
    CHECK_FS(r);
    if (maxSeqNum > 0) {
        // Replace "current" with the found file
        char destPath[MAX_PATH_LEN + 1];
        CHECK(getCurrentFilePath(destPath, sizeof(destPath), name_));
        CHECK(getStagedFilePath(path, sizeof(path), name_, fileName));
        CHECK_FS(lfs_rename(fs, path, destPath));
        return maxSeqNum;
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
            if (entry.type != LFS_TYPE_DIR || (std::strcmp(entry.name, ".") != 0 && std::strcmp(entry.name, "..") != 0)) {
                LOG(WARN, "Found unexpected entry in ledger directory");
            }
            continue;
        }
        CHECK(getTempFilePath(path, sizeof(path), name_, entry.name));
        CHECK_FS(lfs_remove(fs, path));
    }
    CHECK_FS(r);
    return 0;
}

void LedgerBase::addRef() const {
    LedgerManager::instance()->addLedgerRef(this);
}

void LedgerBase::release() const {
    LedgerManager::instance()->releaseLedger(this);
}

LedgerInfo& LedgerInfo::update(const LedgerInfo& info) {
    if (info.scopeType_.has_value()) {
        scopeType_ = info.scopeType_.value();
    }
    if (info.scopeId_.has_value()) {
        scopeId_ = info.scopeId_.value();
    }
    if (info.syncDir_.has_value()) {
        syncDir_ = info.syncDir_.value();
    }
    if (info.dataSize_.has_value()) {
        dataSize_ = info.dataSize_.value();
    }
    if (info.lastUpdated_.has_value()) {
        lastUpdated_ = info.lastUpdated_.value();
    }
    if (info.lastSynced_.has_value()) {
        lastSynced_ = info.lastSynced_.value();
    }
    if (info.updateCount_.has_value()) {
        updateCount_ = info.updateCount_.value();
    }
    if (info.syncPending_.has_value()) {
        syncPending_ = info.syncPending_.value();
    }
    return *this;
}

int LedgerReader::init(LedgerInfo info, int stagedSeqNum, RefCountPtr<Ledger> ledger) {
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
    ledger_ = std::move(ledger);
    info_ = std::move(info);
    open_ = true;
    return 0;
}

int LedgerReader::read(char* data, size_t size) {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t bytesToRead = std::min(size, info_.dataSize() - dataOffs_);
    if (size > 0 && bytesToRead == 0) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    FsLock fs;
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file_, data, bytesToRead));
    if (n != bytesToRead) {
        LOG(ERROR, "Unexpected end of ledger data file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
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
    // Let the ledger flush any data. Avoid holding a lock on the filesystem while trying to acquire
    // a lock on the ledger instance
    fs.unlock();
    int r = ledger_->notifyReaderClosed(staged_);
    fs.lock();
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger data: %d", r);
        return r;
    }
    return result;
}

int LedgerWriter::init(LedgerWriteSource src, int tempSeqNum, RefCountPtr<Ledger> ledger) {
    // Create a temporary file
    char path[MAX_PATH_LEN + 1];
    CHECK(getTempFilePath(path, sizeof(path), ledger->name(), tempSeqNum));
    FsLock fs;
    CHECK_FS(lfs_file_open(fs.instance(), &file_, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL));
    ledger_ = std::move(ledger);
    tempSeqNum_ = tempSeqNum;
    src_ = src;
    open_ = true;
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
    // Lock the ledger instance first then the filesystem, otherwise a deadlock is possible if the
    // LedgerManager has locked the ledger instance already but is waiting on the filesystem lock
    std::unique_lock lock(*ledger_);
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
    // Prepare the updated ledger info
    auto newInfo = ledger_->info().update(info_);
    newInfo.dataSize(dataSize_); // Can't be overridden
    newInfo.updateCount(newInfo.updateCount() + 1); // ditto
    if (!info_.isLastUpdatedSet()) {
        int64_t t = getMillisSinceEpoch();
        if (t < 0) {
            if (t != SYSTEM_ERROR_HAL_RTC_INVALID_TIME) {
                LOG(ERROR, "Failed to get current time: %d", (int)t);
            }
            t = 0; // Current time is unknown
        }
        newInfo.lastUpdated(t);
    }
    if (src_ == LedgerWriteSource::USER && !info_.isSyncPendingSet()) {
        newInfo.syncPending(true);
    }
    // Write the info section
    size_t infoSize = CHECK(writeLedgerInfo(fs.instance(), &file_, ledger_->name(), newInfo));
    // Write the footer
    CHECK(writeFooter(fs.instance(), &file_, dataSize_, infoSize));
    closeFileGuard.dismiss();
    CHECK_FS(lfs_file_close(fs.instance(), &file_));
    // Flush the data. Keep the ledger instance locked so that the ledger state is updated atomically
    // in the filesystem and RAM. TODO: Finalize writing to the temporary ledger file in Ledger rather
    // than in LedgerWriter
    int r = ledger_->notifyWriterClosed(newInfo, tempSeqNum_);
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger data: %d", r);
        return r;
    }
    removeFileGuard.dismiss();
    if (src_ == LedgerWriteSource::USER) {
        // Avoid holding any locks when calling into the manager
        fs.unlock();
        lock.unlock();
        LedgerManager::instance()->notifyLedgerChanged(ledger_->syncContext());
        fs.lock(); // FIXME: FsLock doesn't know when it's unlocked
    }
    return 0;
}

} // namespace system

} // namespace particle

#endif // HAL_PLATFORM_LEDGER
