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

#include <pb_decode.h>

#include "system_ledger_internal.h"

#include "control/common.h" // FIXME: Move Protobuf utilities to another directory

#include "timer_hal.h"
#include "rtc_hal.h"

#include "nanopb_misc.h"
#include "file_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "cloud/cloud.pb.h" // Cloud protocol definitions
#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define PB_CLOUD(_name) particle_cloud_##_name
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

const unsigned MIN_SYNC_DELAY = 5000;
const unsigned MAX_SYNC_DELAY = 30000;

const auto REQUEST_URI = "L";
const auto REQUEST_METHOD = COAP_METHOD_POST;

// Internal result codes
enum Result {
    RESULT_CURRENT_DATA_NOT_FOUND = 1,
    RESULT_RESPONSE_SENT = 2
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

int getStreamForSubmessage(coap_message* msg, pb_istream_t* stream, uint32_t fieldTag) {
    pb_istream_t s = {};
    CHECK(pb_istream_from_coap_message(&s, msg, nullptr));
    for (;;) {
        uint32_t tag = 0;
        auto type = pb_wire_type_t();
        bool eof = false;
        if (!pb_decode_tag(&s, &type, &tag, &eof)) {
            if (eof) {
                *stream = s; // Return an empty stream
                return 0;
            }
            return SYSTEM_ERROR_BAD_DATA;
        }
        if (tag == fieldTag) {
            if (type != PB_WT_STRING) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            // Can't use pb_make_string_substream() as the message may contain incomplete data
            uint32_t size = 0;
            if (!pb_decode_varint32(&s, &size)) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            if (s.bytes_left > size) {
                s.bytes_left = size;
            }
            *stream = s;
            return 0;
        }
        if (!pb_skip_field(&s, type)) {
            return SYSTEM_ERROR_BAD_DATA;
        }
    }
}

int64_t getCurrentTime() {
    if (!hal_rtc_time_is_valid(nullptr)) {
        return SYSTEM_ERROR_HAL_RTC_INVALID_TIME;
    }
    timeval tv = {};
    CHECK(hal_rtc_get_time(&tv, nullptr));
    return tv.tv_sec * 1000ll + tv.tv_usec / 1000;
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

struct LedgerManager::LedgerContext {
    CString name; // Ledger name
    Ledger* instance; // Ledger instance. If null, the ledger is not instantiated
    ledger_sync_direction syncDir; // Sync direction
    int pendingState; // Pending state flags
    bool taskRunning; // Whether an asynchronous task is running for this ledger
    union {
        struct { // Fields specific to a device-to-cloud ledger
            uint64_t syncTime; // When to sync the ledger (ticks)
            uint64_t forcedSyncTime; // When to force-sync the ledger (ticks)
            bool changed; // Whether the ledger has local changes that need to be synchronized
        };
        struct { // Fields specific to a cloud-to-device ledger
            uint64_t lastUpdated; // Time the ledger was last updated (Unix time in ms)
        };
    };

    LedgerContext() :
            instance(nullptr),
            syncDir(LEDGER_SYNC_DIRECTION_UNKNOWN),
            pendingState(0),
            taskRunning(false),
            syncTime(0),
            forcedSyncTime(0),
            changed(false) {
    }

    void resetDeviceToCloudSyncState() {
        syncTime = 0;
        forcedSyncTime = 0;
        changed = false;
    }

    void resetCloudToDeviceSyncState() {
        lastUpdated = 0;
    }
};

LedgerManager::LedgerManager() :
        curLedger_(nullptr),
        nextSyncTime_(0),
        state_(State::NEW),
        pendingState_(0),
        reqId_(COAP_INVALID_REQUEST_ID) {
}

LedgerManager::~LedgerManager() {
    coap_remove_request_handler(REQUEST_URI, REQUEST_METHOD, nullptr);
    coap_remove_connection_handler(connectionCallback, nullptr);
}

int LedgerManager::init() {
    if (state_ != State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Enumerate local ledgers
    LedgerContexts ledgers;
    FsLock fs;
    lfs_dir_t dir = {};
    int r = lfs_dir_open(fs.instance(), &dir, LEDGER_ROOT_DIR);
    if (r == 0) {
        SCOPE_GUARD({
            int r = closeDir(fs.instance(), &dir);
            if (r < 0) {
                LOG(ERROR, "Failed to close directory handle: %d", r);
            }
        });
        lfs_info entry = {};
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
                ctx->lastUpdated = info.lastUpdated();
            } else if (info.syncPending()) { // DEVICE_TO_CLOUD or UNKNOWN
                ctx->changed = true;
            }
            if (!ledgers.append(std::move(ctx))) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        CHECK_FS(r);
    } else if (r != LFS_ERR_NOENT) {
        CHECK_FS(r); // Forward the error
    }
    CHECK(coap_add_connection_handler(connectionCallback, this, nullptr));
    NAMED_SCOPE_GUARD(removeConnHandler, {
        coap_remove_connection_handler(connectionCallback, nullptr);
    });
    CHECK(coap_add_request_handler(REQUEST_URI, REQUEST_METHOD, requestCallback, this, nullptr));
    removeConnHandler.dismiss();
    allLedgers_ = std::move(ledgers);
    state_ = State::OFFLINE;
    return 0;
}

int LedgerManager::getLedger(RefCountPtr<Ledger>& ledger, const char* name, bool create) {
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
        if (!create) {
            return SYSTEM_ERROR_LEDGER_NOT_FOUND;
        }
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
        if (state_ >= State::READY) {
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
    CHECK(rmrf(LEDGER_ROOT_DIR));
    return 0;
}

void LedgerManager::run() {
    std::lock_guard lock(mutex_);
    int r = processTasks();
    if (r < 0) {
        LOG(ERROR, "Failed to process ledger task: %d", r);
        handleError(r);
    }
}

int LedgerManager::processTasks() {
    if (state_ != State::READY || !pendingState_) {
        // Some task is in progress, the manager is in a bad state, or there's nothing to do
        return 0;
    }
    if (pendingState_ & PendingState::GET_INFO) {
        LOG(INFO, "Requesting ledger info");
        CHECK(sendGetInfoRequest());
        return 0;
    }
    if (pendingState_ & PendingState::SUBSCRIBE) {
        LOG(INFO, "Subscribing to ledger updates");
        CHECK(sendSubscribeRequest());
        return 0;
    }
    if (pendingState_ & PendingState::SYNC_TO_CLOUD) {
        auto now = hal_timer_millis(nullptr);
        if (now >= nextSyncTime_) {
            uint64_t t = 0;
            LedgerContext* ctx = nullptr;
            for (auto& c: allLedgers_) {
                if (c->pendingState & PendingState::SYNC_TO_CLOUD) {
                    if (now >= c->syncTime) {
                        ctx = c.get();
                        break;
                    }
                    if (!t || c->syncTime < t) {
                        t = c->syncTime;
                    }
                }
            }
            if (ctx) {
                LOG(TRACE, "Synchronizing ledger: %s", (const char*)ctx->name);
                CHECK(sendSetDataRequest(ctx));
                return 0;
            }
            nextSyncTime_ = t;
        }
    }
    if (pendingState_ & PendingState::SYNC_FROM_CLOUD) {
        LedgerContext* ctx = nullptr;
        for (auto& c: allLedgers_) {
            if (c->pendingState & PendingState::SYNC_FROM_CLOUD) {
                ctx = c.get();
                break;
            }
        }
        if (ctx) {
            LOG(TRACE, "Synchronizing ledger: %s", (const char*)ctx->name);
            CHECK(sendGetDataRequest(ctx));
            return 0;
        }
    }
    return 0;
}

int LedgerManager::notifyConnected() {
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    LOG(TRACE, "Connected");
    for (auto& ctx: allLedgers_) {
        switch (ctx->syncDir) {
        case LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD: {
            if (ctx->changed) {
                setPendingState(ctx.get(), PendingState::SYNC_TO_CLOUD);
                updateSyncTime(ctx.get());
            }
            break;
        }
        case LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE: {
            // TODO: Cache ledger subscriptions in the session data
            setPendingState(ctx.get(), PendingState::SUBSCRIBE);
            break;
        }
        case LEDGER_SYNC_DIRECTION_UNKNOWN: {
            setPendingState(ctx.get(), PendingState::GET_INFO);
            break;
        }
        default:
            break;
        }
    }
    state_ = State::READY;
    return 0;
}

void LedgerManager::notifyDisconnected(int /* error */) {
    if (state_ == State::OFFLINE) {
        return;
    }
    if (state_ == State::NEW) {
        LOG(ERROR, "Invalid manager state: %d", (int)state_);
        return;
    }
    LOG(TRACE, "Disconnected");
    if (stream_) {
        int r = stream_->close(true /* discard */);
        if (r < 0) {
            LOG(ERROR, "Failed to close ledger stream: %d", r);
        }
        stream_.reset();
    }
    for (auto& ctx: allLedgers_) {
        if (ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD || ctx->syncDir == LEDGER_SYNC_DIRECTION_UNKNOWN) {
            ctx->syncTime = 0;
            ctx->forcedSyncTime = 0;
        }
        ctx->pendingState = 0;
        ctx->taskRunning = false;
    }
    pendingState_ = 0;
    nextSyncTime_ = 0;
    curLedger_ = nullptr;
    reqId_ = COAP_INVALID_REQUEST_ID;
    state_ = State::OFFLINE;
}

int LedgerManager::receiveRequest(coap_message* msg, int reqId) {
    if (state_ < State::READY) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Get the request type. XXX: The parsing code here assumes that the message fields are encoded
    // in order of their field numbers, which is not guaranteed by the Protobuf spec in general
    char buf[32] = {};
    size_t n = CHECK(coap_peek_payload(msg, buf, sizeof(buf), nullptr));
    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buf, n);
    uint32_t reqType = 0;
    uint32_t fieldTag = 0;
    auto fieldType = pb_wire_type_t();
    bool eof = false;
    if (!pb_decode_tag(&stream, &fieldType, &fieldTag, &eof)) {
        if (!eof) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        // Request type field is missing so its value is 0
    } else if (fieldTag == PB_CLOUD(Request_type_tag)) {
        if (fieldType != PB_WT_VARINT || !pb_decode_varint32(&stream, &reqType)) {
            return SYSTEM_ERROR_BAD_DATA;
        }
    } // else: Request type field is missing so its value is 0
    switch (reqType) {
    case PB_CLOUD(Request_Type_LEDGER_NOTIFY_UPDATE): {
        CHECK(receiveNotifyUpdateRequest(msg, reqId));
        break;
    }
    case PB_CLOUD(Request_Type_LEDGER_RESET_INFO): {
        CHECK(receiveResetInfoRequest(msg, reqId));
        break;
    }
    default:
        LOG(WARN, "Unknown request type: %d", (int)reqType);
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

int LedgerManager::receiveNotifyUpdateRequest(coap_message* msg, int /* reqId */) {
    LOG(TRACE, "Received update notification");
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg, &stream, PB_CLOUD(Request_ledger_notify_update_tag)));
    PB_LEDGER(NotifyUpdateRequest) pbReq = {};
    pbReq.ledgers.arg = this;
    pbReq.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto self = (LedgerManager*)*arg;
        PB_LEDGER(NotifyUpdateRequest_Ledger) pbLedger = {};
        if (!pb_decode(stream, &PB_LEDGER(NotifyUpdateRequest_Ledger_msg), &pbLedger)) {
            return false;
        }
        // Find the updated ledger
        bool found = false;
        auto it = self->findLedger(pbLedger.name, found);
        if (!found) {
            LOG(WARN, "Unknown ledger: %s", pbLedger.name);
            return true; // Ignore
        }
        auto ctx = it->get();
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            // If the sync direction is unknown at this point, we already know that the ledger info
            // is inconsistent between the device and server and we've been unable to fix it
            if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN) {
                LOG(ERROR, "Received update notification for device-to-cloud ledger: %s", pbLedger.name);
                self->setPendingState(ctx, PendingState::GET_INFO);
            }
            return true;
        }
        // Schedule a sync for the updated ledger
        if (pbLedger.last_updated > ctx->lastUpdated) {
            LOG(TRACE, "Ledger changed: %s", pbLedger.name);
            self->setPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
        }
        return true;
    };
    if (!pb_decode(&stream, &PB_LEDGER(NotifyUpdateRequest_msg), &pbReq)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int LedgerManager::receiveResetInfoRequest(coap_message* msg, int /* reqId */) {
    LOG(WARN, "Received a reset request, re-requesting ledger info");
    for (auto& ctx: allLedgers_) {
        setPendingState(ctx.get(), PendingState::GET_INFO);
    }
    return 0;
}

int LedgerManager::receiveResponse(coap_message* msg, int code, int reqId) {
    if (state_ <= State::READY || reqId != reqId_) {
        return 0; // Ignore response
    }
    auto codeClass = COAP_CODE_CLASS(code);
    if (codeClass != 2 && codeClass != 4) { // Success 2.xx or Client Error 4.xx
        LOG(ERROR, "Ledger request failed: %d.%02d", (int)codeClass, (int)COAP_CODE_DETAIL(code));
        return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
    }
    // Get the protocol-specific result code. XXX: The parsing code here assumes that the message
    // fields are encoded in order of their field numbers, which is not guaranteed by the Protobuf
    // spec in general
    char buf[32] = {};
    int r = coap_peek_payload(msg, buf, sizeof(buf), nullptr);
    if (r < 0) {
        if (r != SYSTEM_ERROR_END_OF_STREAM) {
            return r;
        }
        r = 0; // Response is empty
    }
    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buf, r);
    int64_t result = 0;
    uint32_t fieldTag = 0;
    auto fieldType = pb_wire_type_t();
    bool eof = false;
    if (!pb_decode_tag(&stream, &fieldType, &fieldTag, &eof)) {
        if (!eof) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        // Result code field is missing so its value is 0
    } else if (fieldTag == PB_CLOUD(Response_result_tag)) {
        if (fieldType != PB_WT_VARINT || !pb_decode_svarint(&stream, &result)) {
            return SYSTEM_ERROR_BAD_DATA;
        }
    } // else: Result code field is missing so its value is 0
    if (result == PB_CLOUD(Response_Result_OK) && codeClass != 2) {
        // CoAP response code indicates an error but the protocol-specific result code is OK
        return SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
    }
    switch (state_) {
    case State::SYNC_TO_CLOUD: {
        CHECK(receiveSetDataResponse(msg, result, reqId));
        break;
    }
    case State::SYNC_FROM_CLOUD: {
        CHECK(receiveGetDataResponse(msg, result, reqId));
        break;
    }
    case State::SUBSCRIBE: {
        CHECK(receiveSubscribeResponse(msg, result, reqId));
        break;
    }
    case State::GET_INFO: {
        CHECK(receiveGetInfoResponse(msg, result, reqId));
        break;
    }
    default:
        LOG(ERROR, "Unexpected response");
        return SYSTEM_ERROR_INTERNAL;
    }
    reqId_ = COAP_INVALID_REQUEST_ID;
    state_ = State::READY;
    return 0;
}

int LedgerManager::receiveSetDataResponse(coap_message* msg, int result, int /* reqId */) {
    assert(state_ == State::SYNC_TO_CLOUD && curLedger_ && curLedger_->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD &&
            curLedger_->taskRunning);
    auto name = (const char*)curLedger_->name;
    if (result != 0) {
        LOG(ERROR, "Failed to sync ledger: %s; response result: %d", name, result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger is no longer accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", name);
        setPendingState(curLedger_, PendingState::GET_INFO | PendingState::SYNC_TO_CLOUD);
        curLedger_->taskRunning = false;
        curLedger_ = nullptr;
        return 0;
    }
    LOG(TRACE, "Sent ledger data: %s", name);
    LedgerInfo info;
    auto t = CHECK(getCurrentTime());
    info.lastSynced(t);
    if (!(curLedger_->pendingState & PendingState::SYNC_TO_CLOUD)) {
        info.syncPending(false);
        curLedger_->syncTime = 0;
        curLedger_->forcedSyncTime = 0;
        curLedger_->changed = false;
    } else {
        auto now = hal_timer_millis(nullptr);
        curLedger_->syncTime = now + MIN_SYNC_DELAY;
        curLedger_->forcedSyncTime = now + MAX_SYNC_DELAY;
    }
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, curLedger_->name, false /* create */));
    CHECK(ledger->updateInfo(info));
    curLedger_->taskRunning = false;
    curLedger_ = nullptr;
    // TODO: Reorder the ledger entries so that they're synchronized in a round-robin fashion
    return 0;
}

int LedgerManager::receiveGetDataResponse(coap_message* msg, int result, int /* reqId */) {
    assert(state_ == State::SYNC_FROM_CLOUD && curLedger_ && curLedger_->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE &&
            curLedger_->taskRunning && !stream_);
    auto name = (const char*)curLedger_->name;
    if (result != 0) {
        LOG(ERROR, "Failed to sync ledger: %s; response result: %d", name, result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger is no longer accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", name);
        setPendingState(curLedger_, PendingState::GET_INFO | PendingState::SYNC_FROM_CLOUD);
        curLedger_->taskRunning = false;
        curLedger_ = nullptr;
        return 0;
    }
    LOG(TRACE, "Received ledger data: %s", name);
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, name, false /* create */));
    LedgerWriter writer;
    CHECK(ledger->initWriter(writer, LedgerWriteSource::SYSTEM));
    PB_LEDGER(GetDataResponse) pbResp = {};
    struct DecodeContext {
        LedgerManager* self;
        LedgerWriter* writer;
        int error;
        bool hasData;
    };
    DecodeContext d = { .self = this, .writer = &writer, .error = 0, .hasData = false };
    pbResp.data.arg = &d;
    pbResp.data.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;
        char buf[128];
        while (stream->bytes_left > 0) {
            size_t n = std::min(stream->bytes_left, sizeof(buf));
            if (!pb_read(stream, (pb_byte_t*)buf, n)) {
                return false;
            }
            d->error = d->writer->write(buf, n);
            if (d->error < 0 || (size_t)d->error != n) {
                return false;
            }
        }
        d->hasData = true;
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg, &stream, PB_CLOUD(Response_ledger_get_data_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(GetDataResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    if (d.hasData) {
        auto& info = writer.ledgerInfo();
        info.lastUpdated(pbResp.last_updated);
        auto t = CHECK(getCurrentTime());
        info.lastSynced(t);
        CHECK(writer.close());
    }
    curLedger_->taskRunning = false;
    curLedger_ = nullptr;
    // TODO: Reorder the ledger entries so that they're synchronized in a round-robin fashion
    return 0;
}

int LedgerManager::receiveSubscribeResponse(coap_message* msg, int result, int /* reqId */) {
    assert(state_ == State::SUBSCRIBE);
    if (result != 0) {
        LOG(ERROR, "Failed to subscribe to ledger updates; response result: %d", result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Some of the cloud-to-device ledgers are no longer accessible, re-request their info
        LOG(WARN, "Re-requesting ledger info");
        for (auto& ctx: allLedgers_) {
            if (ctx->taskRunning) {
                setPendingState(ctx.get(), PendingState::GET_INFO);
                ctx->taskRunning = false;
            }
        }
        return 0;
    }
    LOG(INFO, "Subscribed to ledger updates");
    PB_LEDGER(SubscribeResponse) pbResp = {};
    struct DecodeContext {
        LedgerManager* self;
        int error;
    };
    DecodeContext d = { .self = this, .error = 0 };
    pbResp.ledgers.arg = &d;
    pbResp.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;
        PB_LEDGER(SubscribeResponse_Ledger) pbLedger = {};
        if (!pb_decode(stream, &PB_LEDGER(SubscribeResponse_Ledger_msg), &pbLedger)) {
            return false;
        }
        LOG(TRACE, "Ledger: %s", pbLedger.name);
        bool found = false;
        auto it = d->self->findLedger(pbLedger.name, found);
        if (!found) {
            LOG(WARN, "Unknown ledger: %s", pbLedger.name);
            return true; // Ignore
        }
        auto ctx = it->get();
        if (!ctx->taskRunning) {
            LOG(ERROR, "Unexpected subscription: %s", pbLedger.name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        ctx->taskRunning = false;
        if (pbLedger.last_updated > ctx->lastUpdated) {
            d->self->setPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
        }
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg, &stream, PB_CLOUD(Response_ledger_subscribe_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(SubscribeResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    for (auto& ctx: allLedgers_) {
        if (ctx->taskRunning) {
            LOG(ERROR, "Missing subscription: %s", (const char*)ctx->name);
            return SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
        }
    }
    return 0;
}

int LedgerManager::receiveGetInfoResponse(coap_message* msg, int result, int /* reqId */) {
    assert(state_ == State::GET_INFO);
    if (result != 0) {
        LOG(ERROR, "Failed to get ledger info; response result: %d", result);
        return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
    }
    LOG(INFO, "Received ledger info");
    struct DecodeContext {
        LedgerManager* self;
        int error;
        bool resubscribe;
        bool localInfoIsInvalid;
    };
    DecodeContext d = { .self = this, .error = 0, .resubscribe = false, .localInfoIsInvalid = false };
    PB_LEDGER(GetInfoResponse) pbResp = {};
    pbResp.ledgers.arg = &d;
    pbResp.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto d = (DecodeContext*)*arg;
        PB_LEDGER(GetInfoResponse_Ledger) pbLedger = {};
        if (!pb_decode(stream, &PB_LEDGER(GetInfoResponse_Ledger_msg), &pbLedger)) {
            return false;
        }
        LOG(TRACE, "Ledger: %s; scope: %d; sync direction: %d", pbLedger.name, (int)pbLedger.scope,
                (int)pbLedger.sync_direction);
        RefCountPtr<Ledger> ledger;
        d->error = d->self->getLedger(ledger, pbLedger.name, false /* create */);
        if (d->error < 0) {
            return false;
        }
        auto ctx = static_cast<LedgerContext*>(ledger->context());
        if (!ctx->taskRunning) {
            LOG(ERROR, "Unexpected ledger info: %s", pbLedger.name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        ctx->taskRunning = false;
        LedgerInfo newInfo;
        newInfo.syncDirection(static_cast<ledger_sync_direction>(pbLedger.sync_direction));
        newInfo.scope(static_cast<ledger_scope>(pbLedger.scope));
        if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_UNKNOWN || newInfo.scope() == LEDGER_SCOPE_UNKNOWN) {
            LOG(ERROR, "Invalid ledger scope or sync direction: %s", pbLedger.name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        auto localInfo = ledger->info();
        if (localInfo.syncDirection() != newInfo.syncDirection() || localInfo.scope() != newInfo.scope()) {
            if (localInfo.syncDirection() != LEDGER_SYNC_DIRECTION_UNKNOWN) { // DEVICE_TO_CLOUD -> CLOUD_TO_DEVICE or vice versa
                // This should not normally happen as the sync direction and scope of an existing
                // ledger cannot be changed
                LOG(ERROR, "Ledger scope or sync direction changed: %s", pbLedger.name);
                newInfo.syncDirection(LEDGER_SYNC_DIRECTION_UNKNOWN);
                newInfo.scope(LEDGER_SCOPE_UNKNOWN);
                if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // CLOUD_TO_DEVICE -> DEVICE_TO_CLOUD
                    ctx->resetDeviceToCloudSyncState();
                }
                d->localInfoIsInvalid = true; // Will cause a transition to the failed state
            } else if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // UNKNOWN -> CLOUD_TO_DEVICE
                if (localInfo.syncPending()) {
                    LOG(WARN, "Ledger has local changes but its actual sync direction is cloud-to-device: %s", pbLedger.name);
                    newInfo.syncPending(false);
                    newInfo.lastUpdated(0);
                }
                ctx->resetCloudToDeviceSyncState();
                d->resubscribe = true;
            } else if (localInfo.syncPending()) { // UNKNOWN -> DEVICE_TO_CLOUD
                d->self->setPendingState(ctx, PendingState::SYNC_TO_CLOUD);
                d->self->updateSyncTime(ctx);
            }
            // Save the new ledger info
            d->error = ledger->updateInfo(newInfo);
            if (d->error < 0) {
                return false;
            }
            ctx->syncDir = newInfo.syncDirection();
        }
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg, &stream, PB_CLOUD(Response_ledger_get_info_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(GetInfoResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    for (auto& ctx: allLedgers_) {
        if (ctx->taskRunning) {
            // Ledger doesn't exist or is no longer accessible by the device
            LOG(WARN, "Ledger not found: %s", (const char*)ctx->name);
            if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN) { // DEVICE_TO_CLOUD/CLOUD_TO_DEVICE -> UNKNOWN
                LedgerInfo info;
                info.syncDirection(LEDGER_SYNC_DIRECTION_UNKNOWN);
                info.scope(LEDGER_SCOPE_UNKNOWN);
                RefCountPtr<Ledger> ledger;
                CHECK(getLedger(ledger, ctx->name, false /* create */));
                CHECK(ledger->updateInfo(info));
                if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // CLOUD_TO_DEVICE -> UNKNOWN
                    ctx->resetDeviceToCloudSyncState();
                    // Don't bother unsubscribing from the deleted ledger
                }
                ctx->syncDir = LEDGER_SYNC_DIRECTION_UNKNOWN;
            }
            clearPendingState(ctx.get(), ctx->pendingState);
            ctx->taskRunning = false;
        } else if (d.resubscribe && ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            setPendingState(ctx.get(), PendingState::SUBSCRIBE);
        }
    }
    if (d.localInfoIsInvalid) {
        return SYSTEM_ERROR_LEDGER_INCONSISTENT_STATE;
    }
    return 0;
}

int LedgerManager::sendSetDataRequest(LedgerContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_TO_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD && !curLedger_ && !stream_);
    auto name = (const char*)ctx->name;
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, name, false /* create */));
    // Create a reader and get the current ledger info atomically
    std::unique_lock lock(*ledger);
    LedgerReader reader;
    CHECK(ledger->initReader(reader));
    auto info = ledger->info();
    lock.unlock();
    struct EncodeContext {
        LedgerManager* self;
        LedgerReader* reader;
        LedgerInfo* info;
        int error;
    };
    EncodeContext d = { .self = this, .reader = &reader, .info = &info, .error = 0 };
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_SET_DATA);
    pbReq.which_data = PB_CLOUD(Request_ledger_set_data_tag);
    size_t n = strlcpy(pbReq.data.ledger_set_data.name, name, sizeof(pbReq.data.ledger_set_data.name));
    if (n >= sizeof(pbReq.data.ledger_set_data.name)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    pbReq.data.ledger_set_data.last_updated = info.lastUpdated();
    pbReq.data.ledger_set_data.data.arg = &d;
    pbReq.data.ledger_set_data.data.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto d = (EncodeContext*)*arg;
        size_t size = d->info->dataSize();
        if (!pb_encode_tag_for_field(stream, field) || !pb_encode_varint(stream, size)) {
            return false;
        }
        char buf[128];
        while (size > 0) {
            size_t n = std::min(size, sizeof(buf));
            d->error = d->reader->read(buf, n);
            if (d->error < 0 || (size_t)d->error != n) {
                return false;
            }
            if (!pb_write(stream, (const pb_byte_t*)buf, n)) {
                return false;
            }
            size -= n;
        }
        // Rewind the reader so that this callback can be called again
        d->error = d->reader->rewind();
        if (d->error < 0) {
            return false;
        }
        return true;
    };
    coap_message* msg = nullptr;
    int reqId = CHECK(coap_begin_request(&msg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg, nullptr);
    });
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg, nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg, responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    destroyMsgGuard.dismiss();
    // Clear the pending state
    clearPendingState(ctx, PendingState::SYNC_TO_CLOUD);
    ctx->taskRunning = true;
    curLedger_ = ctx;
    reqId_ = reqId;
    state_ = State::SYNC_TO_CLOUD;
    return 0;
}

int LedgerManager::sendGetDataRequest(LedgerContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_FROM_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE && !curLedger_);
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_GET_DATA);
    pbReq.which_data = PB_CLOUD(Request_ledger_get_data_tag);
    auto name = (const char*)ctx->name;
    size_t n = strlcpy(pbReq.data.ledger_get_data.name, name, sizeof(pbReq.data.ledger_get_data.name));
    if (n >= sizeof(pbReq.data.ledger_get_data.name)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    if (ctx->lastUpdated > 0) {
        pbReq.data.ledger_get_data.last_updated = ctx->lastUpdated;
        pbReq.data.ledger_get_data.has_last_updated = true;
    }
    coap_message* msg = nullptr;
    int reqId = CHECK(coap_begin_request(&msg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg, nullptr);
    });
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg, nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg, responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    destroyMsgGuard.dismiss();
    // Clear the pending state
    clearPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
    ctx->taskRunning = true;
    curLedger_ = ctx;
    reqId_ = reqId;
    state_ = State::SYNC_FROM_CLOUD;
    return 0;
}

int LedgerManager::sendSubscribeRequest() {
    assert(state_ == State::READY && (pendingState_ & PendingState::SUBSCRIBE));
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_SUBSCRIBE);
    pbReq.which_data = PB_CLOUD(Request_ledger_subscribe_tag);
    pbReq.data.ledger_subscribe.ledgers.arg = this;
    pbReq.data.ledger_subscribe.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        // Make sure not to update any state in this callback as it may be called multiple times
        auto self = (const LedgerManager*)*arg;
        for (auto& ctx: self->allLedgers_) {
            if (ctx->pendingState & PendingState::SUBSCRIBE) {
                if (!pb_encode_tag_for_field(stream, field) ||
                        !pb_encode_string(stream, (const pb_byte_t*)(const char*)ctx->name, std::strlen(ctx->name))) {
                    return false;
                }
            }
        }
        return true;
    };
    coap_message* msg = nullptr;
    int reqId = CHECK(coap_begin_request(&msg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg, nullptr);
    });
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg, nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg, responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    destroyMsgGuard.dismiss();
    // Clear the pending state
    pendingState_ = 0;
    for (auto& ctx: allLedgers_) {
        if (ctx->pendingState & PendingState::SUBSCRIBE) {
            ctx->pendingState &= ~PendingState::SUBSCRIBE;
            ctx->taskRunning = true;
        }
        pendingState_ |= ctx->pendingState;
    }
    reqId_ = reqId;
    state_ = State::SUBSCRIBE;
    return 0;
}

int LedgerManager::sendGetInfoRequest() {
    assert(state_ == State::READY && (pendingState_ & PendingState::GET_INFO));
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_GET_INFO);
    pbReq.which_data = PB_CLOUD(Request_ledger_get_info_tag);
    pbReq.data.ledger_get_info.ledgers.arg = this;
    pbReq.data.ledger_get_info.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        // Make sure not to update any state in this callback as it may be called multiple times
        auto self = (const LedgerManager*)*arg;
        for (auto& ctx: self->allLedgers_) {
            if (ctx->pendingState & PendingState::GET_INFO) {
                if (!pb_encode_tag_for_field(stream, field) ||
                        !pb_encode_string(stream, (const pb_byte_t*)(const char*)ctx->name, std::strlen(ctx->name))) {
                    return false;
                }
            }
        }
        return true;
    };
    coap_message* msg = nullptr;
    int reqId = CHECK(coap_begin_request(&msg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg, nullptr);
    });
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg, nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg, responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    destroyMsgGuard.dismiss();
    // Clear the pending state
    pendingState_ = 0;
    for (auto& ctx: allLedgers_) {
        if (ctx->pendingState & PendingState::GET_INFO) {
            ctx->pendingState &= ~PendingState::GET_INFO;
            ctx->taskRunning = true;
        }
        pendingState_ |= ctx->pendingState;
    }
    reqId_ = reqId;
    state_ = State::GET_INFO;
    return 0;
}

int LedgerManager::sendResponse(int result, int reqId) {
    coap_message* msg = nullptr;
    int code = (result == 0) ? COAP_RESPONSE_CHANGED : COAP_RESPONSE_BAD_REQUEST;
    CHECK(coap_begin_response(&msg, code, reqId, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg, nullptr);
    });
    PB_CLOUD(Response) pbResp = {};
    pbResp.result = result;
    EncodedString pbMsg(&pbResp.message);
    if (result < 0) {
        pbMsg.data = get_system_error_message(result);
        pbMsg.size = std::strlen(pbMsg.data);
    }
    CHECK(coap_end_response(msg, nullptr /* ack_cb */, requestErrorCallback, nullptr /* arg */, nullptr /* reserved */));
    destroyMsgGuard.dismiss();
    return 0;
}

void LedgerManager::ledgerUpdated(void* context) {
    std::lock_guard lock(mutex_);
    auto ctx = static_cast<LedgerContext*>(context);
    if (ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD || ctx->syncDir == LEDGER_SYNC_DIRECTION_UNKNOWN) {
        // Mark the ledger as changed but only schedule a sync for it if its actual sync direction is known
        ctx->changed = true;
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN && state_ >= State::READY) {
            setPendingState(ctx, PendingState::SYNC_TO_CLOUD);
            updateSyncTime(ctx);
        }
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

void LedgerManager::updateSyncTime(LedgerContext* ctx) {
    auto now = hal_timer_millis(nullptr);
    if (!ctx->forcedSyncTime) {
        ctx->forcedSyncTime = now + MAX_SYNC_DELAY;
    }
    ctx->syncTime = std::min(now + MIN_SYNC_DELAY, ctx->forcedSyncTime);
    if (!nextSyncTime_) {
        nextSyncTime_ = ctx->syncTime;
    }
}

void LedgerManager::handleError(int error) {
    if (error < 0 && state_ >= State::READY) {
        LOG(ERROR, "Ledger error: %d", error);
        state_ = State::FAILED;
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

int LedgerManager::connectionCallback(int error, int status, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    int r = 0;
    switch (status) {
    case COAP_CONNECTION_OPEN: {
        r = self->notifyConnected();
        break;
    }
    case COAP_CONNECTION_CLOSED: {
        self->notifyDisconnected(error);
        break;
    }
    default:
        break;
    }
    if (r < 0) {
        LOG(ERROR, "Failed to handle connection status change: %d", error);
        self->handleError(r);
        // Do not propagate the error to the protocol layer to avoid disrupting the cloud connection
        // on ledger errors. The failed sync will be retried next time the device reconnects
    }
    return 0;
}

int LedgerManager::requestCallback(coap_message* msg, const char* uri, int method, int reqId, void* arg) {
    SCOPE_GUARD({
        coap_destroy_message(msg, nullptr);
    });
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    clear_system_error_message();
    int result = self->receiveRequest(msg, reqId);
    if (result < 0) {
        LOG(ERROR, "Failed to handle request: %d", result);
    }
    int r = self->sendResponse(result, reqId);
    if (r < 0 && r != SYSTEM_ERROR_COAP_REQUEST_NOT_FOUND) { // Response might have been sent already
        LOG(ERROR, "Failed to send response: %d", r);
    }
    if (result < 0) {
        self->handleError(result);
    }
    return 0;
}

int LedgerManager::responseCallback(coap_message* msg, int code, int reqId, void* arg) {
    SCOPE_GUARD({
        coap_destroy_message(msg, nullptr);
    });
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    int r = self->receiveResponse(msg, code, reqId);
    if (r < 0) {
        LOG(ERROR, "Failed to handle response: %d", r);
        self->handleError(r);
    }
    return 0;
}

void LedgerManager::requestErrorCallback(int error, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    if (reqId == self->reqId_) {
        LOG(ERROR, "Request failed: %d", error);
        self->handleError(error);
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

int Ledger::initWriter(LedgerWriter& writer, LedgerWriteSource src) {
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

int Ledger::updateInfo(const LedgerInfo& info) {
    std::lock_guard lock(*this);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
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

int Ledger::writerClosed(const LedgerInfo& info, LedgerWriteSource src, int tempSeqNum) {
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
    if (src == LedgerWriteSource::USER) {
        // Unlock the ledger before calling into the manager to avoid a deadlock
        lock.unlock();
        fs.unlock();
        LedgerManager::instance()->ledgerUpdated(context());
        fs.lock(); // FIXME
    } else if (syncCallback_) { // SYSTEM
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

void Ledger::setLedgerInfo(const LedgerInfo& info) {
    assert(info.hasScope() && info.hasSyncDirection() && info.hasDataSize() && info.hasLastUpdated() &&
            info.hasLastSynced() && info.hasSyncPending());
    scope_ = info.scope();
    syncDir_ = info.syncDirection();
    dataSize_ = info.dataSize();
    lastUpdated_ = info.lastUpdated();
    lastSynced_ = info.lastSynced();
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

int LedgerReader::rewind() {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FsLock fs;
    CHECK_FS(lfs_file_seek(fs.instance(), &file_, 0, LFS_SEEK_SET));
    dataOffs_ = 0;
    return 0;
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
        int64_t t = getCurrentTime();
        if (t < 0) {
            if (t != SYSTEM_ERROR_HAL_RTC_INVALID_TIME) {
                LOG(ERROR, "Failed to get current time: %d", (int)t);
            }
            t = 0; // Time is unknown
        }
        newInfo.lastUpdated(t);
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
