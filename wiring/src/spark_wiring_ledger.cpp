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
#include <cstdint>

#include "spark_wiring_ledger.h"
#include "spark_wiring_logging.h"

#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace {

const size_t INITIAL_JSON_DOCUMENT_SIZE = 256;

// A custom reader for ArduinoJson
class LedgerStreamReader {
public:
    explicit LedgerStreamReader(ledger_stream* stream) :
            stream_(stream),
            error_(0) {
    }

    size_t readBytes(char* data, size_t size) {
        if (error_) {
            return 0;
        }
        int r = ledger_read(stream_, data, size, nullptr);
        if (r < 0) {
            if (r != SYSTEM_ERROR_END_OF_STREAM) {
                LOG(ERROR, "Failed to read from ledger stream: %d", r);
            }
            return 0;
        }
        return r;
    }

    int read() {
        uint8_t b = 0;
        size_t n = readBytes((char*)&b, 1);
        if (n == 0) {
            return -1;
        }
        return b;
    }

    int error() const {
        return error_;
    }

private:
    ledger_stream* stream_;
    int error_;
};

// A custom writer for ArduinoJson
class LedgerStreamWriter {
public:
    explicit LedgerStreamWriter(ledger_stream* stream) :
            stream_(stream),
            error_(0) {
    }

    size_t write(const uint8_t* data, size_t size) {
        if (error_) {
            return 0;
        }
        int r = ledger_write(stream_, (const char*)data, size, nullptr);
        if (r < 0) {
            LOG(ERROR, "Failed to write to ledger stream: %d", r);
            error_ = r;
            return 0;
        }
        return r;
    }

    size_t write(uint8_t b) {
        return write(&b, 1);
    }

    int error() const {
        return error_;
    }

private:
    ledger_stream* stream_;
    int error_;
};

class StringWriter {
public:
    explicit StringWriter(String& str) :
            str_(str) {
    }

    size_t write(const uint8_t* data, size_t size) {
        // TODO: Optimize memory allocations, handle errors
        str_ += String((const char*)data, size);
        return size;
    }

    size_t write(uint8_t b) {
        return write(&b, 1);
    }

private:
    String& str_;
};

} // namespace

namespace detail {

class LedgerImpl {
public:
    explicit LedgerImpl(ledger_instance* ledger) :
            lr_(ledger) {
    }

    int getPage(const char* name, LedgerPage& page);

    int getName(const char*& name) const {
        ledger_info info = {};
        CHECK(ledger_get_info(lr_, &info, nullptr));
        name = info.name;
        return 0;
    }

    int getScope(LedgerScope& scope) const {
        ledger_info info = {};
        CHECK(ledger_get_info(lr_, &info, nullptr));
        scope = static_cast<LedgerScope>(info.scope);
        return 0;
    }

private:
    ledger_instance* lr_;

    static void destroyPageAppData(void* appData);
};

class LedgerPageImpl {
public:
    explicit LedgerPageImpl(ledger_page* page) :
            p_(page) {
    }

    int init() {
        CHECK(loadData());
        return 0;
    }

    int getEntry(const char* name, LedgerEntry& entry) {
        if (!name) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        CString entryName(name);
        if (!entryName) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        entry = LedgerEntry(p_, std::move(entryName));
        return 0;
    }

    void removeEntry(const char* name) {
        doc_->remove(name);
    }

    void clearEntries() {
        doc_->clear();
    }

    void setDocument(std::shared_ptr<ArduinoJson::DynamicJsonDocument> doc) {
        doc_ = std::move(doc);
    }

    std::shared_ptr<ArduinoJson::DynamicJsonDocument> document() const {
        return doc_;
    }

    String toJson() const {
        String s;
        StringWriter writer(s);
        ArduinoJson::serializeJson(*doc_, writer);
        return s;
    }

    int save() {
        CHECK(saveData());
        return 0;
    }

private:
    std::shared_ptr<ArduinoJson::DynamicJsonDocument> doc_;
    ledger_page* p_;

    int loadData() {
        ledger_stream* stream = nullptr;
        CHECK(ledger_open_page(&stream, p_, LEDGER_STREAM_MODE_READ, nullptr));
        SCOPE_GUARD({
            int r = ledger_close_stream(stream, 0 /* flags */, nullptr);
            if (r < 0) {
                LOG(ERROR, "Error while closing stream: %d", r);
            }
        });
        std::shared_ptr<ArduinoJson::DynamicJsonDocument> doc;
        size_t docSize = INITIAL_JSON_DOCUMENT_SIZE;
        // DynamicJsonDocument can't grow dynamically so the parsing is done in a loop
        for (;;) {
            doc.reset(new(std::nothrow) ArduinoJson::DynamicJsonDocument(docSize));
            if (!doc || !doc->capacity()) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            LedgerStreamReader reader(stream);
            auto err = ArduinoJson::deserializeMsgPack(*doc, reader);
            if (err == ArduinoJson::DeserializationError::Ok) {
                break;
            }
            if (err == ArduinoJson::DeserializationError::EmptyInput) {
                doc->set(ArduinoJson::JsonObject());
                break;
            }
            if (err != ArduinoJson::DeserializationError::NoMemory) {
                LOG(ERROR, "Failed to deserialize ledger data: ArduinoJson::DeserializationError(%d)", (int)err.code());
                return SYSTEM_ERROR_LEDGER_DESERIALIZATION;
            }
            docSize = docSize * 3 / 2;
            CHECK(ledger_rewind_stream(stream, nullptr));
        }
        doc_ = doc;
        return 0;
    }

    int saveData() {
        ledger_stream* stream = nullptr;
        CHECK(ledger_open_page(&stream, p_, LEDGER_STREAM_MODE_WRITE, nullptr));
        SCOPE_GUARD({
            int r = ledger_close_stream(stream, 0 /* flags */, nullptr);
            if (r < 0) {
                LOG(ERROR, "Error while closing stream: %d", r);
            }
        });
        LedgerStreamWriter writer(stream);
        ArduinoJson::serializeMsgPack(*doc_, writer);
        if (writer.error() != 0) {
            LOG(ERROR, "Failed to serialize ledger data: %d", writer.error());
            return SYSTEM_ERROR_LEDGER_SERIALIZATION;
        }
        return 0;
    }
};

int LedgerImpl::getPage(const char* name, LedgerPage& page) {
    ledger_page* p = nullptr;
    CHECK(ledger_get_page(&p, lr_, name, nullptr));
    ledger_lock_page(p, nullptr);
    SCOPE_GUARD({
        ledger_unlock_page(p, nullptr);
    });
    if (!ledger_get_page_app_data(p, nullptr)) {
        // Attach application data to the system page instance
        NAMED_SCOPE_GUARD(g, {
            ledger_release_page(p, nullptr);
        });
        std::unique_ptr<LedgerPageImpl> impl(new(std::nothrow) LedgerPageImpl(p));
        if (!impl) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(impl->init());
        ledger_set_page_app_data(p, impl.release(), destroyPageAppData, nullptr); // Transfer ownership to the system
        g.dismiss();
    }
    page = LedgerPage::wrap(p);
    return 0;
}

void LedgerImpl::destroyPageAppData(void* appData) {
    delete static_cast<detail::LedgerPageImpl*>(appData);
}

} // namespace detail

LedgerPage Ledger::page(const char* name) {
    LedgerPage page;
    int r = 0;
    if (isValid() && (r = impl()->getPage(name, page)) < 0) {
        LOG(ERROR, "Failed to get ledger page: %d", r);
    }
    return page;
}

const char* Ledger::name() const {
    const char* name = "";
    int r = 0;
    if (isValid() && (r = impl()->getName(name)) < 0) {
        LOG(ERROR, "Failed to get ledger info: %d", r);
    }
    return name;
}

LedgerScope Ledger::scope() const {
    LedgerScope scope = LedgerScope::INVALID;
    int r = 0;
    if (isValid() && (r = impl()->getScope(scope)) < 0) {
        LOG(ERROR, "Failed to get ledger info: %d", r);
    }
    return scope;
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
        NAMED_SCOPE_GUARD(g, {
            ledger_release(lr, nullptr);
        });
        std::unique_ptr<detail::LedgerImpl> impl(new(std::nothrow) detail::LedgerImpl(lr));
        if (!impl) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        ledger_set_app_data(lr, impl.release(), destroyLedgerAppData, nullptr); // Transfer ownership to the system
        g.dismiss();
    }
    ledger = Ledger::wrap(lr);
    return 0;
}

void Ledger::destroyLedgerAppData(void* appData) {
    delete static_cast<detail::LedgerImpl*>(appData);
}

LedgerEntry LedgerPage::entry(const char* name) const {
    LedgerEntry entry;
    int r = 0;
    if (isValid() && (r = impl()->getEntry(name, entry)) < 0) {
        LOG(ERROR, "Failed to get page entry: %d", r);
    }
    return entry;
}

void LedgerPage::remove(const char* name) {
    if (!isValid()) {
        return;
    }
    impl()->removeEntry(name);
}

void LedgerPage::clear() {
    if (!isValid()) {
        return;
    }
    impl()->clearEntries();
}

ArduinoJson::DynamicJsonDocument LedgerPage::toJsonDocument() const {
    if (!isValid()) {
        return ArduinoJson::DynamicJsonDocument(0);
    }
    return *impl()->document();
}

String LedgerPage::toJson() const {
    if (!isValid()) {
        return String();
    }
    return impl()->toJson();
}

int LedgerPage::save() {
    if (!isValid()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(impl()->save());
    return 0;
}

LedgerEntry::LedgerEntry(ledger_page* page, CString entryName) :
        name_(std::move(entryName)),
        p_(page) {
    ledger_add_page_ref(p_, nullptr);
    doc_ = pageImpl()->document();
    val_ = (*doc_)[static_cast<const char*>(name_)];
}

void LedgerEntry::updatePage() {
    pageImpl()->setDocument(doc_);
}

} // namespace particle
