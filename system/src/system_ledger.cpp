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

#include <memory>

#include "system_ledger.h"

#include "system_ledger_internal.h"
#include "system_threading.h"
#include "scope_guard.h"
#include "check.h"

using namespace particle;
using namespace particle::system;

namespace {

bool isValidLedgerScopeValue(int scope) {
    switch (scope) {
    case LEDGER_SCOPE_INVALID:
    case LEDGER_SCOPE_DEVICE:
    case LEDGER_SCOPE_PRODUCT:
    case LEDGER_SCOPE_OWNER:
        return true;
    default:
        return false;
    }
}

} // namespace

int ledger_get_instance(ledger_instance** ledger, const char* name, int scope, int apiVersion, void* reserved) {
    if (!isValidLedgerScopeValue(scope)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<Ledger> lr;
    CHECK(LedgerManager::instance()->getLedger(name, static_cast<ledger_scope>(scope), apiVersion, lr));
    *ledger = reinterpret_cast<ledger_instance*>(lr.unwrap()); // Transfer ownership to the caller
    return 0;
}

void ledger_add_ref(ledger_instance* ledger, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->addRef();
}

void ledger_release(ledger_instance* ledger, void* reserved) {
    if (ledger) {
        auto lr = reinterpret_cast<Ledger*>(ledger);
        lr->release();
    }
}

void ledger_lock(ledger_instance* ledger, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->lock();
}

void ledger_unlock(ledger_instance* ledger, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->unlock();
}

void ledger_set_callbacks(ledger_instance* ledger, const ledger_callbacks* callbacks, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->setCallbacks(callbacks->page_sync, callbacks->remote_page_change);
}

void ledger_set_app_data(ledger_instance* ledger, void* appData, ledger_destroy_app_data_callback destroy,
        void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->setAppData(appData, destroy);
}

void* ledger_get_app_data(ledger_instance* ledger, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    return lr->appData();
}

int ledger_get_info(ledger_instance* ledger, ledger_info* info, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    lr->lock();
    SCOPE_GUARD({
        lr->unlock();
    });
    info->name = lr->name();
    info->scope = lr->scope();
    if (info->linked_page_names && info->linked_page_count > 0) {
        Vector<CString> names;
        CHECK(lr->getLinkedPageNames(names));
        auto count = std::min<size_t>(names.size(), info->linked_page_count);
        for (size_t i = 0; i < count; ++i) {
            info->linked_page_names[i] = names.at(i).unwrap(); // Transfer ownership to the caller
        }
        info->linked_page_count = count;
    }
    return 0;
}

int ledger_set_default_sync_options(ledger_instance* ledger, const ledger_sync_options* options, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int ledger_get_page(ledger_page** page, ledger_instance* ledger, const char* name, void* reserved) {
    auto lr = reinterpret_cast<Ledger*>(ledger);
    RefCountPtr<LedgerPage> p;
    CHECK(lr->getPage(name, p));
    *page = reinterpret_cast<ledger_page*>(p.unwrap()); // Transfer ownership to the caller
    return 0;
}

void ledger_add_page_ref(ledger_page* page, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    p->addRef();
}

void ledger_release_page(ledger_page* page, void* reserved) {
    if (page) {
        auto p = reinterpret_cast<LedgerPage*>(page);
        p->release();
    }
}

void ledger_lock_page(ledger_page* page, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    p->lock();
}

void ledger_unlock_page(ledger_page* page, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    p->unlock();
}

ledger_instance* ledger_get_page_ledger(ledger_page* page, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    return reinterpret_cast<ledger_instance*>(p->ledger());
}

void ledger_set_page_callbacks(ledger_page* page, const ledger_page_callbacks* callbacks, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    p->setLocalChangeCallback(callbacks->local_change);
}

void ledger_set_page_app_data(ledger_page* page, void* app_data, ledger_destroy_app_data_callback destroy,
        void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    p->setAppData(app_data, destroy);
}

void* ledger_get_page_app_data(ledger_page* page, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    return p->appData();
}

int ledger_get_page_info(ledger_page* page, ledger_page_info* info, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int ledger_sync_page(ledger_page* page, const ledger_sync_options* options, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int ledger_unlink_page(ledger_page* page, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int ledger_remove_page(ledger_page* page, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int ledger_open_page(ledger_stream** stream, ledger_page* page, int mode, void* reserved) {
    auto p = reinterpret_cast<LedgerPage*>(page);
    std::unique_ptr<LedgerStream> s;
    if (mode & LEDGER_STREAM_MODE_READ) {
        if (mode & LEDGER_STREAM_MODE_WRITE) {
            return SYSTEM_ERROR_INVALID_ARGUMENT; // Opening for both reading and writing is not supported
        }
        CHECK(p->openInputStream(s));
    } else if (mode & LEDGER_STREAM_MODE_WRITE) {
        if (mode & LEDGER_STREAM_MODE_READ) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        CHECK(p->openOutputStream(LedgerChangeSource::USER, s));
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    *stream = reinterpret_cast<ledger_stream*>(s.release()); // Transfer ownership to the caller
    return 0;
}

int ledger_close_stream(ledger_stream* stream, int flags, void* reserved) {
    auto s = reinterpret_cast<LedgerStream*>(stream);
    bool flush = !(flags & LEDGER_STREAM_CLOSE_DISCARD);
    CHECK(s->close(flush));
    return 0;
}

int ledger_read(ledger_stream* stream, char* data, size_t size, void* reserved) {
    auto s = reinterpret_cast<LedgerStream*>(stream);
    size_t n = CHECK(s->read(data, size));
    return n;
}

int ledger_write(ledger_stream* stream, const char* data, size_t size, void* reserved) {
    auto s = reinterpret_cast<LedgerStream*>(stream);
    size_t n = CHECK(s->write(data, size));
    return n;
}
