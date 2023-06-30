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

#include <optional>
#include <utility>
#include <memory>
#include <mutex>

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

enum class LedgerChangeSource {
    USER,
    SYSTEM
};

// The reference counter of a ledger instance is managed by the LedgerManager. We can't safely use a
// regular atomic counter, such as RefCount, because the LedgerManager maintains a list of all created
// ledger instances that needs to be updated when any of the instances is destroyed. Shared pointers
// would work but those can't be used in dynalib interfaces
class LedgerBase {
public:
    LedgerBase() :
            refCount_(1) {
    }

    LedgerBase(const LedgerBase&) = delete;

    virtual ~LedgerBase() = default;

    void addRef() const;
    void release() const;

    LedgerBase& operator=(const LedgerBase&) = delete;

private:
    mutable int refCount_;

    friend class LedgerManager;
};

class LedgerManager {
public:
    LedgerManager() = default;

    LedgerManager(const LedgerBase&) = delete;

    int getLedger(const char* name, RefCountPtr<Ledger>& ledger);

    int removeLedgerData(const char* name);
    int removeAllData();

    LedgerManager& operator=(const LedgerManager&) = delete;

    static LedgerManager* instance();

private:
    Vector<Ledger*> ledgers_; // Instantiated ledgers
    mutable StaticRecursiveMutex mutex_; // Manager lock

    std::pair<Vector<Ledger*>::ConstIterator, bool> findLedger(const char* name) const;

    void addLedgerRef(const LedgerBase* ledger); // Called by LedgerBase::addRef()
    void releaseLedger(const LedgerBase* ledger); // Called by LedgerBase::release()

    friend class LedgerBase;
};

class Ledger: public LedgerBase {
public:
    Ledger();
    ~Ledger();

    int init(const char* name);

    int beginRead(LedgerReader& reader);
    int beginWrite(LedgerChangeSource src, LedgerWriter& writer);

    LedgerInfo info() const;

    const char* name() const {
        return name_; // Immutable
    }

    void setSyncCallback(ledger_sync_callback callback) {
        std::lock_guard lock(*this);
        syncCallback_ = callback;
    }

    void setAppData(void* data, ledger_destroy_app_data_callback destroy);

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

private:
    int lastSeqNum_; // Counter incremented every time the ledger is opened for writing
    int stagedSeqNum_; // Sequence number assigned to the most recent staged ledger data
    int curReaderCount_; // Number of active readers of the current ledger data
    int stagedReaderCount_; // Number of active readers of the staged ledger data
    int stagedFileCount_; // Number of staged data files created

    int64_t lastUpdated_; // Last time the ledger was updated
    int64_t lastSynced_; // Last time the ledger was synchronized
    bool syncPending_; // Whether the ledger has local changes that have not yet been synchronized

    ledger_sync_callback syncCallback_; // Callback to invoke when the ledger has been synchronized
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data
    void* appData_; // Application data

    CString name_; // Ledger name
    ledger_scope scope_; // Ledger scope
    ledger_sync_direction syncDir_; // Sync direction

    mutable StaticRecursiveMutex mutex_; // Ledger lock

    int endRead(LedgerReader* reader); // Called by LedgerReader
    int endWrite(LedgerWriter* writer, const LedgerInfo& info); // Called by LedgerWriter
    int discardWrite(LedgerWriter* writer); // ditto

    int loadLedgerInfo(lfs_t* fs);
    int writeLedgerInfo(lfs_t* fs, lfs_file_t* file);
    void updateLedgerInfo(const LedgerInfo& info);

    int initCurrentData(lfs_t* fs);
    int flushStagedData(lfs_t* fs);
    int removeTempData(lfs_t* fs);

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

private:
    std::optional<ledger_scope> scope_;
    std::optional<ledger_sync_direction> syncDir_;
    std::optional<int64_t> lastUpdated_;
    std::optional<int64_t> lastSynced_;
    std::optional<bool> syncPending_;
};

class LedgerStream {
public:
    virtual ~LedgerStream() = default;

    virtual int read(char* data, size_t size) = 0;
    virtual int write(const char* data, size_t size) = 0;
    virtual int close(bool flush = true) = 0;
};

class LedgerReader: public LedgerStream {
public:
    LedgerReader() :
            file_(),
            dataSize_(0),
            dataOffs_(0),
            seqNum_(0),
            open_(false) {
    }

    LedgerReader(const LedgerReader&) = delete;

    LedgerReader(LedgerReader&& reader) :
            LedgerReader() {
        swap(*this, reader);
    }

    ~LedgerReader() {
        end();
    }

    int read(char* data, size_t size) override;

    int write(const char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int close(bool flush) override;

    int end();

    bool isOpen() const {
        return open_;
    }

    lfs_file_t* file() {
        return &file_;
    }

    int seqNumber() const {
        return seqNum_;
    }

    LedgerReader& operator=(LedgerReader&& reader) {
        swap(*this, reader);
        return *this;
    }

    LedgerReader& operator=(const LedgerReader&) = default;

    friend void swap(LedgerReader& r1, LedgerReader& r2) {
        using std::swap; // For ADL
        swap(r1.ledger_, r2.ledger_);
        swap(r1.file_, r2.file_);
        swap(r1.dataSize_, r2.dataSize_);
        swap(r1.dataOffs_, r2.dataOffs_);
        swap(r1.seqNum_, r2.seqNum_);
        swap(r1.open_, r2.open_);
    }

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    lfs_file_t file_; // File handle
    size_t dataSize_; // Size of the data section of the ledger file
    size_t dataOffs_; // Current offset in the data section
    int seqNum_; // Sequence number assigned to the ledger data being read
    bool open_; // Whether the reader is open

    LedgerReader(lfs_file_t* file, size_t dataSize, int seqNum, Ledger* ledger) :
            ledger_(ledger),
            file_(*file),
            dataSize_(dataSize),
            dataOffs_(0),
            seqNum_(seqNum),
            open_(true) {
    }

    friend class Ledger;
};

class LedgerWriter: public LedgerStream {
public:
    LedgerWriter() :
            file_(),
            changeSrc_(),
            dataSize_(0),
            seqNum_(0),
            open_(false) {
    }

    LedgerWriter(const LedgerWriter&) = delete;

    LedgerWriter(LedgerWriter&& writer) :
            LedgerWriter() {
        swap(*this, writer);
    }

    ~LedgerWriter() {
        discard();
    }

    int read(char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int write(const char* data, size_t size) override;
    int close(bool flush) override;

    int end(const LedgerInfo& info = LedgerInfo());

    void discard();

    bool isOpen() const {
        return open_;
    }

    lfs_file_t* file() {
        return &file_;
    }

    size_t dataSize() const {
        return dataSize_;
    }

    int seqNumber() const {
        return seqNum_;
    }

    LedgerChangeSource changeSource() const {
        return changeSrc_;
    }

    LedgerWriter& operator=(LedgerWriter&& writer) {
        swap(*this, writer);
        return *this;
    }

    LedgerWriter& operator=(const LedgerWriter&) = delete;

    friend void swap(LedgerWriter& w1, LedgerWriter& w2) {
        using std::swap; // For ADL
        swap(w1.ledger_, w2.ledger_);
        swap(w1.file_, w2.file_);
        swap(w1.changeSrc_, w2.changeSrc_);
        swap(w1.dataSize_, w2.dataSize_);
        swap(w1.seqNum_, w2.seqNum_);
        swap(w1.open_, w2.open_);
    }

private:
    RefCountPtr<Ledger> ledger_; // Ledger instance
    lfs_file_t file_; // File handle
    LedgerChangeSource changeSrc_; // Who is writing to the ledger
    size_t dataSize_; // Size of the written data
    int seqNum_; // Sequence number assigned to the ledger data being written
    bool open_; // Whether the writer is open

    LedgerWriter(lfs_file_t* file, LedgerChangeSource src, int seqNum, Ledger* ledger) :
            ledger_(ledger),
            file_(*file),
            changeSrc_(src),
            dataSize_(0),
            seqNum_(seqNum),
            open_(true) {
    }

    friend class Ledger;
};

inline void LedgerBase::addRef() const {
    LedgerManager::instance()->addLedgerRef(this);
}

inline void LedgerBase::release() const {
    LedgerManager::instance()->releaseLedger(this);
}

} // namespace particle::system
