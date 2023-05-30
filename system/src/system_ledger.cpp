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

#include "system_ledger.h"

#include "system_ledger_internal.h"
#include "system_threading.h"
#include "scope_guard.h"
#include "check.h"

using namespace particle;
using namespace particle::system;

int ledger_get_instance(ledger_instance** ledger, const char* name, int apiVersion, void* reserved) {
    RefCountPtr<Ledger> lp;
    CHECK(LedgerManager::instance()->initLedger(name, apiVersion, lp));
    *ledger = reinterpret_cast<ledger_instance*>(lp.unwrap()); // Transfer ownership to the caller
    return 0;
}

void ledger_add_ref(ledger_instance* ledger, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->addRef();
}

void ledger_release(ledger_instance* ledger, void* reserved) {
    if (ledger) {
        auto lp = reinterpret_cast<Ledger*>(ledger);
        lp->release();
    }
}

void ledger_lock(ledger_instance* ledger, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->lock();
}

void ledger_unlock(ledger_instance* ledger, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->unlock();
}

void ledger_set_callbacks(ledger_instance* ledger, const ledger_callbacks* callbacks, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->setCallbacks(callbacks->page_sync, callbacks->page_sync_arg, callbacks->page_change, callbacks->page_change_arg);
}

void ledger_set_app_data(ledger_instance* ledger, void* appData, ledger_destroy_app_data_callback destroy,
        void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->setAppData(appData, destroy);
}

void* ledger_get_app_data(ledger_instance* ledger, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    return lp->appData();
}

int ledger_get_info(ledger_instance* ledger, ledger_info* info, void* reserved) {
    auto lp = reinterpret_cast<Ledger*>(ledger);
    lp->lock();
    SCOPE_GUARD({
        lp->unlock();
    });
    info->name = lp->name();
    info->scope = lp->scope();
    if (info->linked_page_names && info->linked_page_count > 0) {
        Vector<CString> names;
        CHECK(lp->getLinkedPageNames(names));
        auto count = std::min<size_t>(names.size(), info->linked_page_count);
        for (size_t i = 0; i < count; ++i) {
            info->linked_page_names[i] = names.at(i).unwrap(); // Transfer ownership to the caller
        }
        info->linked_page_count = count;
    }
    return 0;
}

int ledger_set_default_sync_options(ledger_instance* ledger, const ledger_sync_options* options, void* reserved) {
    return 0;
}

int ledger_get_page(ledger_page** page, ledger_instance* ledger, const char* name, void* reserved) {
    return 0;
}

void ledger_add_page_ref(ledger_page* ledger, void* reserved) {
}

void ledger_release_page(ledger_page* page, void* reserved) {
}

void ledger_lock_page(ledger_page* page, void* reserved) {
}

void ledger_unlock_page(ledger_page* page, void* reserved) {
}

ledger_instance* ledger_get_page_ledger(ledger_page* page, void* reserved) {
    return nullptr;
}

void ledger_set_page_app_data(ledger_page* page, void* app_data, ledger_destroy_app_data_callback destroy,
        void* reserved) {
}

void* ledger_get_page_app_data(ledger_page* page, void* reserved) {
    return nullptr;
}

int ledger_get_page_info(ledger_page* page, ledger_page_info* info, void* reserved) {
    return 0;
}

int ledger_sync_page(ledger_page* page, const ledger_sync_options* options, void* reserved) {
    return 0;
}

int ledger_unlink_page(ledger_page* page, void* reserved) {
    return 0;
}

int ledger_remove_page(ledger_page* page, void* reserved) {
    return 0;
}

int ledger_open_page(ledger_stream** stream, ledger_page* page, int mode, void* reserved) {
    return 0;
}

void ledger_close_stream(ledger_stream* stream, void* reserved) {
}

int ledger_flush_stream(ledger_stream* stream, void* reserved) {
    return 0;
}

int ledger_read(ledger_stream* stream, char* data, size_t size, void* reserved) {
    return 0;
}

int ledger_write(ledger_stream* stream, const char* data, size_t size, void* reserved) {
    return 0;
}
