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

#include "system_ledger_internal.h"

#include "system_error.h"
#include "check.h"

namespace particle::system {

namespace {

int copyStringVector(const Vector<CString>& src, Vector<CString>& dest) {
    dest.clear();
    if (!dest.reserve(src.size())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    for (int i = 0; i < src.size(); ++i) {
        CString s = src.at(i);
        if (!!s != !!src.at(i)) {
            return SYSTEM_ERROR_NO_MEMORY; // strdup() failed
        }
        dest.append(std::move(s));
    }
    return 0;
}

} // namespace

int Ledger::init(const char* name, ledger_scope scope, int apiVersion) {
    if (name) {
        name_ = name;
        if (!name_) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    if ((name && scope == LEDGER_SCOPE_DEVICE) || (!name && scope != LEDGER_SCOPE_DEVICE)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    scope_ = scope;
    apiVersion_ = apiVersion;
    return 0;
}

void Ledger::destroy() {
    // Remove this ledger from the list of instantiated ledgers
    LedgerManager::instance()->disposeLedger(name_);
    // Destroy application data
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
}

int Ledger::getLinkedPageNames(Vector<CString>& names) const {
    std::lock_guard lock(mutex_);
    CHECK(copyStringVector(linkedPageNames_, names));
    return 0;
}

void Ledger::setCallbacks(ledger_page_sync_callback pageSyncCallback, void* pageSyncArg,
        ledger_page_change_callback pageChangeCallback, void* pageChangeArg) {
    std::lock_guard lock(mutex_);
    pageSyncCallback_ = pageSyncCallback;
    pageSyncArg_ = pageSyncArg;
    pageChangeCallback_ = pageChangeCallback;
    pageChangeArg_ = pageChangeArg;
}

void Ledger::setAppData(void* data, ledger_destroy_app_data_callback destroy) {
    std::lock_guard lock(mutex_);
    appData_ = data;
    destroyAppData_ = destroy;
}

int LedgerManager::initLedger(const char* name, int apiVersion, RefCountPtr<Ledger>& ledger) {
    return 0;
}

void LedgerManager::disposeLedger(const char* name) {
}

int LedgerManager::init() {
    return 0;
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

} // namespace particle::system
