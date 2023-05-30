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

#include <mutex>

#include "system_ledger.h"
#include "static_recursive_mutex.h"
#include "ref_count.h"
#include "c_string.h"

#include "spark_wiring_vector.h"

namespace particle::system {

class LedgerPage;

class Ledger: public RefCount<Ledger> {
public:
    Ledger() :
            pageSyncCallback_(nullptr),
            pageSyncArg_(nullptr),
            pageChangeCallback_(nullptr),
            pageChangeArg_(nullptr),
            appData_(nullptr),
            destroyAppData_(nullptr),
            scope_(LEDGER_SCOPE_UNKNOWN),
            apiVersion_(0) {
    }

    ~Ledger() {
        destroy();
    }

    int init(const char* name, ledger_scope scope, int apiVersion);
    void destroy();

    int getLinkedPageNames(Vector<CString>& names) const;

    void setCallbacks(ledger_page_sync_callback pageSyncCallback, void* pageSyncArg,
            ledger_page_change_callback pageChangeCallback, void* pageChangeArg);

    void setAppData(void* data, ledger_destroy_app_data_callback destroy);

    void* appData() const {
        std::lock_guard lock(mutex_);
        return appData_;
    }

    ledger_scope scope() const {
        std::lock_guard lock(mutex_);
        return scope_;
    }

    const char* name() const {
        // The name is immutable so no locking is needed
        return name_;
    }

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

private:
    Vector<CString> linkedPageNames_; // Names of linked pages
    Vector<LedgerPage*> pages_; // Instantiated pages
    CString name_; // Ledger name
    mutable StaticRecursiveMutex mutex_; // Ledger lock
    // Ledger callbacks
    ledger_page_sync_callback pageSyncCallback_;
    void* pageSyncArg_;
    ledger_page_change_callback pageChangeCallback_;
    void* pageChangeArg_;
    void* appData_; // Application data
    ledger_destroy_app_data_callback destroyAppData_; // Destructor for the application data
    ledger_scope scope_; // Ledger scope
    int apiVersion_; // API version
};

class LedgerManager {
public:
    int initLedger(const char* name, int apiVersion, RefCountPtr<Ledger>& ledger);

    void disposeLedger(const char* name);

    int init();

    static LedgerManager* instance();

private:
    Vector<Ledger*> sharedLedgers_; // Instantiated product and organization ledgers
    Ledger* devLedger_; // Device ledger
    mutable StaticRecursiveMutex mutex_; // Manager lock
};

} // namespace particle::system
