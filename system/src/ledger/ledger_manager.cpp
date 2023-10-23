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

#include "timer_hal.h"

#include "nanopb_misc.h"
#include "file_util.h"
#include "time_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "cloud/cloud.pb.h"
#include "cloud/ledger.pb.h"

#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_LEDGER(_name) particle_cloud_ledger_##_name

LOG_SOURCE_CATEGORY("system.ledger");

namespace particle {

using control::common::EncodedString;
using fs::FsLock;

namespace system {

namespace {

const unsigned MAX_LEDGER_COUNT = 20;

const unsigned MIN_SYNC_DELAY = 5000;
const unsigned MAX_SYNC_DELAY = 30000;

const auto REQUEST_URI = "L";
const auto REQUEST_METHOD = COAP_METHOD_POST;

const size_t STREAM_BUFFER_SIZE = 128;

const size_t MAX_PATH_LEN = 127;

int encodeSetDataRequestPrefix(pb_ostream_t* stream, const char* ledgerName, const LedgerInfo& info) {
    if (!pb_encode_tag(stream, PB_WT_STRING, PB_LEDGER(SetDataRequest_name_tag)) || // name
            !pb_encode_string(stream, (const pb_byte_t*)ledgerName, std::strlen(ledgerName))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    auto lastUpdated = info.lastUpdated();
    if (lastUpdated && (!pb_encode_tag(stream, PB_WT_64BIT, PB_LEDGER(SetDataRequest_last_updated_tag)) || // last_updated
            !pb_encode_fixed64(stream, &lastUpdated))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    if (!pb_encode_tag(stream, PB_WT_STRING, PB_LEDGER(SetDataRequest_data_tag)) || // data
            !pb_encode_varint(stream, info.dataSize())) {
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

inline int closeDir(lfs_t* fs, lfs_dir_t* dir) { // Transforms the LittleFS error to a system error
    CHECK_FS(lfs_dir_close(fs, dir));
    return 0;
}

} // namespace

namespace detail {

struct LedgerSyncContext {
    CString name; // Ledger name
    Ledger* instance; // Ledger instance. If null, the ledger is not instantiated
    ledger_sync_direction syncDir; // Sync direction
    int pendingState; // Pending state flags (see LedgerManager::PendingState)
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

    LedgerSyncContext() :
            instance(nullptr),
            syncDir(LEDGER_SYNC_DIRECTION_UNKNOWN),
            pendingState(0),
            taskRunning(false),
            syncTime(0),
            forcedSyncTime(0),
            changed(false) {
    }

    void resetDeviceToCloudState() {
        syncTime = 0;
        forcedSyncTime = 0;
        changed = false;
    }

    void resetCloudToDeviceState() {
        lastUpdated = 0;
    }
};

} // namespace detail

LedgerManager::LedgerManager() :
        curCtx_(nullptr),
        msg_(nullptr),
        nextSyncTime_(0),
        bytesInBuf_(0),
        state_(State::NEW),
        pendingState_(0),
        reqId_(COAP_INVALID_REQUEST_ID) {
}

LedgerManager::~LedgerManager() {
    if (state_ != State::NEW) {
        coap_remove_request_handler(REQUEST_URI, REQUEST_METHOD, nullptr);
        coap_remove_connection_handler(connectionCallback, nullptr);
    }
}

int LedgerManager::init() {
    if (state_ != State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // TODO: Allow seeking in ledger and CoAP message streams so that an intermediate buffer is not
    // needed when streaming ledger data to and from the server
    std::unique_ptr<char[]> buf(new char[STREAM_BUFFER_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // Enumerate local ledgers
    LedgerSyncContexts contexts;
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
            if (contexts.size() >= (int)MAX_LEDGER_COUNT) {
                LOG(ERROR, "Maximum number of ledgers exceeded, skipping ledger: %s", entry.name);
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
            std::unique_ptr<LedgerSyncContext> ctx(new(std::nothrow) LedgerSyncContext());
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
            if (!contexts.append(std::move(ctx))) {
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
            LOG(ERROR, "Maximum number of ledgers exceeded");
            return SYSTEM_ERROR_LEDGER_TOO_MANY;
        }
        // Create a new sync context
        newCtx.reset(new(std::nothrow) LedgerSyncContext());
        if (!newCtx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        newCtx->name = name;
        if (!newCtx->name) {
            return SYSTEM_ERROR_NO_MEMORY;
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
        }
    }
    ctx->instance = lr.get();
    ledger = std::move(lr);
    return 0;
}

int LedgerManager::getLedgerNames(Vector<CString>& names) {
    std::lock_guard lock(mutex_);
    if (state_ == State::NEW) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FsLock fs;
    Vector<CString> namesVec;
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
            if (!name || !namesVec.append(std::move(name))) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        CHECK_FS(r);
    } else if (r != LFS_ERR_NOENT) {
        CHECK_FS(r); // Forward the error
    }
    names = std::move(namesVec);
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
                LOG(TRACE, "Synchronizing ledger: %s", (const char*)ctx->name);
                CHECK(sendSetDataRequest(ctx));
                return 0;
            }
            nextSyncTime_ = t;
        }
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
    for (auto& ctx: contexts_) {
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
    reset();
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
        auto it = self->findContext(pbLedger.name, found);
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
    for (auto& ctx: contexts_) {
        setPendingState(ctx.get(), PendingState::GET_INFO);
    }
    return 0;
}

int LedgerManager::receiveResponse(coap_message* msg, int status) {
    assert(state_ > State::READY);
    auto codeClass = COAP_CODE_CLASS(status);
    if (codeClass != 2 && codeClass != 4) { // Success 2.xx or Client Error 4.xx
        LOG(ERROR, "Ledger request failed: %d.%02d", (int)codeClass, (int)COAP_CODE_DETAIL(status));
        return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
    }
    // Get the protocol-specific result code. XXX: It's assumed that the message fields are encoded
    // in order of their field numbers, which is not guaranteed by the Protobuf spec in general
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

int LedgerManager::receiveSetDataResponse(coap_message* msg, int result) {
    assert(state_ == State::SYNC_TO_CLOUD && curCtx_ && curCtx_->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD &&
            curCtx_->taskRunning);
    auto name = (const char*)curCtx_->name;
    if (result == 0) {
        LOG(TRACE, "Sent ledger data: %s", name);
        LedgerInfo info;
        auto now = CHECK(getMillisSinceEpoch());
        info.lastSynced(now);
        curCtx_->syncTime = 0;
        curCtx_->forcedSyncTime = 0;
        if (!(curCtx_->pendingState & PendingState::SYNC_TO_CLOUD)) {
            info.syncPending(false);
            curCtx_->changed = false;
        } else {
            // Ledger changed while being synchronized
            updateSyncTime(curCtx_);
        }
        RefCountPtr<Ledger> ledger;
        CHECK(getLedger(ledger, name));
        CHECK(ledger->updateInfo(info));
        ledger->notifySynced(); // TODO: Invoke asynchronously
        // TODO: Reorder the ledger entries so that they're synchronized in a round-robin fashion
    } else {
        LOG(ERROR, "Failed to sync ledger: %s; response result: %d", name, result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger is no longer accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", name);
        setPendingState(curCtx_, PendingState::GET_INFO | PendingState::SYNC_TO_CLOUD);
    }
    curCtx_->taskRunning = false;
    curCtx_ = nullptr;
    state_ = State::READY;
    return 0;
}

int LedgerManager::receiveGetDataResponse(coap_message* msg, int result) {
    assert(state_ == State::SYNC_FROM_CLOUD && curCtx_ && curCtx_->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE &&
            curCtx_->taskRunning && !stream_ && !msg_);
    auto name = (const char*)curCtx_->name;
    if (result != 0) {
        LOG(ERROR, "Failed to sync ledger: %s; response result: %d", name, result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Ledger is no longer accessible, re-request its info
        LOG(WARN, "Re-requesting ledger info: %s", name);
        setPendingState(curCtx_, PendingState::GET_INFO | PendingState::SYNC_FROM_CLOUD);
        curCtx_->taskRunning = false;
        curCtx_ = nullptr;
        state_ = State::READY;
        return 0;
    }
    LOG(TRACE, "Received ledger data: %s", name);
    // Open the ledger for writing
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, name));
    std::unique_ptr<LedgerWriter> writer(new(std::nothrow) LedgerWriter());
    CHECK(ledger->initWriter(*writer, LedgerWriteSource::SYSTEM));
    // Ledger data may span multiple CoAP messages. Nanopb streams are synchronous so the response
    // is decoded manually
    pb_istream_t pbStream = {};
    CHECK(getStreamForSubmessage(msg, &pbStream, PB_CLOUD(Response_ledger_get_data_tag)));
    uint64_t lastUpdated = 0;
    for (;;) {
        uint32_t tag = 0;
        auto type = pb_wire_type_t();
        bool eof = false;
        if (!pb_decode_tag(&pbStream, &type, &tag, &eof)) {
            if (!eof) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            break;
        }
        if (tag == PB_LEDGER(GetDataResponse_last_updated_tag)) {
            if (type != PB_WT_64BIT || !pb_decode_fixed64(&pbStream, &lastUpdated)) {
                return SYSTEM_ERROR_BAD_DATA;
            }
        } else if (tag == PB_LEDGER(GetDataResponse_data_tag)) {
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
    msg_ = msg;
    // Read the first chunk of the ledger data
    CHECK(receiveLedgerData());
    return 0;
}

int LedgerManager::receiveSubscribeResponse(coap_message* msg, int result) {
    assert(state_ == State::SUBSCRIBE);
    if (result != 0) {
        LOG(ERROR, "Failed to subscribe to ledger updates; response result: %d", result);
        if (result != PB_CLOUD(Response_Result_LEDGER_NOT_FOUND) &&
                result != PB_CLOUD(Response_Result_LEDGER_INVALID_SYNC_DIRECTION)) {
            return SYSTEM_ERROR_LEDGER_REQUEST_FAILED;
        }
        // Some of the cloud-to-device ledgers are no longer accessible, re-request their info
        LOG(WARN, "Re-requesting ledger info");
        for (auto& ctx: contexts_) {
            if (ctx->taskRunning) {
                setPendingState(ctx.get(), PendingState::GET_INFO);
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
        LOG(TRACE, "Ledger: %s", pbLedger.name);
        bool found = false;
        auto it = d->self->findContext(pbLedger.name, found);
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
    for (auto& ctx: contexts_) {
        if (ctx->taskRunning) {
            LOG(ERROR, "Missing subscription: %s", (const char*)ctx->name);
            return SYSTEM_ERROR_LEDGER_INVALID_RESPONSE;
        }
    }
    state_ = State::READY;
    return 0;
}

int LedgerManager::receiveGetInfoResponse(coap_message* msg, int result) {
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
        d->error = d->self->getLedger(ledger, pbLedger.name);
        if (d->error < 0) {
            return false;
        }
        auto ctx = ledger->syncContext();
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
                    ctx->resetDeviceToCloudState();
                }
                d->localInfoIsInvalid = true; // Will cause a transition to the failed state
            } else if (newInfo.syncDirection() == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // UNKNOWN -> CLOUD_TO_DEVICE
                if (localInfo.syncPending()) {
                    LOG(WARN, "Ledger has local changes but its actual sync direction is cloud-to-device: %s", pbLedger.name);
                    newInfo.syncPending(false);
                    newInfo.lastUpdated(0);
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
            ctx->syncDir = newInfo.syncDirection();
        }
        return true;
    };
    pb_istream_t stream = {};
    CHECK(getStreamForSubmessage(msg, &stream, PB_CLOUD(Response_ledger_get_info_tag)));
    if (!pb_decode(&stream, &PB_LEDGER(GetInfoResponse_msg), &pbResp)) {
        return (d.error < 0) ? d.error : SYSTEM_ERROR_BAD_DATA;
    }
    for (auto& ctx: contexts_) {
        if (ctx->taskRunning) {
            // Ledger doesn't exist or is no longer accessible by the device
            LOG(WARN, "Ledger not found: %s", (const char*)ctx->name);
            if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN) { // DEVICE_TO_CLOUD/CLOUD_TO_DEVICE -> UNKNOWN
                LedgerInfo info;
                info.syncDirection(LEDGER_SYNC_DIRECTION_UNKNOWN);
                info.scope(LEDGER_SCOPE_UNKNOWN);
                RefCountPtr<Ledger> ledger;
                CHECK(getLedger(ledger, ctx->name));
                CHECK(ledger->updateInfo(info));
                if (ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) { // CLOUD_TO_DEVICE -> UNKNOWN
                    ctx->resetDeviceToCloudState();
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
    state_ = State::READY;
    return 0;
}

int LedgerManager::sendSetDataRequest(LedgerSyncContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_TO_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD && !curCtx_ && !stream_ && !msg_);
    auto name = (const char*)ctx->name;
    // Open the ledger for reading
    RefCountPtr<Ledger> ledger;
    CHECK(getLedger(ledger, name));
    std::unique_ptr<LedgerReader> reader(new(std::nothrow) LedgerReader());
    CHECK(ledger->initReader(*reader));
    auto info = reader->info();
    stream_.reset(reader.release());
    NAMED_SCOPE_GUARD(destroyStreamGuard, {
        stream_.reset();
    });
    // Create a request message
    reqId_ = CHECK(coap_begin_request(&msg_, REQUEST_URI, REQUEST_METHOD, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    NAMED_SCOPE_GUARD(destroyMsgGuard, {
        coap_destroy_message(msg_, nullptr);
        msg_ = nullptr;
        reqId_ = COAP_INVALID_REQUEST_ID;
    });
    // Ledger data may not fit in a single CoAP message. Nanopb streams are synchronous so the
    // request is encoded manually
    pb_ostream_t pbStream = PB_OSTREAM_SIZING;
    // Calculate the size of the request's submessage (particle.cloud.ledger.SetDataRequest)
    CHECK(encodeSetDataRequestPrefix(&pbStream, name, info));
    size_t submsgSize = pbStream.bytes_written + info.dataSize();
    // Encode a request message (particle.cloud.Request)
    CHECK(pb_ostream_from_coap_message(&pbStream, msg_, nullptr));
    if (!pb_encode_tag(&pbStream, PB_WT_VARINT, PB_CLOUD(Request_type_tag)) || // type
            !pb_encode_varint(&pbStream, PB_CLOUD(Request_Type_LEDGER_SET_DATA))) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    if (!pb_encode_tag(&pbStream, PB_WT_STRING, PB_CLOUD(Request_ledger_set_data_tag)) || // ledger_set_data
            !pb_encode_varint(&pbStream, submsgSize)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(encodeSetDataRequestPrefix(&pbStream, name, info));
    // Encode and send a first chunk of the ledger data
    CHECK(sendLedgerData());
    destroyMsgGuard.dismiss();
    destroyStreamGuard.dismiss();
    // Clear the pending state
    clearPendingState(ctx, PendingState::SYNC_TO_CLOUD);
    ctx->taskRunning = true;
    curCtx_ = ctx;
    state_ = State::SYNC_TO_CLOUD;
    return 0;
}

int LedgerManager::sendGetDataRequest(LedgerSyncContext* ctx) {
    assert(state_ == State::READY && (ctx->pendingState & PendingState::SYNC_FROM_CLOUD) &&
            ctx->syncDir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE && !curCtx_);
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
    curCtx_ = ctx;
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
        for (auto& ctx: self->contexts_) {
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
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_GET_INFO);
    pbReq.which_data = PB_CLOUD(Request_ledger_get_info_tag);
    pbReq.data.ledger_get_info.ledgers.arg = this;
    pbReq.data.ledger_get_info.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        // Make sure not to update any state in this callback as it may be called multiple times
        auto self = (const LedgerManager*)*arg;
        for (auto& ctx: self->contexts_) {
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
    for (auto& ctx: contexts_) {
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

int LedgerManager::sendLedgerData() {
    assert(stream_ && coapMsg_);
    bool eof = false;
    for (;;) {
        if (bytesInBuf_ > 0) {
            size_t size = bytesInBuf_;
            int r = CHECK(coap_write_payload(msg_, buf_.get(), &size, messageBlockCallback, requestErrorCallback, this, nullptr));
            if (r == COAP_RESULT_WAIT_BLOCK) {
                assert(size < bytesInBuf_);
                bytesInBuf_ -= size;
                std::memmove(buf_.get(), buf_.get() + size, bytesInBuf_);
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
        CHECK(coap_end_request(msg_, responseCallback, nullptr /* ack_cb */, requestErrorCallback, this, nullptr));
        msg_ = nullptr;
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
        int r = coap_read_payload(msg_, buf_.get(), &size, messageBlockCallback, requestErrorCallback, this, nullptr);
        if (r < 0) {
            if (r == SYSTEM_ERROR_END_OF_STREAM) {
                eof = true;
                break;
            }
            return r;
        }
        bytesInBuf_ = size;
        if (r == COAP_RESULT_WAIT_BLOCK) {
            break;
        }
    }
    if (eof) {
        auto now = CHECK(getMillisSinceEpoch());
        auto writer = static_cast<LedgerWriter*>(stream_.get());
        writer->updateInfo(LedgerInfo().lastSynced(now));
        auto ledger = writer->ledger();
        CHECK(stream_->close());
        stream_.reset();
        coap_destroy_message(msg_, nullptr);
        msg_ = nullptr;
        reqId_ = COAP_INVALID_REQUEST_ID;
        curCtx_->taskRunning = false;
        curCtx_ = nullptr;
        state_ = State::READY;
        ledger->notifySynced(); // TODO: Invoke asynchronously
    }
    return 0;
}

int LedgerManager::sendResponse(int result, int reqId) {
    coap_message* msg = nullptr;
    int code = (result == 0) ? COAP_STATUS_CHANGED : COAP_STATUS_BAD_REQUEST;
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

void LedgerManager::updateSyncTime(LedgerSyncContext* ctx) {
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
        reset();
        state_ = State::FAILED;
        // TODO: Try recovering after a delay rather than next time the device connects to the cloud
    }
}

void LedgerManager::reset() {
    if (msg_) {
        coap_destroy_message(msg_, nullptr);
        msg_ = nullptr;
    }
    if (reqId_ != COAP_INVALID_REQUEST_ID) {
        coap_cancel_request(reqId_, nullptr);
        reqId_ = COAP_INVALID_REQUEST_ID;
    }
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
        ctx->pendingState = 0;
        ctx->taskRunning = false;
    }
    pendingState_ = 0;
    nextSyncTime_ = 0;
    bytesInBuf_ = 0;
    curCtx_ = nullptr;
}

LedgerManager::LedgerSyncContexts::ConstIterator LedgerManager::findContext(const char* name, bool& found) const {
    found = false;
    auto it = std::lower_bound(contexts_.begin(), contexts_.end(), name, [&found](const auto& ctx, const char* name) {
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
        ctx->changed = true;
        if (ctx->syncDir != LEDGER_SYNC_DIRECTION_UNKNOWN && state_ >= State::READY) {
            setPendingState(ctx, PendingState::SYNC_TO_CLOUD);
            updateSyncTime(ctx);
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
        if (result >= 0) {
            result = r;
        }
    }
    if (result < 0) {
        self->handleError(result);
    }
    return 0;
}

int LedgerManager::responseCallback(coap_message* msg, int status, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    assert(!self->msg_ && self->reqId_ == reqId);
    SCOPE_GUARD({
        if (!self->msg_) {
            coap_destroy_message(msg, nullptr);
            self->reqId_ = COAP_INVALID_REQUEST_ID;
        } // else: Receiving a blockwise response
    });
    int r = self->receiveResponse(msg, status);
    if (r < 0) {
        LOG(ERROR, "Failed to handle response: %d", r);
        self->handleError(r);
    }
    return 0;
}

int LedgerManager::messageBlockCallback(coap_message* msg, int reqId, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    assert(self->msg_ == msg && self->reqId_ == reqId);
    int r = 0;
    if (self->state_ == State::SYNC_TO_CLOUD) {
        r = self->sendLedgerData();
    } else if (self->state_ == State::SYNC_FROM_CLOUD) {
        r = self->receiveLedgerData();
    }
    if (r < 0) {
        self->handleError(r);
    }
    return r;
}

void LedgerManager::requestErrorCallback(int error, int /* reqId */, void* arg) {
    auto self = static_cast<LedgerManager*>(arg);
    std::lock_guard lock(self->mutex_);
    LOG(ERROR, "Request failed: %d", error);
    self->handleError(error);
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

} // namespace system

} // namespace particle

#endif // HAL_PLATFORM_LEDGER
