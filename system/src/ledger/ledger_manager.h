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

#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include <memory>
#include <cstdint>

#include "coap_api.h"

#include "static_recursive_mutex.h"
#include "ref_count.h"

#include "spark_wiring_vector.h"

namespace particle::system {

namespace detail {

class LedgerSyncContext;

} // namespace detail

class Ledger;
class LedgerBase;
class LedgerStream;

class LedgerManager {
public:
    LedgerManager(const LedgerManager&) = delete;

    ~LedgerManager();

    int init();

    int getLedger(RefCountPtr<Ledger>& ledger, const char* name, bool create = false);

    int removeLedgerData(const char* name);
    int removeAllData();

    void run();

    LedgerManager& operator=(const LedgerManager&) = delete;

    static LedgerManager* instance();

protected:
    using LedgerSyncContext = detail::LedgerSyncContext;

    void notifyLedgerChanged(LedgerSyncContext* ctx); // Called by Ledger

    void addLedgerRef(const LedgerBase* ledger); // Called by LedgerBase
    void releaseLedger(const LedgerBase* ledger); // ditto

private:
    enum class State {
        NEW, // Manager is not initialized
        FAILED, // Synchronization failed
        OFFLINE, // Device is offline
        READY, // Ready to run a task
        SYNC_TO_CLOUD, // Synchronizing a device-to-cloud ledger
        SYNC_FROM_CLOUD, // Synchronizing a cloud-to-device ledger
        SUBSCRIBE, // Subscribing to ledger updates
        GET_INFO // Getting ledger info
    };

    enum PendingState {
        SYNC_TO_CLOUD = 0x01, // Synchronization of a device-to-cloud ledger is pending
        SYNC_FROM_CLOUD = 0x02, // Synchronization of a cloud-to-device ledger is pending
        SUBSCRIBE = 0x04, // Subscription to updates is pending
        GET_INFO = 0x08 // Ledger info is missing
    };

    typedef Vector<std::unique_ptr<LedgerSyncContext>> LedgerSyncContexts;

    LedgerSyncContexts contexts_; // Preallocated context objects for all known ledgers

    std::unique_ptr<LedgerStream> stream_; // Input or output stream open for the ledger being synchronized
    LedgerSyncContext* curCtx_; // Context of the ledger being synchronized

    uint64_t nextSyncTime_; // Nearest time a device-to-cloud ledger needs to be synchronized

    State state_; // Current state
    int pendingState_; // Pending state flags
    int reqId_; // ID of the ongoing CoAP request

    mutable StaticRecursiveMutex mutex_; // Manager lock

    LedgerManager(); // Use LedgerManager::instance()

    int processTasks();

    int notifyConnected();
    void notifyDisconnected(int error);

    int receiveRequest(coap_message* msg, int reqId);
    int receiveNotifyUpdateRequest(coap_message* msg, int reqId);
    int receiveResetInfoRequest(coap_message* msg, int reqId);

    int receiveResponse(coap_message* msg, int code, int reqId);
    int receiveSetDataResponse(coap_message* msg, int result, int reqId);
    int receiveGetDataResponse(coap_message* msg, int result, int reqId);
    int receiveSubscribeResponse(coap_message* msg, int result, int reqId);
    int receiveGetInfoResponse(coap_message* msg, int result, int reqId);

    int sendSetDataRequest(LedgerSyncContext* ctx);
    int sendGetDataRequest(LedgerSyncContext* ctx);
    int sendSubscribeRequest();
    int sendGetInfoRequest();

    int sendResponse(int result, int reqId);

    void setPendingState(LedgerSyncContext* ctx, int state);
    void clearPendingState(LedgerSyncContext* ctx, int state);
    void updateSyncTime(LedgerSyncContext* ctx);

    void handleError(int error);

    LedgerSyncContexts::ConstIterator findContext(const char* name, bool& found) const;

    static int connectionCallback(int error, int status, void* arg);
    static int requestCallback(coap_message* msg, const char* uri, int method, int reqId, void* arg);
    static int responseCallback(coap_message* msg, int code, int reqId, void* arg);
    static void requestErrorCallback(int error, int reqId, void* arg);

    friend class Ledger;
    friend class LedgerBase;
};

} // namespace particle::system

#endif // HAL_PLATFORM_LEDGER
