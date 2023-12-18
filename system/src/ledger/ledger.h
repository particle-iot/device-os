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

#include <mutex>
#include <optional>
#include <cstring>
#include <cstdint>

#include "system_ledger.h"

#include "filesystem.h"
#include "static_recursive_mutex.h"
#include "ref_count.h"
#include "system_error.h"

namespace particle::system {

const auto LEDGER_ROOT_DIR = "/usr/ledger";

const size_t MAX_LEDGER_SCOPE_ID_SIZE = 32;

namespace detail {

class LedgerSyncContext;

} // namespace detail

class LedgerManager;
class LedgerReader;
class LedgerWriter;
class LedgerInfo;

enum class LedgerWriteSource {
    USER,
    SYSTEM
};

struct LedgerScopeId {
    char data[MAX_LEDGER_SCOPE_ID_SIZE];
    size_t size;
};

const LedgerScopeId EMPTY_LEDGER_SCOPE_ID = {};

// The reference counter of a ledger instance is managed by the LedgerManager. We can't safely use a
// regular atomic counter, such as RefCount, because the LedgerManager maintains a list of all created
// ledger instances that needs to be updated when any of the instances is destroyed. Shared pointers
// would work but those can't be used in dynalib interfaces
class LedgerBase {
public:
    explicit LedgerBase(detail::LedgerSyncContext* ctx = nullptr) :
            syncCtx_(ctx),
            refCount_(1) {
    }

    LedgerBase(const LedgerBase&) = delete;

    virtual ~LedgerBase() = default;

    void addRef() const;
    void release() const;

    LedgerBase& operator=(const LedgerBase&) = delete;

protected:
    detail::LedgerSyncContext* syncContext() const { // Called by LedgerManager and LedgerWriter
        return syncCtx_;
    }

    int& refCount() const { // Called by LedgerManager
        return refCount_;
    }

private:
    detail::LedgerSyncContext* syncCtx_; // Sync context
    mutable int refCount_; // Reference count

    friend class LedgerManager;
    friend class LedgerWriter;
};

class Ledger: public LedgerBase {
public:
    explicit Ledger(detail::LedgerSyncContext* ctx = nullptr);
    ~Ledger();

    int initReader(LedgerReader& reader);
    int initWriter(LedgerWriter& writer, LedgerWriteSource src);

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
    int updateInfo(const LedgerInfo& info); // ditto
    void notifySynced(); // ditto

    int notifyReaderClosed(bool staged); // Called by LedgerReader
    int notifyWriterClosed(const LedgerInfo& info, int tempSeqNum); // Called by LedgerWriter

private:
    int lastSeqNum_; // Counter incremented every time the ledger is opened for writing
    int stagedSeqNum_; // Sequence number assigned to the most recent staged ledger data
    int curReaderCount_; // Number of active readers of the current ledger data
    int stagedReaderCount_; // Number of active readers of the staged ledger data
    int stagedFileCount_; // Number of staged data files created

    int64_t lastUpdated_; // Time the ledger was last time updated
    int64_t lastSynced_; // Time the ledger was last synchronized
    size_t dataSize_; // Size of the ledger data
    unsigned updateCount_; // Counter incremented every time the ledger is updated
    bool syncPending_; // Whether the ledger has local changes that have not yet been synchronized

    ledger_sync_callback syncCallback_; // Callback to invoke when the ledger has been synchronized
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data
    void* appData_; // Application data

    const char* name_; // Ledger name (allocated by LedgerManager)
    LedgerScopeId scopeId_; // Scope ID
    ledger_scope scopeType_; // Scope type
    ledger_sync_direction syncDir_; // Sync direction

    bool inited_; // Whether the ledger is initialized

    mutable StaticRecursiveMutex mutex_; // Ledger lock

    int loadLedgerInfo(lfs_t* fs);
    void setLedgerInfo(const LedgerInfo& info);

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

    LedgerInfo& scopeType(ledger_scope type) {
        scopeType_ = type;
        return *this;
    }

    ledger_scope scopeType() const {
        return scopeType_.value_or(LEDGER_SCOPE_UNKNOWN);
    }

    bool isScopeTypeSet() const {
        return scopeType_.has_value();
    }

    LedgerInfo& scopeId(LedgerScopeId id) {
        scopeId_ = std::move(id);
        return *this;
    }

    const LedgerScopeId& scopeId() const {
        if (!scopeId_.has_value()) {
            return EMPTY_LEDGER_SCOPE_ID;
        }
        return scopeId_.value();
    }

    bool isScopeIdSet() const {
        return scopeId_.has_value();
    }

    LedgerInfo& syncDirection(ledger_sync_direction dir) {
        syncDir_ = dir;
        return *this;
    }

    ledger_sync_direction syncDirection() const {
        return syncDir_.value_or(LEDGER_SYNC_DIRECTION_UNKNOWN);
    }

    bool isSyncDirectionSet() const {
        return syncDir_.has_value();
    }

    LedgerInfo& dataSize(size_t size) {
        dataSize_ = size;
        return *this;
    }

    size_t dataSize() const {
        return dataSize_.value_or(0);
    }

    bool isDataSizeSet() const {
        return dataSize_.has_value();
    }

    LedgerInfo& lastUpdated(int64_t time) {
        lastUpdated_ = time;
        return *this;
    }

    int64_t lastUpdated() const {
        return lastUpdated_.value_or(0);
    }

    bool isLastUpdatedSet() const {
        return lastUpdated_.has_value();
    }

    LedgerInfo& lastSynced(int64_t time) {
        lastSynced_ = time;
        return *this;
    }

    int64_t lastSynced() const {
        return lastSynced_.value_or(0);
    }

    bool isLastSyncedSet() const {
        return lastSynced_.has_value();
    }

    LedgerInfo& updateCount(unsigned count) {
        updateCount_ = count;
        return *this;
    }

    unsigned updateCount() const {
        return updateCount_.value_or(0);
    }

    bool isUpdateCountSet() const {
        return updateCount_.has_value();
    }

    LedgerInfo& syncPending(bool pending) {
        syncPending_ = pending;
        return *this;
    }

    bool syncPending() const {
        return syncPending_.value_or(false);
    }

    bool isSyncPendingSet() const {
        return syncPending_.has_value();
    }

    LedgerInfo& update(const LedgerInfo& info);

private:
    // When adding a new field, make sure to update the following methods and functions:
    // LedgerInfo::update()
    // Ledger::info()
    // Ledger::setLedgerInfo()
    // Ledger::loadLedgerInfo()
    // writeLedgerInfo()
    std::optional<LedgerScopeId> scopeId_;
    std::optional<int64_t> lastUpdated_;
    std::optional<int64_t> lastSynced_;
    std::optional<size_t> dataSize_;
    std::optional<unsigned> updateCount_;
    std::optional<ledger_scope> scopeType_;
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

    Ledger* ledger() const {
        return ledger_.get();
    }

    const LedgerInfo& info() const {
        return info_;
    }

    bool isOpen() const {
        return open_;
    }

    LedgerReader& operator=(const LedgerReader&) = delete;

protected:
    int init(LedgerInfo info, int stagedSeqNum, RefCountPtr<Ledger> ledger); // Called by Ledger

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    LedgerInfo info_; // Ledger info
    lfs_file_t file_; // File handle
    size_t dataOffs_; // Current offset in the data section
    bool staged_; // Whether the data being read is staged
    bool open_; // Whether the reader is open

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

    void updateInfo(const LedgerInfo& info) {
        info_.update(info);
    }

    Ledger* ledger() const {
        return ledger_.get();
    }

    bool isOpen() const {
        return open_;
    }

    LedgerWriter& operator=(const LedgerWriter&) = delete;

protected:
    int init(LedgerWriteSource src, int tempSeqNum, RefCountPtr<Ledger> ledger); // Called by Ledger

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    LedgerInfo info_; // Ledger info updates
    lfs_file_t file_; // File handle
    LedgerWriteSource src_; // Who is writing to the ledger
    size_t dataSize_; // Size of the data written
    int tempSeqNum_; // Sequence number assigned to the temporary ledger data
    bool open_; // Whether the writer is open

    friend class Ledger;
};

inline bool operator==(const LedgerScopeId& id1, const LedgerScopeId& id2) {
    return id1.size == id2.size && std::memcmp(id1.data, id2.data, id1.size) == 0;
}

inline bool operator!=(const LedgerScopeId& id1, const LedgerScopeId& id2) {
    return !(id1 == id2);
}

} // namespace particle::system

#endif // HAL_PLATFORM_LEDGER
