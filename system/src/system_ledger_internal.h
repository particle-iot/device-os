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

#include <memory>

#include "system_ledger.h"
#include "filesystem.h"
#include "static_recursive_mutex.h"
#include "ref_count.h"
#include "c_string.h"
#include "system_error.h"

#include "spark_wiring_vector.h"

namespace particle::system {

class Ledger;
class LedgerPage;
class LedgerStream;
class LedgerPageInputStream;
class LedgerPageOutputStream;

enum class LedgerChangeSource {
    SYSTEM = 1,
    APP = 2
};

class LedgerManager {
public:
    LedgerManager() :
            inited_(false) {
    }

    int init();

    int getLedger(const char* name, ledger_scope scope,int apiVersion, RefCountPtr<Ledger>& ledger);
    void addLedgerRef(Ledger* ledger); // Called by Ledger::addRef()
    void releaseLedger(Ledger* ledger); // Called by Ledger::release()

    static LedgerManager* instance();

private:
    Vector<Ledger*> ledgerInstances_; // Instantiated ledgers
    mutable StaticRecursiveMutex mutex_; // Manager lock
    bool inited_; // Whether the manager was initialized successfully
};

class Ledger {
public:
    Ledger() :
            pageSyncCallback_(nullptr),
            pageChangeCallback_(nullptr),
            appData_(nullptr),
            destroyAppData_(nullptr),
            scope_(LEDGER_SCOPE_INVALID),
            apiVersion_(0),
            refCount_(1) {
    }

    ~Ledger();

    int init(const char* name, ledger_scope scope, int apiVersion);

    void setCallbacks(ledger_page_sync_callback pageSync, ledger_page_change_callback pageChange);

    void setAppData(void* data, ledger_destroy_app_data_callback destroy);
    void* appData() const;

    int getPage(const char* name, RefCountPtr<LedgerPage>& page);
    void addPageRef(LedgerPage* page); // Called by LedgerPage::addRef()
    void releasePage(LedgerPage* page); // Called by LedgerPage::release()

    int getLinkedPageNames(Vector<CString>& names) const;

    bool isUserWritable() const {
        return scope_ == LEDGER_SCOPE_DEVICE;
    }

    ledger_scope scope() const {
        return scope_;
    }

    const char* name() const {
        return name_;
    }

    void lock() const {
        mutex_.lock();
    }

    void unlock() const {
        mutex_.unlock();
    }

    void addRef() {
        // The ledger's reference counter is managed by the LedgerManager. We can't safely use a
        // regular atomic counter, such as RefCount, because the LedgerManager maintains a list of
        // reusable ledger instances which needs to be updated when a ledger is destroyed. Using
        // shared pointers would solve the problem but those can't be used in dynalib interfaces
        LedgerManager::instance()->addLedgerRef(this);
    }

    void release() {
        LedgerManager::instance()->releaseLedger(this);
    }

private:
    Vector<CString> linkedPageNames_; // Names of linked pages
    Vector<LedgerPage*> pageInstances_; // Instantiated pages
    CString name_; // Ledger name
    mutable StaticRecursiveMutex mutex_; // Ledger lock

    // Ledger callbacks
    ledger_page_sync_callback pageSyncCallback_;
    ledger_page_change_callback pageChangeCallback_;

    void* appData_; // Application data
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data

    ledger_scope scope_; // Ledger scope
    int apiVersion_; // API version
    int refCount_; // Reference count

    int loadLedgerInfo();
    int saveLedgerInfo();

    friend class LedgerManager; // For accessing refCount_
};

class LedgerPage {
public:
    LedgerPage() :
            appData_(nullptr),
            destroyAppData_(nullptr),
            readerCount_(0),
            refCount_(1) {
    }

    ~LedgerPage();

    int init(const char* name, Ledger* ledger);

    void setAppData(void* data, ledger_destroy_app_data_callback destroy);
    void* appData() const;

    int openStream(int mode, bool forceMode, std::unique_ptr<LedgerStream>& stream);
    int inputStreamClosed(LedgerPageInputStream* stream); // Called by LedgerPageInputStream
    int outputStreamClosed(LedgerPageOutputStream* stream, bool discard); // Called by LedgerPageOutputStream

    const char* name() const {
        return name_;
    }

    Ledger* ledger() const {
        return ledger_.get();
    }

    void lock() const {
        ledger_->lock();
    }

    void unlock() const {
        ledger_->unlock();
    }

    void addRef() {
        // The page's reference counter is managed by the parent ledger instance
        ledger_->addPageRef(this);
    }

    void release() {
        ledger_->releasePage(this);
    }

private:
    CString name_; // Page name
    RefCountPtr<Ledger> ledger_; // Parent ledger
    CString updatedPageFile_; // Temporary file with pending changes to the page

    void* appData_; // Application data
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data

    int readerCount_; // Number of input streams open for this page
    int refCount_; // Reference count

    friend class Ledger; // For accessing refCount_

    int loadPageInfo();
};

// TODO: Use the interface classes from stream.h
class LedgerStream {
public:
    virtual ~LedgerStream() = default;

    virtual int close(int flags) = 0;

    virtual int read(char* data, size_t size) = 0;
    virtual int write(const char* data, size_t size) = 0;
};

class LedgerPageInputStream: public LedgerStream {
public:
    LedgerPageInputStream() :
            file_(),
            streamOpen_(false),
            fileOpen_(false) {
    }

    ~LedgerPageInputStream();

    int init(const char* pageFile, LedgerPage* page);

    int close(int flags) override;
    int read(char* data, size_t size) override;

    int write(const char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

private:
    RefCountPtr<LedgerPage> page_; // Page being read
    lfs_file_t file_; // File handle
    bool streamOpen_; // Whether the stream is open
    bool fileOpen_; // Whether the page file is open
};

class LedgerPageOutputStream: public LedgerStream {
public:
    LedgerPageOutputStream() :
            file_(),
            pageInfoSize_(0),
            open_(false) {
    }

    ~LedgerPageOutputStream();

    int init(const char* tempPageFile, LedgerPage* page);

    int close(int flags) override;

    int read(char* data, size_t size) override {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int write(const char* data, size_t size) override;

    const char* fileName() const {
        return fileName_;
    }

private:
    CString fileName_; // Temporary file name
    RefCountPtr<LedgerPage> page_; // Page being written
    lfs_file_t file_; // File handle
    size_t pageInfoSize_; // Size of the page info section of the page file
    bool open_; // Whether the stream is open
};

} // namespace particle::system
