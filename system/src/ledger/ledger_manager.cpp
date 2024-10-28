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
#include <mutex>
#include <cstring>
#include <cassert>

#include <pb_decode.h>

#include "control/common.h" // FIXME: Move Protobuf utilities to another directory
#include "ledger.h"
#include "ledger_manager.h"
#include "ledger_util.h"
#include "system_ledger.h"
#include "system_cloud.h"

#include "timer_hal.h"

#include "nanopb_misc.h"
#include "file_util.h"
#include "time_util.h"
#include "endian_util.h"
#include "str_compat.h"
#include "scope_guard.h"
#include "check.h"

#include "cloud/cloud.pb.h"
#include "cloud/ledger.pb.h"

#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_LEDGER(_name) particle_cloud_ledger_##_name

static_assert(PB_CLOUD(Response_Result_OK) == 0); // Used by value in the code

namespace particle {

using control::common::EncodedString;
using fs::FsLock;

namespace system {

namespace {

const unsigned MAX_LEDGER_COUNT = 20;

const unsigned MIN_SYNC_DELAY = 5000;
const unsigned MAX_SYNC_DELAY = 30000;

const unsigned MIN_RETRY_DELAY = 30000;
const unsigned MAX_RETRY_DELAY = 5 * 60000;

const auto REQUEST_URI = "L";
const auto REQUEST_METHOD = COAP_METHOD_POST;

const size_t STREAM_BUFFER_SIZE = 128;

const size_t MAX_PATH_LEN = 127;

int encodeSetDataRequestPrefix(pb_ostream_t* stream, const char* ledgerName, const LedgerInfo& info) {
    // Ledger data may not fit in a single CoAP message. Nanopb streams are synchronous so the
    // request is encoded manually
    if (!pb_encode_tag(stream, PB_WT_STRING, PB_LEDGER(SetDataRequest_name_tag)) || // name
            !pb_encode_string(stream, (const pb_byte_t*)ledgerName, std::strlen(ledgerName))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    auto& scopeId = info.scopeId();
    if (scopeId.size && (!pb_encode_tag(stream, PB_WT_STRING, PB_LEDGER(SetDataRequest_scope_id_tag)) || // scope_id
            !pb_encode_string(stream, (const pb_byte_t*)scopeId.data, scopeId.size))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    auto lastUpdated = info.lastUpdated();
    if (lastUpdated && (!pb_encode_tag(stream, PB_WT_64BIT, PB_LEDGER(SetDataRequest_last_updated_tag)) || // last_updated
            !pb_encode_fixed64(stream, &lastUpdated))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    auto dataSize = info.dataSize();
    // Encode only the tag and size of the data field. The data itself is encoded by the calling code
    if (dataSize && (!pb_encode_tag(stream, PB_WT_STRING, PB_LEDGER(SetDataRequest_data_tag)) || // data
            !pb_encode_varint(stream, dataSize))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    return 0;
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

// Returns true if the result code returned by the server indicates that the ledger is or may no
// longer be accessible by the device
inline bool isLedgerAccessError(int result) {
    return result == PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) ||
            result == PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION) ||
            result == PB_CLOUD(Response_Result_LEDGER_SCOPE_CHANGED);
}

inline int closeDir(lfs_t* fs, lfs_dir_t* dir) { // Transforms the LittleFS error to a system error
    CHECK_FS(lfs_dir_close(fs, dir));
    return 0;
}

} // namespace

namespace detail {

struct LedgerSyncContext {
    char name[LEDGER_MAX_NAME_LENGTH + 1]; // Ledger name
    LedgerScopeId scopeId; // Scope ID
    Ledger* instance; // Ledger instance. If null, the ledger is not instantiated
    ledger_sync_direction syncDir; // Sync direction
    int getInfoCount; // Number of GET_INFO requests sent for this ledger
    int pendingState; // Pending state flags (LedgerManager::PendingState)
    bool syncPending; // Whether the ledger needs to be synchronized
    bool taskRunning; // Whether an asynchronous task is running for this ledger
    union {
        struct { // Fields specific to a device-to-cloud ledger or a ledger with unknown sync direction
            uint64_t syncTime; // Time when the ledger should be synchronized (ticks)
            uint64_t forcedSyncTime; // The latest time when the ledger should be synchronized (ticks)
            uint64_t updateTime; // Time the ledger was last updated (ticks)
            unsigned updateCount; // Value of the ledger's update counter when the sync started
        };
        struct { // Fields specific to a cloud-to-device ledger
            uint64_t lastUpdated; // Time the ledger was last updated (Unix time in milliseconds)
        };
    };

    LedgerSyncContext() :
            name(),
            scopeId(EMPTY_LEDGER_SCOPE_ID),
            instance(nullptr),
            syncDir(LEDGER_SYNC_DIRECTION_UNKNOWN),
            getInfoCount(0),
            pendingState(0),
            syncPending(false),
            taskRunning(false),
            syncTime(0),
            forcedSyncTime(0),
            updateTime(0),
            updateCount(0) {
    }

    void updateFromLedgerInfo(const LedgerInfo& info) {
        if (info.isSyncDirectionSet()) {
            syncDir = info.syncDirection();
        }
        if (info.isScopeIdSet()) {
            scopeId = info.scopeId();
        }
        if (info.isSyncPendingSet()) {
            syncPending = info.syncPending();
        }
        if (info.isLastUpdatedSet() && syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            lastUpdated = info.lastUpdated();
        }
    }

    void resetDeviceToCloudState() {
        syncTime = 0;
        forcedSyncTime = 0;
        updateTime = 0;
        updateCount = 0;
    }

    void resetCloudToDeviceState() {
        lastUpdated = 0;
    }
};

} // namespace detail

LedgerManager::LedgerManager() :
        timer_(timerCallback, this),
        curCtx_(nullptr),
        nextSyncTime_(0),
        retryTime_(0),
        retryDelay_(0),
        bytesInBuf_(0),
        state_(State::NEW),
        pendingState_(0),
        reqId_(COAP_INVALID_REQUEST_ID),
        resubscribe_(false) {
}

LedgerManager::~LedgerManager() {
    reset();
    if (state_ != State::NEW) {
        coap_remove_request_handler(REQUEST_URI, REQUEST_METHOD, nullptr);
        coap_remove_connection_handler(connectionCallback, nullptr);
    }
}

int LedgerManager::init() {
    std::lock_guard lock(mutex_);
    if (state_ != State::NEW) {
        return 0; // Already initialized
    }
    // Device must not be connected to the cloud
    if (spark_cloud_flag_connected()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // TODO: Allow seeking in ledger and CoAP message streams so that an intermediate buffer is not
    // needed when streaming ledger data to and from the server
    std::unique_ptr<char[]> buf(new char[STREAM_BUFFER_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto fsInstance = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr);
    assert(fsInstance);
    FsLock fs(fsInstance);
    CHECK(filesystem_mount(fsInstance));
    // Enumerate local ledgers
    LedgerSyncContexts contexts;
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
            if (contexts.size() >= (int)MAX_LEDGER_COUNT) {
                LOG(ERROR, "Maximum number of ledgers reached, skipping ledger: %s", entry.name);
                continue;
            }
            // Load the ledger info
            Ledger ledger;
            int r = ledger.init(entry.name);
            if (r < 0) {
                LOG(ERROR, "Failed to initialize ledger: %d", r);
                continue;
            }
            // Create a sync context for the ledger
            std::unique_ptr<LedgerSyncContext> ctx(new(std::nothrow) LedgerSyncContext());
            if (!ctx) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            size_t n = strlcpy(ctx->name, entry.name, sizeof(ctx->name));
            if (n >= sizeof(ctx->name)) {
                return SYSTEM_ERROR_INTERNAL; // Name length is validated in Ledger::init()
            }
            auto info = ledger.info();
            ctx->scopeId = info.scopeId();
            ctx->syncDir = info.syncDirection();
            ctx->syncPending = info.syncPending();
            if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
                ctx->lastUpdated = info.lastUpdated();
            }
            // Context objects are sorted by ledger name
            bool found = false;
            auto it = findContext(contexts, ctx->name, found);
            assert(!found);
            it = contexts.insert(it, std::move(ctx));
            if (it == contexts.end()) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        CHECK_FS(r);
    } else if (r != LFS_ERR_NOENT) {
        CHECK_FS(r); // Forward the error
    }
    CHECK(coap_add_connection_handler(connectionCallback, this, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(removeConnHandler, {
        coap_remove_connection_handler(connectionCallback, nullptr /* reserved */);
    });
    CHECK(coap_add_request_handler(REQUEST_URI, REQUEST_METHOD, 0 /* flags */, requestCallback, this, nullptr /* reserved */));
    removeConnHandler.dismiss();
    contexts_ = std::move(contexts);
    buf_ = std::move(buf);
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
    auto it = findContext(name, found);
    if (found && (*it)->instance) {
        ledger = (*it)->instance;
        return 0;
    }
    std::unique_ptr<LedgerSyncContext> newCtx;
    LedgerSyncContext* ctx = nullptr;
    if (!found) {
        if (!create) {
            return SYSTEM_ERROR_LEDGER_NOT_FOUND;
        }
        if (contexts_.size() >= (int)MAX_LEDGER_COUNT) {
            LOG(ERROR, "Maximum number of ledgers reached");
            return SYSTEM_ERROR_LEDGER_TOO_MANY;
        }
        // Create a new sync context
        newCtx.reset(new(std::nothrow) LedgerSyncContext());
        if (!newCtx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        size_t n = strlcpy(newCtx->name, name, sizeof(newCtx->name));
        if (n >= sizeof(newCtx->name)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        ctx = newCtx.get();
    } else {
        ctx = it->get();
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
        it = contexts_.insert(it, std::move(newCtx));
        if (it == contexts_.end()) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        if (state_ >= State::READY) {
            // Request the info about the newly created ledger
            setPendingState(ctx, PendingState::GET_INFO);
            runNext();
        }
    }
    ctx->instance = lr.get();
    ledger = std::move(lr);
    return 0;
}

int LedgerManager::getLedgerNames(Vector<CString>& namesArg) {
    std::lock_guard lock(mutex_);
    if (state_ == State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Return all ledgers found in the filesystem, not just the usable ones
    FsLock fs;
    Vector<CString> names;
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
            CString name(entry.name);
            if (!name || !names.append(std::move(name))) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        CHECK_FS(r);
    } else if (r != LFS_ERR_NOENT) {
        CHECK_FS(r); // Forward the error
    }
    namesArg = std::move(names);
    return 0;
}

int LedgerManager::removeLedgerData(const char* name) {
    if (!*name) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(mutex_);
    // TODO: Allow removing ledgers regardless of the device state or if the given ledger is in use
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool found = false;
    auto it = findContext(name, found);
    if (found) {
        if ((*it)->instance) {
            return SYSTEM_ERROR_LEDGER_IN_USE;
        }
        contexts_.erase(it);
    }
    char path[MAX_PATH_LEN + 1];
    CHECK(formatLedgerPath(path, sizeof(path), name));
    CHECK(rmrf(path));
    return 0;
}

int LedgerManager::removeAllData() {
    std::lock_guard lock(mutex_);
    // TODO: Allow removing ledgers regardless of the device state or if the given ledger is in use
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    for (auto& ctx: contexts_) {
        if (ctx->instance) {
            return SYSTEM_ERROR_LEDGER_IN_USE;
        }
    }
    contexts_.clear();
    CHECK(rmrf(LEDGER_ROOT_DIR));
    return 0;
}

int LedgerManager::run() {
    auto now = hal_timer_millis(nullptr);
    if (state_ == State::FAILED) {
        if (now >= retryTime_) {
            LOG(INFO, "Retrying synchronization");
            startSync();
            return 1; // Have a task to run
        }
        runNext(retryTime_ - now);
        return 0;
    }
    if (state_ != State::READY || (!pendingState_ && !resubscribe_)) {
        // Some task is in progress, the manager is not in an appropriate state, or there's nothing to do
        return 0;
    }
    if (pendingState_ & PendingState::GET_INFO) {
        LOG(INFO, "Requesting ledger info");
        CHECK(sendGetInfoRequest());
        return 1;
    }
    if ((pendingState_ & PendingState::SUBSCRIBE) || resubscribe_) {
        LOG(INFO, "Subscribing to ledger updates");
        CHECK(sendSubscribeRequest());
        return 1;
    }
    unsigned nextSyncDelay = 0;
    if (pendingState_ & PendingState::SYNC_TO_CLOUD) {
        if (now >= nextSyncTime_) {
            uint64_t t = 0;
            LedgerSyncContext* ctx = nullptr;
            for (auto& c: contexts_) {
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
                LOG(TRACE, "Synchronizing ledger: %s", ctx->name);
                CHECK(sendSetDataRequest(ctx));
                return 1;
            }
            // Timestamp in nextSyncTime_ was outdated
            nextSyncTime_ = t;
        }
        nextSyncDelay = nextSyncTime_ - now;
    }
    if (pendingState_ & PendingState::SYNC_FROM_CLOUD) {
        LedgerSyncContext* ctx = nullptr;
        for (auto& c: contexts_) {
            if (c->pendingState & PendingState::SYNC_FROM_CLOUD) {
                ctx = c.get();
                break;
            }
        }
        if (ctx) {
            LOG(TRACE, "Synchronizing ledger: %s", ctx->name);
            CHECK(sendGetDataRequest(ctx));
            return 1;
        }
    }
    if (nextSyncDelay > 0) {
        runNext(nextSyncDelay);
    }
    return 0;
}

int LedgerManager::notifyConnected() {
    if (state_ != State::OFFLINE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    LOG(TRACE, "Connected");
    startSync();
    return 0;
}

void LedgerManager::notifyDisconnected(int /* error */) {
    if (state_ == State::OFFLINE) {
        return;
    }
    if (state_ == State::NEW) {
        LOG_DEBUG(ERROR, "Unexpected manager state: %d", (int)state_);
        return;
    }
    LOG(TRACE, "Disconnected");
    reset();
    retryDelay_ = 0;
    state_ = State::OFFLINE;
}

int LedgerManager::receiveRequest(CoapMessagePtr& msg, int reqId) {
    if (state_ < State::READY) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Get the request type. XXX: It's assumed that the message fields are encoded in order of their
    // field numbers, which is not guaranteed by the Protobuf spec in general
    char buf[32] = {};
    size_t n = CHECK(coap_peek_block(msg.get(), buf, sizeof(buf), nullptr));
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
        LOG(ERROR, "Unknown request type: %d", (int)reqType);
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

int LedgerManager::receiveNotifyUpdateRequest(CoapMessagePtr& msg, int /* reqId */) {
    LOG(TRACE, "Received update notification");
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg.get(), &stream, PB_CLOUD(Request_ledger_notify_update_tag)));
    PB_LEDGER(NotifyUpdateRequest) pbReq = {};
    pbReq.ledgers.arg = this;
    pbReq.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto self = (LedgerManager*)*arg;
        PB_LEDGER(NotifyUpdateRequest_Ledger) pbLedger = {};
        if (!pb_decode(stream, &PB_LEDGER(NotifyUpdateRequest_Ledger_msg), &pbLedger)) {
            return false;
        }
        // Get the context of the updated ledger
        bool found = false;
        auto it = self->findContext(pbLedger.name, found);
        if (!found) {
            LOG(WARN, "Unknown ledger: %s", pbLedger.name);
            return true; // Ignore
        }
        auto ctx = it->get();
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            if (ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD) {
                LOG(ERROR, "Received update notification for device-to-cloud ledger: %s", ctx->name);
            } else {
                LOG(ERROR, "Received update notification for ledger with unknown sync direction: %s", ctx->name);
            }
            self->setPendingState(ctx, PendingState::GET_INFO);
            return true;
        }
        if (pbLedger.last_updated > ctx->lastUpdated) {
            // Schedule a sync for the updated ledger
            LOG(TRACE, "Ledger changed: %s", ctx->name);
            self->setPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
        }
        return true;
    };
    if (!pb_decode(&stream, &PB_LEDGER(NotifyUpdateRequest_msg), &pbReq)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int LedgerManager::receiveResetInfoRequest(CoapMessagePtr& /* msg */, int /* reqId */) {
    LOG(WARN, "Received a reset request, re-requesting ledger info");
    for (auto& ctx: contexts_) {
        setPendingState(ctx.get(), PendingState::GET_INFO);
    }
    return 0;
}

int LedgerManager::receiveResponse(CoapMessagePtr& msg, int status) {
    assert(state_ > State::READY);
    auto codeClass = COAP_CODE_CLASS(status);
    if (codeClass != 2 && codeClass != 4) { // Success 2.xx or Client Error 4.xx
        LOG(ERROR, "Ledger request failed: %d.%02d", (int)codeClass, (int)COAP_CODE_DETAIL(status));
        return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
    }
    // Get the protocol-specific result code. XXX: It's assumed that the message fields are encoded
    // in order of their field numbers, which is not guaranteed by the Protobuf spec in general
    char buf[32] = {};
    int r = coap_peek_block(msg.get(), buf, sizeof(buf), nullptr);
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
    if (codeClass != 2) {
        if (result == PB_CLOUD(Response_Result_OK)) {
            // CoAP response code indicates an error but the protocol-specific result code is OK
            return SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
        }
        // This is not necessarily a critical error
        LOG(WARN, "Ledger request failed: %d.%02d", (int)codeClass, (int)COAP_CODE_DETAIL(status));
    }
    switch (state_) {
    case State::SYNC_TO_CLOUD: {
        CHECK(receiveSetDataResponse(msg, result));
        break;
    }
    case State::SYNC_FROM_CLOUD: {
        CHECK(receiveGetDataResponse(msg, result));
        break;
    }
    case State::SUBSCRIBE: {
        CHECK(receiveSubscribeResponse(msg, result));
        break;
    }
    case State::GET_INFO: {
        CHECK(receiveGetInfoResponse(msg, result));
        break;
    }
    default:
        LOG(ERROR, "Unexpected response");
        return SYSTEM_ERROR_INTERNAL;
    }
    return 0;
}

int LedgerManager::receiveSetDataResponse(CoapMessagePtr& /* msg */, int result) {
    assert(state_ == State::SYNC_TO_CLOUD && curCtx_ && curCtx_->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD &&
            curCtx_->taskRunning);
    if (result == 0) {
        LOG(TRACE, "Sent ledger data: %s", curCtx_->name);
        LedgerInfo newInfo;
        auto now = CHECK(getMillisSinceEpoch());
        newInfo.lastSynced(now);
        RefCountPtr<Ledger> ledger;
        CHECK(getLedger(ledger, curCtx_->name));
        // Make sure the ledger can't be changed while we're updating its persistently stored state
        // and sync context
        std::unique_lock ledgerLock(*ledger);
        curCtx_->syncTime = 0;
        curCtx_->forcedSyncTime = 0;
        if (ledger->info().updateCount() == curCtx_->updateCount) {
            newInfo.syncPending(false);
            curCtx_->syncPending = false;
        } else {
            // Ledger changed while being synchronized
            assert(curCtx_->pendingState & PendingState::SYNC_TO_CLOUD);
            updateSyncTime(curCtx_);
        }
        CHECK(ledger->updateInfo(newInfo));
        ledgerLock.unlock();
        ledger->notifySynced(); // TODO: Invoke asynchronously
        // TODO: Reorder the ledger entries so that they're synchronized in a round-robin fashion
    } else {
        LOG(ERROR, "Failed to sync ledger: %s; result: %d", curCtx_->name, result);
        if (!isLedgerAccessError(result)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger may no longer be accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", curCtx_->name);
        setPendingState(curCtx_, PendingState::GET_INFO | PendingState::SYNC_TO_CLOUD);
    }
    curCtx_->taskRunning = false;
    curCtx_ = nullptr;
    state_ = State::READY;
    return 0;
}

int LedgerManager::receiveGetDataResponse(CoapMessagePtr& msg, int result) {
    assert(state_ == State::SYNC_FROM_CLOUD && curCtx_ && curCtx_->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE &&
            curCtx_->taskRunning && !stream_ && !msg_);
    if (result != 0) {
        LOG(ERROR, "Failed to sync ledger: %s; result: %d", curCtx_->name, result);
        if (!isLedgerAccessError(result)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger may no longer be accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", curCtx_->name);
        setPendingState(curCtx_, PendingState::GET_INFO | PendingState::SYNC_FROM_CLOUD);
        curCtx_->taskRunning = false;
        curCtx_ = nullptr;
        state_ = State::READY;
        return 0;
    }
    LOG(TRACE, "Received ledger data: %s", curCtx_->name);
    // Open the ledger for writing
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, curCtx_->name));
    std::unique_ptr<LedgerWriter> writer(new(std::nothrow) LedgerWriter());
    CHECK(ledger->initWriter(*writer, LedgerWriteSource::SYSTEM));
    // Ledger data may span multiple CoAP messages. Nanopb streams are synchronous so the response
    // is decoded manually
    pb_istream_t pbStream = {};
    CHECK(getStreamForSubmessage(msg.get(), &pbStream, PB_CLOUD(Response_ledger_get_data_tag)));
    uint64_t lastUpdated = 0;
    for (;;) {
        uint32_t fieldTag = 0;
        auto fieldType = pb_wire_type_t();
        bool eof = false;
        if (!pb_decode_tag(&pbStream, &fieldType, &fieldTag, &eof)) {
            if (!eof) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            break;
        }
        if (fieldTag == PB_LEDGER(GetDataResponse_last_updated_tag)) {
            if (fieldType != PB_WT_64BIT || !pb_decode_fixed64(&pbStream, &lastUpdated)) {
                return SYSTEM_ERROR_BAD_DATA;
            }
        } else if (fieldTag == PB_LEDGER(GetDataResponse_data_tag)) {
            if (!pb_skip_field(&pbStream, PB_WT_VARINT)) { // Skip the field length
                return SYSTEM_ERROR_BAD_DATA;
            }
            // "data" is always the last field in the message. XXX: It's assumed that the message
            // fields are encoded in order of their field numbers, which is not guaranteed by the
            // Protobuf spec in general
            break;
        }
    }
    writer->updateInfo(LedgerInfo().lastUpdated(lastUpdated));
    stream_.reset(writer.release());
    msg_ = std::move(msg);
    // Read the first chunk of the ledger data
    CHECK(receiveLedgerData());
    return 0;
}

int LedgerManager::receiveSubscribeResponse(CoapMessagePtr& msg, int result) {
    assert(state_ == State::SUBSCRIBE);
    if (result != 0) {
        LOG(ERROR, "Failed to subscribe to ledger updates; result: %d", result);
        if (!isLedgerAccessError(result)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Some of the ledgers may no longer be accessible, re-request the info for the ledgers we
        // tried to subscribe to
        for (auto& ctx: contexts_) {
            if (ctx->taskRunning) {
                LOG(WARN, "Re-requesting ledger info: %s", ctx->name);
                setPendingState(ctx.get(), PendingState::GET_INFO | PendingState::SUBSCRIBE);
                ctx->taskRunning = false;
            }
        }
        state_ = State::READY;
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
        LOG(TRACE, "Subscribed to ledger updates: %s", pbLedger.name);
        bool found = false;
        auto it = d->self->findContext(pbLedger.name, found);
        if (!found) {
            LOG(WARN, "Unknown ledger: %s", pbLedger.name);
            return true; // Ignore
        }
        auto ctx = it->get();
        if (!ctx->taskRunning) {
            LOG(ERROR, "Unexpected subscription: %s", ctx->name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        ctx->taskRunning = false;
        if ((pbLedger.has_last_updated && pbLedger.last_updated > ctx->lastUpdated) || ctx->syncPending) {
            d->self->setPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
        }
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg.get(), &stream, PB_CLOUD(Response_ledger_subscribe_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(SubscribeResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    for (auto& ctx: contexts_) {
        if (ctx->taskRunning) {
            LOG(ERROR, "Missing subscription: %s", ctx->name);
            return SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
        }
    }
    resubscribe_ = false;
    state_ = State::READY;
    return 0;
}

int LedgerManager::receiveGetInfoResponse(CoapMessagePtr& msg, int result) {
    assert(state_ == State::GET_INFO);
    if (result != 0) {
        LOG(ERROR, "Failed to get ledger info; result: %d", result);
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
        LOG(TRACE, "Received ledger info: name: %s; sync direction: %d; scope type: %d;", pbLedger.name,
                (int)pbLedger.sync_direction, (int)pbLedger.scope_type);
        LOG_PRINT(TRACE, "scope ID: ");
        LOG_DUMP(TRACE, pbLedger.scope_id.bytes, pbLedger.scope_id.size);
        LOG_PRINT(TRACE, "\r\n");
        RefCountPtr<Ledger> ledger;
        d->error = d->self->getLedger(ledger, pbLedger.name);
        if (d->error < 0) {
            return false;
        }
        auto ctx = ledger->syncContext();
        assert(ctx);
        if (!ctx->taskRunning) {
            LOG(ERROR, "Received unexpected ledger info: %s", ctx->name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        ctx->taskRunning = false;
        LedgerInfo newInfo;
        newInfo.syncDirection(static_cast<ledger_sync_direction>(pbLedger.sync_direction));
        newInfo.scopeType(static_cast<ledger_scope>(pbLedger.scope_type));
        if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_UNKNOWN || newInfo.scopeType() == LEDGER_SCOPE_UNKNOWN) {
            LOG(ERROR, "Received ledger info has invalid scope type or sync direction: %s", pbLedger.name);
            d->error = SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
            return false;
        }
        LedgerScopeId remoteScopeId = {};
        if (pbLedger.scope_id.size > sizeof(remoteScopeId.data)) {
            d->error = SYSTEM_ERROR_INTERNAL; // This should have been validated by nanopb
            return false;
        }
        std::memcpy(remoteScopeId.data, pbLedger.scope_id.bytes, pbLedger.scope_id.size);
        remoteScopeId.size = pbLedger.scope_id.size;
        newInfo.scopeId(remoteScopeId);
        auto localInfo = ledger->info();
        auto& localScopeId = localInfo.scopeId();
        bool scopeIdChanged = localScopeId != remoteScopeId;
        if (scopeIdChanged || localInfo.syncDirection() != newInfo.syncDirection() || localInfo.scopeType() != newInfo.scopeType()) {
            if (localInfo.syncDirection() != LEDGER_SYNC_DIRECTION_UNKNOWN) {
                if (scopeIdChanged) {
                    // Device was likely moved to another product which happens to have a ledger
                    // with the same name
                    LOG(WARN, "Ledger scope changed: %s", ctx->name);
                    d->self->clearPendingState(ctx, ctx->pendingState);
                    if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD) {
                        // Do not sync this ledger until it's updated again
                        newInfo.syncPending(false);
                        if (localInfo.syncDirection() == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
                            ctx->resetDeviceToCloudState();
                            d->resubscribe = true;
                        }
                    } else {
                        assert(newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE);
                        // Ignore the timestamps when synchronizing this ledger
                        newInfo.syncPending(true);
                        if (localInfo.syncDirection() == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD) {
                            ctx->resetCloudToDeviceState();
                        }
                        d->resubscribe = true;
                    }
                } else { // DEVICE_TO_CLOUD -> CLOUD_TO_DEVICE or vice versa
                    // This should not normally happen as the sync direction and scope type of an
                    // existing ledger cannot be changed
                    LOG(ERROR, "Ledger scope type or sync direction changed: %s", ctx->name);
                    newInfo.syncDirection(LEDGER_SYNC_DIRECTION_UNKNOWN);
                    newInfo.scopeType(LEDGER_SCOPE_UNKNOWN);
                    if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
                        ctx->resetDeviceToCloudState();
                    }
                    d->localInfoIsInvalid = true; // Will cause a transition to the failed state
                }
            } else if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // UNKNOWN -> CLOUD_TO_DEVICE
                if (localInfo.syncPending()) {
                    LOG(WARN, "Ledger has local changes but its actual sync direction is cloud-to-device: %s", ctx->name);
                    // Ignore the timestamps when synchronizing this ledger
                    newInfo.syncPending(true);
                }
                ctx->resetCloudToDeviceState();
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
            ctx->updateFromLedgerInfo(newInfo);
        }
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg.get(), &stream, PB_CLOUD(Response_ledger_get_info_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(GetInfoResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    for (auto& ctx: contexts_) {
        if (ctx->taskRunning) {
            // Ledger doesn't exist or is no longer accessible by the device
            LOG(WARN, "Ledger not found: %s", ctx->name);
            if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN) { // DEVICE_TO_CLOUD/CLOUD_TO_DEVICE -> UNKNOWN
                LedgerInfo info;
                info.syncDirection(LEDGER_SYNC_DIRECTION_UNKNOWN);
                info.scopeType(LEDGER_SCOPE_UNKNOWN);
                RefCountPtr<Ledger> ledger;
                CHECK(getLedger(ledger, ctx->name));
                CHECK(ledger->updateInfo(info));
                if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
                    ctx->resetDeviceToCloudState();
                    d.resubscribe = true;
                }
                ctx->updateFromLedgerInfo(info);
            }
            clearPendingState(ctx.get(), ctx->pendingState);
            ctx->taskRunning = false;
        } else if (d.resubscribe && ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            setPendingState(ctx.get(), PendingState::SUBSCRIBE);
        }
    }
    if (d.resubscribe) {
        // Make sure to clear the subscriptions on the server if no ledgers left to subscribe to.
        // TODO: Reuse pendingState_ for storing a state not associated with a particular sync context
        resubscribe_ = true;
    }
    if (d.localInfoIsInvalid) {
        return SYSTEM_ERROR_LEDGER_INCONSISTENT_STATE;
    }
    state_ = State::READY;
    return 0;
}

int LedgerManager::sendSetDataRequest(LedgerSyncContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_TO_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD && !curCtx_ && !stream_ && !msg_);
    // Open the ledger for reading
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, ctx->name));
    std::unique_ptr<LedgerReader> reader(new(std::nothrow) LedgerReader());
    CHECK(ledger->initReader(*reader));
    auto info = reader->info();
    // Create a request message
    coap_message* apiMsg = nullptr;
    int reqId = CHECK(coap_begin_request(&apiMsg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    // Calculate the size of the request's submessage (particle.cloud.ledger.SetDataRequest)
    pb_ostream_t pbStream = PB_OSTREAM_SIZING;
    CHECK(encodeSetDataRequestPrefix(&pbStream, ctx->name, info));
    size_t submsgSize = pbStream.bytes_written + info.dataSize();
    // Encode the outer request message (particle.cloud.Request)
    CHECK(pb_ostream_from_coap_message(&pbStream, msg.get(), nullptr));
    if (!pb_encode_tag(&pbStream, PB_WT_VARINT, PB_CLOUD(Request_type_tag)) || // type
            !pb_encode_varint(&pbStream, PB_CLOUD(Request_Type_LEDGER_SET_DATA))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    if (!pb_encode_tag(&pbStream, PB_WT_STRING, PB_CLOUD(Request_ledger_set_data_tag)) || // ledger_set_data
            !pb_encode_varint(&pbStream, submsgSize)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(encodeSetDataRequestPrefix(&pbStream, ctx->name, info));
    // Encode and send the first chunk of the ledger data
    stream_.reset(reader.release());
    reqId_ = reqId;
    msg_ = std::move(msg);
    CHECK(sendLedgerData());
    // Clear the pending state
    clearPendingState(ctx, PendingState::SYNC_TO_CLOUD);
    ctx->updateCount = info.updateCount();
    ctx->taskRunning = true;
    curCtx_ = ctx;
    state_ = State::SYNC_TO_CLOUD;
    return 0;
}

int LedgerManager::sendGetDataRequest(LedgerSyncContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_FROM_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE && !curCtx_);
    // Prepare a request message
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_GET_DATA);
    pbReq.which_data = PB_CLOUD(Request_ledger_get_data_tag);
    size_t n = strlcpy(pbReq.data.ledger_get_data.name, ctx->name, sizeof(pbReq.data.ledger_get_data.name));
    if (n >= sizeof(pbReq.data.ledger_get_data.name)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    if (ctx->scopeId.size > sizeof(pbReq.data.ledger_get_data.scope_id.bytes)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    std::memcpy(pbReq.data.ledger_get_data.scope_id.bytes, ctx->scopeId.data, ctx->scopeId.size);
    pbReq.data.ledger_get_data.scope_id.size = ctx->scopeId.size;
    // Send the request
    coap_message* apiMsg = nullptr;
    int reqId = CHECK(coap_begin_request(&apiMsg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg.get(), nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg.get(), responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    msg.release();
    // Clear the pending state
    clearPendingState(ctx, PendingState::SYNC_FROM_CLOUD);
    ctx->taskRunning = true;
    curCtx_ = ctx;
    reqId_ = reqId;
    state_ = State::SYNC_FROM_CLOUD;
    return 0;
}

int LedgerManager::sendSubscribeRequest() {
    assert(state_ == State::READY && (pendingState_ & PendingState::SUBSCRIBE));
    struct EncodeContext {
        LedgerManager* self;
        int error;
    };
    EncodeContext d = { .self = this, .error = 0 };
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_SUBSCRIBE);
    pbReq.which_data = PB_CLOUD(Request_ledger_subscribe_tag);
    pbReq.data.ledger_subscribe.ledgers.arg = &d;
    pbReq.data.ledger_subscribe.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        // Make sure not to update any state in this callback as it may be called multiple times
        auto d = (EncodeContext*)*arg;
        for (auto& ctx: d->self->contexts_) {
            if (ctx->pendingState & PendingState::SUBSCRIBE) {
                PB_LEDGER(SubscribeRequest_Ledger) pbLedger = {};
                size_t n = strlcpy(pbLedger.name, ctx->name, sizeof(pbLedger.name));
                if (n >= sizeof(pbLedger.name) || ctx->scopeId.size > sizeof(pbLedger.scope_id.bytes)) {
                    d->error = SYSTEM_ERROR_INTERNAL;
                    return false;
                }
                std::memcpy(pbLedger.scope_id.bytes, ctx->scopeId.data, ctx->scopeId.size);
                pbLedger.scope_id.size = ctx->scopeId.size;
                if (!pb_encode_tag_for_field(stream, field) ||
                        !pb_encode_submessage(stream, &PB_LEDGER(SubscribeRequest_Ledger_msg), &pbLedger)) {
                    return false;
                }
            }
        }
        return true;
    };
    coap_message* apiMsg = nullptr;
    int reqId = CHECK(coap_begin_request(&apiMsg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg.get(), nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg.get(), responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    msg.release();
    // Clear the pending state
    pendingState_ = 0;
    for (auto& ctx: contexts_) {
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
    struct EncodeContext {
        LedgerManager* self;
        int error;
    };
    EncodeContext d = { .self = this, .error = 0 };
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_GET_INFO);
    pbReq.which_data = PB_CLOUD(Request_ledger_get_info_tag);
    pbReq.data.ledger_get_info.ledgers.arg = &d;
    pbReq.data.ledger_get_info.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        // Make sure not to update any state in this callback as it may be called multiple times
        auto d = (EncodeContext*)*arg;
        for (auto& ctx: d->self->contexts_) {
            if (ctx->pendingState & PendingState::GET_INFO) {
                // This is to prevent sending GET_INFO requests in a loop if a subsequent SET_DATA or
                // SUBSCRIBE request keeps failing with a result code that triggers another GET_INFO
                // request. This can only happen due to a server error
                if (ctx->getInfoCount >= 10) {
                    LOG(ERROR, "Sent too many info requests for ledger: %s", ctx->name);
                    d->error = SYSTEM_ERROR_LEDGER_INCONSISTENT_STATE;
                    return false;
                }
                if (!pb_encode_tag_for_field(stream, field) ||
                        !pb_encode_string(stream, (const pb_byte_t*)ctx->name, std::strlen(ctx->name))) {
                    return false;
                }
            }
        }
        return true;
    };
    coap_message* apiMsg = nullptr;
    int reqId = CHECK(coap_begin_request(&apiMsg, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    pb_ostream_t stream = {};
    CHECK(pb_ostream_from_coap_message(&stream, msg.get(), nullptr));
    if (!pb_encode(&stream, &PB_CLOUD(Request_msg), &pbReq)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(msg.get(), responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
    msg.release();
    // Clear the pending state
    pendingState_ = 0;
    for (auto& ctx: contexts_) {
        if (ctx->pendingState & PendingState::GET_INFO) {
            ctx->pendingState &= ~PendingState::GET_INFO;
            ++ctx->getInfoCount;
            ctx->taskRunning = true;
        }
        pendingState_ |= ctx->pendingState;
    }
    reqId_ = reqId;
    state_ = State::GET_INFO;
    return 0;
}

int LedgerManager::sendLedgerData() {
    assert(stream_ && msg_);
    bool eof = false;
    for (;;) {
        if (bytesInBuf_ > 0) {
            size_t size = bytesInBuf_;
            int r = CHECK(coap_write_block(msg_.get(), buf_.get(), &size, messageBlockCallback, requestErrorCallback, this, nullptr));
            if (r == COAP_RESULT_WAIT_BLOCK) {
                assert(size < bytesInBuf_);
                bytesInBuf_ -= size;
                std::memmove(buf_.get(), buf_.get() + size, bytesInBuf_);
                LOG_DEBUG(TRACE, "Waiting current block of ledger data to be sent");
                break;
            }
            assert(size == bytesInBuf_);
            bytesInBuf_ = 0;
        }
        int r = stream_->read(buf_.get(), STREAM_BUFFER_SIZE);
        if (r < 0) {
            if (r == SYSTEM_ERROR_END_OF_STREAM) {
                eof = true;
                break;
            }
            return r;
        }
        assert(r != 0);
        bytesInBuf_ = r;
    }
    if (eof) {
        CHECK(stream_->close());
        stream_.reset();
        CHECK(coap_end_request(msg_.get(), responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
        msg_.release();
    }
    return 0;
}

int LedgerManager::receiveLedgerData() {
    assert(curCtx_ && stream_ && msg_);
    bool eof = false;
    for (;;) {
        if (bytesInBuf_ > 0) {
            CHECK(stream_->write(buf_.get(), bytesInBuf_));
            bytesInBuf_ = 0;
        }
        size_t size = STREAM_BUFFER_SIZE;
        int r = coap_read_block(msg_.get(), buf_.get(), &size, messageBlockCallback, requestErrorCallback, this, nullptr);
        if (r < 0) {
            if (r == SYSTEM_ERROR_END_OF_STREAM) {
                eof = true;
                break;
            }
            return r;
        }
        bytesInBuf_ = size;
        if (r == COAP_RESULT_WAIT_BLOCK) {
            LOG_DEBUG(TRACE, "Waiting next block of ledger data to be received");
            break;
        }
    }
    if (eof) {
        auto now = CHECK(getMillisSinceEpoch());
        auto writer = static_cast<LedgerWriter*>(stream_.get());
        LedgerInfo info;
        info.lastSynced(now);
        info.syncPending(false);
        writer->updateInfo(info);
        RefCountPtr<Ledger> ledger(writer->ledger());
        CHECK(stream_->close());
        stream_.reset();
        msg_.reset();
        reqId_ = COAP_INVALID_REQUEST_ID;
        curCtx_->taskRunning = false;
        curCtx_ = nullptr;
        state_ = State::READY;
        ledger->notifySynced(); // TODO: Invoke asynchronously
    }
    return 0;
}

int LedgerManager::sendResponse(int result, int reqId) {
    int code = (result == 0) ? COAP_STATUS_CHANGED : COAP_STATUS_BAD_REQUEST;
    coap_message* apiMsg = nullptr;
    CHECK(coap_begin_response(&apiMsg, code, reqId, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    PB_CLOUD(Response) pbResp = {};
    pbResp.result = result;
    EncodedString pbMsg(&pbResp.message);
    if (result < 0) {
        pbMsg.data = get_system_error_message(result);
        pbMsg.size = std::strlen(pbMsg.data);
    }
    CHECK(coap_end_response(msg.get(), nullptr /* ack_cb */, requestErrorCallback, nullptr /* arg */, nullptr /* reserved */));
    msg.release();
    return 0;
}

void LedgerManager::setPendingState(LedgerSyncContext* ctx, int state) {
    ctx->pendingState |= state;
    pendingState_ |= state;
}

void LedgerManager::clearPendingState(LedgerSyncContext* ctx, int state) {
    ctx->pendingState &= ~state;
    pendingState_ = 0;
    for (auto& ctx: contexts_) {
        pendingState_ |= ctx->pendingState;
    }
}

void LedgerManager::updateSyncTime(LedgerSyncContext* ctx, bool changed) {
    // TODO: Revisit the throttling algorithm
    auto now = hal_timer_millis(nullptr);
    bool throttle = ctx->updateTime && now - ctx->updateTime < MIN_SYNC_DELAY;
    if (changed) {
        ctx->updateTime = now;
    }
    if (throttle) {
        if (!ctx->forcedSyncTime) {
            ctx->forcedSyncTime = now + MAX_SYNC_DELAY;
        }
        ctx->syncTime = std::min(now + MIN_SYNC_DELAY, ctx->forcedSyncTime);
    } else {
        // Synchronize the first update in series right away
        ctx->syncTime = now;
        ctx->forcedSyncTime = now;
    }
    if (!nextSyncTime_ || ctx->syncTime < nextSyncTime_) {
        nextSyncTime_ = ctx->syncTime;
    }
}

void LedgerManager::startSync() {
    assert(state_ == State::OFFLINE || state_ == State::FAILED);
    for (auto& ctx: contexts_) {
        switch (ctx->syncDir) {
        case LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD: {
            if (ctx->syncPending) {
                setPendingState(ctx.get(), PendingState::SYNC_TO_CLOUD);
                updateSyncTime(ctx.get());
            }
            break;
        }
        case LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE: {
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
}

void LedgerManager::reset() {
    timer_.stop();
    if (reqId_ != COAP_INVALID_REQUEST_ID) {
        coap_cancel_request(reqId_, nullptr);
        reqId_ = COAP_INVALID_REQUEST_ID;
    }
    msg_.reset();
    if (stream_) {
        int r = stream_->close(true /* discard */);
        if (r < 0) {
            LOG(ERROR, "Failed to close ledger stream: %d", r);
        }
        stream_.reset();
    }
    for (auto& ctx: contexts_) {
        if (ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD || ctx->syncDir == LEDGER_SYNC_DIRECTION_UNKNOWN) {
            ctx->syncTime = 0;
            ctx->forcedSyncTime = 0;
        }
        ctx->getInfoCount = 0;
        ctx->pendingState = 0;
        ctx->taskRunning = false;
    }
    pendingState_ = 0;
    nextSyncTime_ = 0;
    bytesInBuf_ = 0;
    curCtx_ = nullptr;
    // retryDelay_ is reset when disconnecting from the cloud
}

void LedgerManager::runNext(unsigned delay) {
    int r = timer_.start(delay);
    if (r < 0) {
        LOG(ERROR, "Failed to start timer: %d", r);
    }
}

void LedgerManager::handleError(int error) {
    assert(error < 0);
    if (state_ >= State::READY) {
        retryDelay_ = std::clamp(retryDelay_ * 2, MIN_RETRY_DELAY, MAX_RETRY_DELAY);
        LOG(ERROR, "Synchronization failed: %d; retrying in %us", error, retryDelay_ / 1000);
        reset();
        retryTime_ = hal_timer_millis(nullptr) + retryDelay_;
        state_ = State::FAILED;
    }
}

LedgerManager::LedgerSyncContexts::ConstIterator LedgerManager::findContext(const LedgerSyncContexts& contexts, const char* name, bool& found) {
    found = false;
    auto it = std::lower_bound(contexts.begin(), contexts.end(), name, [&found](const auto& ctx, const char* name) {
        auto r = std::strcmp(ctx->name, name);
        if (r == 0) {
            found = true;
        }
        return r < 0;
    });
    return it;
}

void LedgerManager::notifyLedgerChanged(LedgerSyncContext* ctx) {
    std::lock_guard lock(mutex_);
    if (ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD || ctx->syncDir == LEDGER_SYNC_DIRECTION_UNKNOWN) {
        // Mark the ledger as changed but only schedule a sync for it if its actual sync direction is known
        ctx->syncPending = true;
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN && state_ >= State::READY) {
            setPendingState(ctx, PendingState::SYNC_TO_CLOUD);
            updateSyncTime(ctx, true /* changed */);
            runNext();
        }
    }
}

void LedgerManager::addLedgerRef(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    ++ledger->refCount();
}

void LedgerManager::releaseLedger(const LedgerBase* ledger) {
    std::lock_guard lock(mutex_);
    if (--ledger->refCount() == 0) {
        auto ctx = ledger->syncContext();
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
    }
    // Make sure to run any asynchronous tasks that might be pending whenever the manager is
    // notified about some event
    self->runNext();
    return 0;
}

int LedgerManager::requestCallback(coap_message* apiMsg, const char* uri, int method, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    clear_system_error_message();
    CoapMessagePtr msg(apiMsg);
    int result = self->receiveRequest(msg, reqId);
    if (result < 0) {
        LOG(ERROR, "Error while handling request: %d", result);
    }
    int r = self->sendResponse(result, reqId);
    if (r < 0 && r != SYSTEM_ERROR_COAP_REQUEST_NOT_FOUND) { // Response might have been sent already
        LOG(ERROR, "Failed to send response: %d", r);
        if (result >= 0) {
            result = r;
        }
    }
    if (result < 0) {
        self->handleError(result);
    }
    self->runNext();
    return 0;
}

int LedgerManager::responseCallback(coap_message* apiMsg, int status, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    CoapMessagePtr msg(apiMsg);
    int r = self->receiveResponse(msg, status);
    if (r < 0) {
        LOG(ERROR, "Error while handling response: %d", r);
        self->handleError(r);
    }
    self->runNext();
    return 0;
}

int LedgerManager::messageBlockCallback(coap_message* msg, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    assert(self->msg_.get() == msg && self->reqId_ == reqId);
    int r = 0;
    if (self->state_ == State::SYNC_TO_CLOUD) {
        r = self->sendLedgerData();
    } else if (self->state_ == State::SYNC_FROM_CLOUD) {
        r = self->receiveLedgerData();
    } else {
        LOG(ERROR, "Unexpected block message");
        r = SYSTEM_ERROR_INTERNAL;
    }
    if (r < 0) {
        self->handleError(r);
    }
    self->runNext();
    return 0;
}

void LedgerManager::requestErrorCallback(int error, int /* reqId */, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    LOG(ERROR, "Request failed: %d", error);
    self->handleError(error);
    self->runNext();
}

void LedgerManager::timerCallback(void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    int r = self->run();
    if (r < 0) {
        LOG(ERROR, "Failed to process ledger task: %d", r);
        self->handleError(r);
    }
    if (r != 0) {
        // Keep running until there's nothing to do
        self->runNext();
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    // XXX: Lazy initialization is used so that ledger instances can be requested in the global
    // scope by the application. It's dangerous because the application's global constructors are
    // called before the system is fully initialized but seems to work in this case
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        int r = mgr.init();
        if (r < 0) {
            LOG(ERROR, "Failed to initialize ledger manager: %d", r);
        }
    });
    return &mgr;
}

} // namespace system

} // namespace particle

#endif // HAL_PLATFORM_LEDGER
