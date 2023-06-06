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

//#include <ArduinoJson.hpp>

#include <memory>

#include "spark_wiring_ledger.h"
#include "spark_wiring_logging.h"

#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace detail {

class LedgerImpl {
public:
    explicit LedgerImpl(ledger_instance* ledger) :
            lr_(ledger),
            name_(nullptr),
            scope_(LedgerScope::INVALID) {
    }

    int init() {
        ledger_info info = {};
        CHECK(ledger_get_info(lr_, &info, nullptr));
        name_ = info.name;
        scope_ = static_cast<LedgerScope>(info.scope);
        return 0;
    }

    int getPage(const char* name, ledger_page*& page);

    const char* name() const {
        return name_;
    }

    LedgerScope scope() const {
        return scope_;
    }

private:
    ledger_instance* lr_;
    const char* name_;
    LedgerScope scope_;

    static void destroyPageAppData(void* appData);
};

class LedgerPageImpl {
public:
    explicit LedgerPageImpl(ledger_page* page) :
            p_(page) {
    }

private:
    ledger_page* p_;
};

int LedgerImpl::getPage(const char* name, ledger_page*& page) {
    ledger_page* p = nullptr;
    CHECK(ledger_get_page(&p, lr_, name, nullptr));
    ledger_lock_page(p, nullptr);
    SCOPE_GUARD({
        ledger_unlock_page(p, nullptr);
    });
    if (!ledger_get_page_app_data(p, nullptr)) {
        // Attach application data to the system page instance
        std::unique_ptr<detail::LedgerPageImpl> impl(new(std::nothrow) detail::LedgerPageImpl(p));
        if (!impl) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        ledger_set_page_app_data(p, impl.release(), destroyPageAppData, nullptr); // Transfer ownership to the system
    }
    page = p;
    return 0;
}

void LedgerImpl::destroyPageAppData(void* appData) {
    delete static_cast<detail::LedgerPageImpl*>(appData);
}

} // namespace detail

LedgerPage Ledger::page(const char* name) {
    if (!isValid()) {
        return LedgerPage();
    }
    ledger_page* p = nullptr;
    int r = impl()->getPage(name, p);
    if (r < 0) {
        LOG(ERROR, "Failed to get ledger page: %d", r);
        return LedgerPage();
    }
    return LedgerPage(p);
}

const char* Ledger::name() const {
    if (!isValid()) {
        return "";
    }
    return impl()->name();
}

LedgerScope Ledger::scope() const {
    if (!isValid()) {
        return LedgerScope::INVALID;
    }
    return impl()->scope();
}

int Ledger::getInstance(const char* name, LedgerScope scope, Ledger& ledger) {
    ledger_instance* lr = nullptr;
    CHECK(ledger_get_instance(&lr, name, static_cast<ledger_scope>(scope), LEDGER_API_VERSION, nullptr));
    ledger_lock(lr, nullptr);
    SCOPE_GUARD({
        ledger_unlock(lr, nullptr);
    });
    if (!ledger_get_app_data(lr, nullptr)) {
        // Attach application data to the system ledger instance
        std::unique_ptr<detail::LedgerImpl> impl(new(std::nothrow) detail::LedgerImpl(lr));
        if (!impl) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(impl->init());
        ledger_set_app_data(lr, impl.release(), destroyLedgerAppData, nullptr); // Transfer ownership to the system
    }
    ledger = Ledger(lr);
    return 0;
}

void Ledger::destroyLedgerAppData(void* appData) {
    delete static_cast<detail::LedgerImpl*>(appData);
}

} // namespace particle
