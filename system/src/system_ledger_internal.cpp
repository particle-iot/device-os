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

#ifndef UNIT_TEST
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include <algorithm>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cassert>

#include "system_ledger_internal.h"

#include "control/common.h" // FIXME: Move Protobuf utilities to another directory

#include "timer_hal.h"
#include "rtc_hal.h"

#include "file_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define PB_LEDGER(_name) particle_cloud_ledger_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

LOG_SOURCE_CATEGORY("system.ledger");

static_assert(LEDGER_SCOPE_UNKNOWN == (int)PB_LEDGER(Scope_SCOPE_UNKNOWN) &&
        LEDGER_SCOPE_DEVICE == (int)PB_LEDGER(Scope_SCOPE_DEVICE) &&
        LEDGER_SCOPE_PRODUCT == (int)PB_LEDGER(Scope_SCOPE_PRODUCT) &&
        LEDGER_SCOPE_OWNER == (int)PB_LEDGER(Scope_SCOPE_OWNER));

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
const auto LEDGER_ROOT_DIR = "/usr/ledger";
const auto TEMP_DATA_DIR_NAME = "temp";
const auto STAGED_DATA_DIR_NAME = "staged";
const auto CURRENT_DATA_FILE_NAME = "current";

const unsigned DATA_FORMAT_VERSION = 1;

const size_t MAX_LEDGER_NAME_LEN = 32; // Must match the maximum length in ledger.proto
const size_t MAX_PATH_LEN = 127;

const unsigned SYNC_DELAY = 5000;
const unsigned MAX_SYNC_DELAY = 30000;

// Internal result codes
enum Result {
    RESULT_CURRENT_DATA_NOT_FOUND = 1
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
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    size_t n = strlcpy(pbInfo.name, ledgerName, sizeof(pbInfo.name));
    if (n >= sizeof(pbInfo.name)) {
        return SYSTEM_ERROR_INTERNAL; // The name is longer than specified in ledger.proto
    }
    pbInfo.scope = static_cast<PB_LEDGER(Scope)>(info.scope());
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

struct LedgerManager::LedgerContext {
    CString name; // Ledger name
    Ledger* instance; // Ledger instance. If null, the ledger is not instantiated
    ledger_sync_direction syncDir; // Sync direction
    int pendingState; // Pending state flags
    union {
        struct { // Fields specific to a device-to-cloud ledger
            uint64_t syncTime; // When to sync the ledger
            uint64_t forcedSyncTime; // When to force-sync the ledger
        };
        struct { // Fields specific to a cloud-to-device ledger
            uint64_t updateTime; // When the ledger was last updated
        };
    };

    LedgerContext() :
            instance(nullptr),
            syncDir(LEDGER_SYNC_DIRECTION_UNKNOWN),
            pendingState(0),
            syncTime(0),
            forcedSyncTime(0) {
    }
};

LedgerManager::LedgerManager() :
        connHandler_(nullptr),
        curLedger_(nullptr),
        nextSyncTime_(0),
        nextForcedSyncTime_(0),
        lastSyncDir_(LEDGER_SYNC_DIRECTION_UNKNOWN),
        state_(State::NEW),
        pendingState_(0) {
}

LedgerManager::~LedgerManager() {
}

int LedgerManager::init(OutboundConnectionHandler* connHandler) {
    if (state_ != State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Enumerate local ledgers
    FsLock fs;
    lfs_dir_t dir = {};
    CHECK_FS(lfs_dir_open(fs.instance(), &dir, LEDGER_ROOT_DIR));
    SCOPE_GUARD({
        int r = closeDir(fs.instance(), &dir);
        if (r < 0) {
            LOG(ERROR, "Failed to close directory handle: %d", r);
        }
    });
    LedgerContexts ledgers;
    lfs_info entry = {};
    int r = 0;
    while ((r = lfs_dir_read(fs.instance(), &dir, &entry)) == 1) {
        if (entry.type != LFS_TYPE_DIR) {
            LOG(WARN, "Found unexpected entry in ledger directory");
            continue;
        }
        if (std::strcmp(entry.name, ".") == 0 || std::strcmp(entry.name, "..") == 0) {
            continue;
        }
        // Load the ledger info
        Ledger ledger;
        int r = ledger.init(entry.name);
        if (r < 0) {
            LOG(ERROR, "Failed to initialize ledger: %d", r);
            continue;
        }
        // Create a sync context
        std::unique_ptr<LedgerContext> ctx(new(std::nothrow) LedgerContext());
        if (!ctx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        ctx->name = entry.name;
        if (!ctx->name) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        auto info = ledger.info();
        ctx->syncDir = info.syncDirection();
        if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            ctx->updateTime = info.lastUpdated();
        } else if (info.syncPending()) { // DEVICE_TO_CLOUD or UNKNOWN
            setPendingState(ctx.get(), PendingState::SYNC_TO_CLOUD);
        }
        if (!ledgers.append(std::move(ctx))) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    CHECK_FS(r);
    allLedgers_ = std::move(ledgers);
    connHandler_ = connHandler;
    state_ = State::OFFLINE;
    return 0;
}

int LedgerManager::getLedger(const char* name, RefCountPtr<Ledger>& ledger) {
    std::lock_guard lock(mutex_);
    if (state_ == State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Check if the requested ledger is already instantiated
    bool found = false;
    auto it = findLedger(name, found);
    if (found && (*it)->instance) {
        ledger = (*it)->instance;
        return 0;
    }
    std::unique_ptr<LedgerContext> newCtx;
    LedgerContext* ctx = it->get();
    if (!found) {
        // Create a new sync context
        newCtx.reset(new(std::nothrow) LedgerContext());
        if (!newCtx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        newCtx->name = name;
        if (!newCtx->name) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        ctx = newCtx.get();
    }
    // Create a ledger instance
    auto lr = makeRefCountPtr<Ledger>(ctx);
    if (!lr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = lr->init(ctx->name);
    if (r < 0) {
        LOG(ERROR, "Failed to initialize ledger: %d", r);
        return r;
    }
    if (!found) {
        it = allLedgers_.insert(it, std::move(newCtx));
        if (it == allLedgers_.end()) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        if (state_ > State::OFFLINE) {
            // Request the info about the newly created ledger
            setPendingState(ctx, PendingState::GET_INFO);
        }
    }
    ctx->instance = lr.get();
    ledger = std::move(lr);
    return 0;
}

int LedgerManager::removeLedgerData(const char* name) {
    if (!*name) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(mutex_);
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool found = false;
    auto it = findLedger(name, found);
    if (found) {
        if ((*it)->instance) {
            return SYSTEM_ERROR_LEDGER_IN_USE;
        }
        assert(curLedger_ != it->get());
        clearPendingState(it->get(), (*it)->pendingState);
        allLedgers_.erase(it);
    }
    char path[MAX_PATH_LEN + 1];
    CHECK(getLedgerDirPath(path, sizeof(path), name));
    CHECK(rmrf(path));
    return 0;
}

int LedgerManager::removeAllData() {
    std::lock_guard lock(mutex_);
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    for (auto& ctx: allLedgers_) {
        if (ctx->instance) {
            return SYSTEM_ERROR_LEDGER_IN_USE;
        }
    }
    allLedgers_.clear();
    pendingState_ = 0;
    CHECK(rmrf(LEDGER_ROOT_DIR));
    return 0;
}

int LedgerManager::connected() {
    std::lock_guard lock(mutex_);
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int r = connectedImpl();
    if (r < 0) {
        LOG(ERROR, "Ledger error: %d", r);
        state_ = State::FAILED;
        return r;
    }
    return 0;
}

int LedgerManager::connectedImpl() {
    for (auto& ctx: allLedgers_) {
        switch (ctx->syncDir) {
        case LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE:
            // TODO: Cache ledger subscriptions in the session data
            setPendingState(ctx.get(), PendingState::SUBSCRIBE);
            break;
        case LEDGER_SYNC_DIRECTION_UNKNOWN:
            setPendingState(ctx.get(), PendingState::GET_INFO);
            break;
        }
    }
    state_ = State::READY;
    return 0;
}

int LedgerManager::disconnected() {
    std::lock_guard lock(mutex_);
    if (state_ == State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int r = disconnectedImpl();
    if (r < 0) {
        LOG(ERROR, "Ledger error: %d", r);
        state_ = State::FAILED;
        return r;
    }
    return 0;
}

int LedgerManager::disconnectedImpl() {
    if (state_ == State::OFFLINE) {
        return 0;
    }
    if (stream_) {
        int r = stream_->close(true /* discard */);
        if (r < 0) {
            LOG(ERROR, "Failed to close ledger stream: %d", r);
        }
        stream_.reset();
    }
    for (auto& ctx: allLedgers_) {
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // DEVICE_TO_CLOUD or UNKNOWN
            ctx->syncTime = 0;
            ctx->forcedSyncTime = 0;
        }
        // Don't clear the SYNC_TO_CLOUD flag to ensure the local changes get sync'd when the device
        // reconnects
        ctx->pendingState &= PendingState::SYNC_TO_CLOUD;
    }
    pendingState_ = 0;
    if (state_ == State::SYNC_TO_CLOUD) {
        // Retry when the device reconnects
        assert(curLedger_);
        setPendingState(curLedger_, PendingState::SYNC_TO_CLOUD);
    }
    curLedger_ = nullptr;
    nextSyncTime_ = 0;
    nextForcedSyncTime_ = 0;
    state_ = State::OFFLINE;
    return 0;
}

int LedgerManager::requestSent(int reqId) {
    std::lock_guard lock(mutex_);
    return 0; // TODO
}

int LedgerManager::responseSent(int reqId) {
    std::lock_guard lock(mutex_);
    return 0; // TODO
}

int LedgerManager::requestReceived(int reqId, const char* data, size_t size, bool hasMore) {
    std::lock_guard lock(mutex_);
    return 0; // TODO
}

int LedgerManager::responseReceived(int reqId, const char* data, size_t size, bool hasMore) {
    std::lock_guard lock(mutex_);
    return 0; // TODO
}

void LedgerManager::run() {
    std::lock_guard lock(mutex_);
    if (state_ != State::READY) {
        // Some task is in progress or the manager is in a bad state
        return;
    }
    int r = runImpl();
    if (r < 0) {
        LOG(ERROR, "Ledger error: %d", r);
        state_ = State::FAILED;
    }
}

int LedgerManager::runImpl() {
    if (!pendingState_) {
        return 0;
    }
    if (pendingState_ & PendingState::GET_INFO) {
        LOG(TRACE, "Requesting ledger info");
        CHECK(sendGetInfoRequest());
        state_ = State::GET_INFO;
        return 0;
    }
    if (pendingState_ & PendingState::SUBSCRIBE) {
        LOG(TRACE, "Subscribing to ledger updates");
        CHECK(sendSubscribeRequest());
        state_ = State::SUBSCRIBE;
        return 0;
    }
    bool syncToCloud = pendingState_ & PendingState::SYNC_TO_CLOUD;
    bool syncFromCloud = pendingState_ & PendingState::SYNC_FROM_CLOUD;
    if (syncToCloud || syncFromCloud) {
        uint64_t now = 0;
        if (syncToCloud) {
            now = hal_timer_millis(nullptr);
            // Alternate between device-to-cloud and cloud-to-device ledgers when choosing the next
            // ledger to sync
            if ((now < nextSyncTime_ && now < nextForcedSyncTime_) || (syncFromCloud &&
                    lastSyncDir_ == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD)) {
                syncToCloud = false;
            }
        }
        if (syncToCloud) {
            // Sync a device-to-cloud ledger
            LedgerContext* ctx = nullptr;
            for (auto& c: allLedgers_) {
                if ((c->pendingState & PendingState::SYNC_TO_CLOUD) && (c->syncTime <= now || c->forcedSyncTime <= now)) {
                    ctx = c.get();
                    break;
                }
            }
            if (!ctx) {
                return SYSTEM_ERROR_INTERNAL;
            }
            LOG(TRACE, "Synchronizing ledger: \"%s\"", (const char*)ctx->name);
            CHECK(sendSetDataRequest(ctx));
            lastSyncDir_ = LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD;
            state_ = State::SYNC_TO_CLOUD;
        } else if (syncFromCloud) {
            // Sync a cloud-to-device ledger
            LedgerContext* ctx = nullptr;
            for (auto& c: allLedgers_) {
                if ((c->pendingState & PendingState::SYNC_FROM_CLOUD)) {
                    ctx = c.get();
                    break;
                }
            }
            if (!ctx) {
                return SYSTEM_ERROR_INTERNAL;
            }
            LOG(TRACE, "Synchronizing ledger: \"%s\"", (const char*)ctx->name);
            CHECK(sendGetDataRequest(ctx));
            lastSyncDir_ = LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE;
            state_ = State::SYNC_FROM_CLOUD;
        }
        return 0;
    }
    // State flags indicate that some task is pending but there's nothing to do
    return SYSTEM_ERROR_INTERNAL;
}

int LedgerManager::sendSetDataRequest(LedgerContext* ctx) {
    return 0; // TODO
}

int LedgerManager::sendGetDataRequest(LedgerContext* ctx) {
    return 0; // TODO
}

int LedgerManager::sendSubscribeRequest() {
    return 0; // TODO
}

int LedgerManager::sendGetInfoRequest() {
    return 0; // TODO
}

void LedgerManager::ledgerUpdated(Ledger* ledger) {
    std::lock_guard lock(mutex_);
    auto ctx = static_cast<LedgerContext*>(ledger->context());
    if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
        return;
    }
    setPendingState(ctx, PendingState::SYNC_TO_CLOUD);
    auto now = hal_timer_millis(nullptr);
    ctx->syncTime = now + SYNC_DELAY;
    if (!ctx->forcedSyncTime) {
        ctx->forcedSyncTime = now + MAX_SYNC_DELAY;
    }
    if (!nextSyncTime_) {
        nextSyncTime_ = ctx->syncTime;
    }
    if (!nextForcedSyncTime_) {
        nextForcedSyncTime_ = ctx->forcedSyncTime;
    }
}

void LedgerManager::setPendingState(LedgerContext* ctx, int state) {
    ctx->pendingState |= state;
    pendingState_ |= state;
}

void LedgerManager::clearPendingState(LedgerContext* ctx, int state) {
    ctx->pendingState &= ~state;
    pendingState_ = 0;
    for (auto& ctx: allLedgers_) {
        pendingState_ |= ctx->pendingState;
    }
}

LedgerManager::LedgerContexts::ConstIterator LedgerManager::findLedger(const char* name, bool& found) const {
    found = false;
    auto it = std::lower_bound(allLedgers_.begin(), allLedgers_.end(), name, [&found](const auto& ctx, const char* name) {
        auto r = std::strcmp(ctx->name, name);
        if (r == 0) {
            found = true;
        }
        return r < 0;
    });
    return it;
}

void LedgerManager::addLedgerRef(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    ++ledger->refCount();
}

void LedgerManager::releaseLedger(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    if (--ledger->refCount() == 0) {
        auto ctx = static_cast<LedgerContext*>(ledger->context());
        ctx->instance = nullptr;
        delete ledger;
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

Ledger::Ledger(void* ctx) :
        LedgerBase(ctx),
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
        name_(""),
        scope_(LEDGER_SCOPE_UNKNOWN),
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
        if (r > 0) {
            // Found staged data. Reload the ledger info
            CHECK(loadLedgerInfo(fs.instance()));
        }
    }
    inited_ = true;
    return 0;
}

int Ledger::initReader(LedgerReader& reader) {
    std::lock_guard lock(*this);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(reader.init(dataSize_, stagedSeqNum_, this));
    if (stagedSeqNum_ > 0) {
        ++stagedReaderCount_;
    } else {
        ++curReaderCount_;
    }
    return 0;
}

int Ledger::initWriter(LedgerWriteSource src, LedgerWriter& writer) {
    std::lock_guard lock(*this);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
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

int Ledger::readerClosed(bool staged) {
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
                result = SYSTEM_ERROR_LEDGER_INCONSISTENT;
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

int Ledger::writerClosed(const LedgerInfo& info, LedgerWriteSource src, int tempSeqNum) {
    std::lock_guard lock(*this);
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
    updateLedgerInfo(info);
    LedgerManager::instance()->ledgerUpdated(this);
    if (src == LedgerWriteSource::SYSTEM && syncCallback_) {
        syncCallback_(reinterpret_cast<ledger_instance*>(this), appData_);
    }
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
    scope_ = static_cast<ledger_scope>(pbInfo.scope);
    syncDir_ = static_cast<ledger_sync_direction>(pbInfo.sync_direction);
    dataSize_ = dataSize;
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

int LedgerReader::init(size_t dataSize, int stagedSeqNum, Ledger* ledger) {
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
    ledger_ = ledger;
    dataSize_ = dataSize;
    open_ = true;
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
    ledger_ = ledger;
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
    // Write the footer
    CHECK(writeFooter(fs.instance(), &file_, dataSize_, infoSize));
    closeFileGuard.dismiss();
    CHECK_FS(lfs_file_close(fs.instance(), &file_));
    // Flush the data
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

#endif // HAL_PLATFORM_LEDGER
