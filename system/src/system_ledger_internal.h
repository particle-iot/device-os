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

#include <optional>
#include <utility>
#include <memory>
#include <mutex>
#include <cstdint>

#include "system_ledger.h"

#include "filesystem.h"

#include "static_recursive_mutex.h"
#include "c_string.h"
#include "ref_count.h"
#include "system_error.h"

#include "spark_wiring_vector.h"

namespace particle::system {

class Ledger;
class LedgerInfo;
class LedgerManager;
class LedgerStream;
class LedgerReader;
class LedgerWriter;

enum class LedgerWriteSource {
    USER,
    SYSTEM
};

// The reference counter of a ledger instance is managed by the LedgerManager. We can't safely use a
// regular atomic counter, such as RefCount, because the LedgerManager maintains a list of all created
// ledger instances that needs to be updated when any of the instances is destroyed. Shared pointers
// would work but those can't be used in dynalib interfaces
class LedgerBase {
public:
    explicit LedgerBase(void* ctx = nullptr) :
            ctx_(ctx),
            refCount_(1) {
    }

    LedgerBase(const LedgerBase&) = delete;

    virtual ~LedgerBase() = default;

    void addRef() const;
    void release() const;

    LedgerBase& operator=(const LedgerBase&) = delete;

protected:
    void* context() const { // Called by LedgerManager
        return ctx_;
    }

    int& refCount() const { // ditto
        return refCount_;
    }

private:
    void* ctx_;
    mutable int refCount_;

    friend class LedgerManager;
};

// TODO: Refactor and expose the CoAP API from the comms library and use that in the ledger code
class InboundConnectionHandler {
public:
    virtual ~InboundConnectionHandler() = default;

    virtual int connected() = 0;
    virtual int disconnected() = 0;
    virtual int requestSent(int reqId) = 0;
    virtual int responseSent(int reqId) = 0;
    virtual int requestReceived(int reqId, const char* data, size_t size, bool hasMore) = 0;
    virtual int responseReceived(int reqId, const char* data, size_t size, bool hasMore) = 0;
};

class OutboundConnectionHandler {
public:
    virtual ~OutboundConnectionHandler() = default;

    virtual int sendRequest(int reqId, const char* data, size_t size, bool hasMore) = 0;
    virtual int sendResponse(int reqId, const char* data, size_t size, bool hasMore) = 0;
};

class LedgerManager: public InboundConnectionHandler {
public:
    LedgerManager(const LedgerManager&) = delete;

    ~LedgerManager();

    int init(OutboundConnectionHandler* connHandler);

    int getLedger(const char* name, RefCountPtr<Ledger>& ledger);

    int removeLedgerData(const char* name);
    int removeAllData();

    int connected() override;
    int disconnected() override;
    int requestSent(int reqId) override;
    int responseSent(int reqId) override;
    int requestReceived(int reqId, const char* data, size_t size, bool hasMore) override;
    int responseReceived(int reqId, const char* data, size_t size, bool hasMore) override;

    void run();

    LedgerManager& operator=(const LedgerManager&) = delete;

    static LedgerManager* instance();

protected:
    void ledgerUpdated(Ledger* ledger); // Called by Ledger

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

    struct LedgerContext;
    typedef Vector<std::unique_ptr<LedgerContext>> LedgerContexts;

    LedgerContexts allLedgers_; // Preallocated context objects for all known ledgers
    OutboundConnectionHandler* connHandler_; // Connection handler

    std::unique_ptr<LedgerStream> stream_; // Input or output stream open for the ledger being synchronized
    LedgerContext* curLedger_; // Context of the ledger being synchronized

    uint64_t nextSyncTime_; // Nearest time when a device-to-cloud ledger needs to be synchronized
    uint64_t nextForcedSyncTime_; // Nearest time when a device-to-cloud ledger needs to be force-sync'd
    ledger_sync_direction lastSyncDir_; // Direction in which the last ledger was synchronized

    State state_; // Current state
    int pendingState_; // Pending state flags

    mutable StaticRecursiveMutex mutex_; // Manager lock

    LedgerManager(); // Use LedgerManager::instance()

    int connectedImpl();
    int disconnectedImpl();
    int runImpl();

    int sendSetDataRequest(LedgerContext* ctx);
    int sendGetDataRequest(LedgerContext* ctx);
    int sendSubscribeRequest();
    int sendGetInfoRequest();

    void setPendingState(LedgerContext* ctx, int state);
    void clearPendingState(LedgerContext* ctx, int state);

    LedgerContexts::ConstIterator findLedger(const char* name, bool& found) const;
    
    friend class Ledger;
    friend class LedgerBase;
};

class Ledger: public LedgerBase {
public:
    explicit Ledger(void* ctx = nullptr);
    ~Ledger();

    int initReader(LedgerReader& reader);
    int initWriter(LedgerWriteSource src, LedgerWriter& writer);

    // Returns a LedgerInfo object with all fields set
    LedgerInfo info() const;

    const char* name() const {
        return name_; // Immutable
    }

    void setSyncCallback(ledger_sync_callback callback) {
        std::lock_guard lock(*this);
        syncCallback_ = callback;
    }

    void setAppData(void* data, ledger_destroy_app_data_callback destroy) {
        std::lock_guard lock(*this);
        appData_ = data;
        destroyAppData_ = destroy;
    }

    void* appData() const {
        std::lock_guard lock(*this);
        return appData_;
    }

    void lock() const {
        mutex_.lock();
    }

    void unlock() const {
        mutex_.unlock();
    }

protected:
    int init(const char* name); // Called by LedgerManager

    int readerClosed(bool staged); // Called by LedgerReader
    int writerClosed(const LedgerInfo& info, LedgerWriteSource src, int tempSeqNum); // Called by LedgerWriter

private:
    int lastSeqNum_; // Counter incremented every time the ledger is opened for writing
    int stagedSeqNum_; // Sequence number assigned to the most recent staged ledger data
    int curReaderCount_; // Number of active readers of the current ledger data
    int stagedReaderCount_; // Number of active readers of the staged ledger data
    int stagedFileCount_; // Number of staged data files created

    int64_t lastUpdated_; // Last time the ledger was updated
    int64_t lastSynced_; // Last time the ledger was synchronized
    size_t dataSize_; // Size of the ledger data
    bool syncPending_; // Whether the ledger has local changes that have not yet been synchronized

    ledger_sync_callback syncCallback_; // Callback to invoke when the ledger has been synchronized
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data
    void* appData_; // Application data

    const char* name_; // Ledger name
    ledger_scope scope_; // Ledger scope
    ledger_sync_direction syncDir_; // Sync direction

    bool inited_; // Whether the ledger is initialized

    mutable StaticRecursiveMutex mutex_; // Ledger lock

    int loadLedgerInfo(lfs_t* fs);
    void updateLedgerInfo(const LedgerInfo& info);

    int initCurrentData(lfs_t* fs);
    int flushStagedData(lfs_t* fs);
    int removeTempData(lfs_t* fs);

    friend class LedgerManager;
    friend class LedgerReader;
    friend class LedgerWriter;
};

class LedgerInfo {
public:
    LedgerInfo() = default;

    LedgerInfo& scope(ledger_scope scope) {
        scope_ = scope;
        return *this;
    }

    ledger_scope scope() const {
        return scope_.value_or(LEDGER_SCOPE_UNKNOWN);
    }

    bool hasScope() const {
        return scope_.has_value();
    }

    LedgerInfo& syncDirection(ledger_sync_direction dir) {
        syncDir_ = dir;
        return *this;
    }

    ledger_sync_direction syncDirection() const {
        return syncDir_.value_or(LEDGER_SYNC_DIRECTION_UNKNOWN);
    }

    bool hasSyncDirection() const {
        return syncDir_.has_value();
    }

    LedgerInfo& dataSize(size_t size) {
        dataSize_ = size;
        return *this;
    }

    size_t dataSize() const {
        return dataSize_.value_or(0);
    }

    bool hasDataSize() const {
        return dataSize_.has_value();
    }

    LedgerInfo& lastUpdated(int64_t time) {
        lastUpdated_ = time;
        return *this;
    }

    int64_t lastUpdated() const {
        return lastUpdated_.value_or(0);
    }

    bool hasLastUpdated() const {
        return lastUpdated_.has_value();
    }

    LedgerInfo& lastSynced(int64_t time) {
        lastSynced_ = time;
        return *this;
    }

    int64_t lastSynced() const {
        return lastSynced_.value_or(0);
    }

    bool hasLastSynced() const {
        return lastSynced_.has_value();
    }

    LedgerInfo& syncPending(bool pending) {
        syncPending_ = pending;
        return *this;
    }

    bool syncPending() const {
        return syncPending_.value_or(false);
    }

    bool hasSyncPending() const {
        return syncPending_.has_value();
    }

    LedgerInfo& update(const LedgerInfo& info);

private:
    // When adding new fields, make sure to update LedgerInfo::update() and Ledger::updateLedgerInfo()
    std::optional<int64_t> lastUpdated_;
    std::optional<int64_t> lastSynced_;
    std::optional<size_t> dataSize_;
    std::optional<ledger_scope> scope_;
    std::optional<ledger_sync_direction> syncDir_;
    std::optional<bool> syncPending_;
};

class LedgerStream {
public:
    virtual ~LedgerStream() = default;

    virtual int read(char* data, size_t size) = 0;
    virtual int write(const char* data, size_t size) = 0;
    virtual int close(bool discard = false) = 0;
};

class LedgerReader: public LedgerStream {
public:
    LedgerReader() :
            file_(),
            dataSize_(0),
            dataOffs_(0),
            staged_(false),
            open_(false) {
    }

    // Reader instances are not copyable nor movable
    LedgerReader(const LedgerReader&) = delete;

    ~LedgerReader() {
        close(true /* discard */);
    }

    int read(char* data, size_t size) override;

    int write(const char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int close(bool discard = false) override;

    bool isOpen() const {
        return open_;
    }

    LedgerReader& operator=(const LedgerReader&) = default;

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    lfs_file_t file_; // File handle
    size_t dataSize_; // Size of the data section of the ledger file
    size_t dataOffs_; // Current offset in the data section
    bool staged_; // Whether the data being read is staged
    bool open_; // Whether the reader is open

    int init(size_t dataSize, int stagedSeqNum, Ledger* ledger); // Called by Ledger

    friend class Ledger;
};

class LedgerWriter: public LedgerStream {
public:
    LedgerWriter() :
            file_(),
            src_(),
            dataSize_(0),
            tempSeqNum_(0),
            open_(false) {
    }

    // Writer instances are not copyable nor movable
    LedgerWriter(const LedgerWriter&) = delete;

    ~LedgerWriter() {
        close(true /* discard */);
    }

    int read(char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int write(const char* data, size_t size) override;
    int close(bool discard = false) override;

    LedgerInfo& ledgerInfo() {
        return ledgerInfo_;
    }

    const LedgerInfo& ledgerInfo() const {
        return ledgerInfo_;
    }

    bool isOpen() const {
        return open_;
    }

    LedgerWriter& operator=(const LedgerWriter&) = delete;

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    LedgerInfo ledgerInfo_; // Ledger info updates
    lfs_file_t file_; // File handle
    LedgerWriteSource src_; // Who is writing to the ledger
    size_t dataSize_; // Size of the data written
    int tempSeqNum_; // Sequence number assigned to the temporary ledger data
    bool open_; // Whether the writer is open

    int init(LedgerWriteSource src, int tempSeqNum, Ledger* ledger); // Called by Ledger

    friend class Ledger;
};

inline void LedgerBase::addRef() const {
    LedgerManager::instance()->addLedgerRef(this);
}

inline void LedgerBase::release() const {
    LedgerManager::instance()->releaseLedger(this);
}

} // namespace particle::system

#endif // HAL_PLATFORM_LEDGER
