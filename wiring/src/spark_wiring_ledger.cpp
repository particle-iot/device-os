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

#include "spark_wiring_platform.h"

#if Wiring_Ledger

#include <memory>
#include <cstdlib>

#include "spark_wiring_ledger.h"

#include "spark_wiring_stream.h"
#include "spark_wiring_error.h"

#include "system_task.h"

#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace {

class LedgerStream: public Stream {
public:
    explicit LedgerStream(ledger_instance* ledger) :
            ledger_(ledger),
            stream_(nullptr),
            bytesRead_(0),
            bytesWritten_(0) {
    }

    ~LedgerStream() {
        close(LEDGER_STREAM_CLOSE_DISCARD);
    }

    int read() override {
        uint8_t b;
        size_t n = readBytes((char*)&b, 1);
        if (n != 1) {
            return -1;
        }
        return b;
    }

    size_t readBytes(char* data, size_t size) override {
        if (!stream_ || error() < 0) {
            return 0;
        }
        int r = ledger_read(stream_, data, size, nullptr);
        if (r < 0) {
            // Suppress the error message if the ledger data is empty
            if (r != Error::END_OF_STREAM || bytesRead_ > 0) {
                LOG(ERROR, "ledger_read() failed: %d", r);
            }
            setError(r);
            return 0;
        }
        bytesRead_ += r;
        return r;
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t* data, size_t size) override {
        if (!stream_ || error() < 0) {
            return 0;
        }
        int r = ledger_write(stream_, (const char*)data, size, nullptr);
        if (r < 0) {
            LOG(ERROR, "ledger_write() failed: %d", r);
            setError(r);
            return 0;
        }
        bytesWritten_ += r;
        return r;
    }

    int available() override {
        return -1; // Not supported
    }

    int peek() override {
        return -1; // Not supported
    }

    void flush() override {
    }

    int open(int mode) {
        close(LEDGER_STREAM_CLOSE_DISCARD);
        int r = ledger_open(&stream_, ledger_, mode, nullptr);
        if (r < 0) {
            LOG(ERROR, "ledger_open() failed: %d", r);
            return r;
        }
        return 0;
    }

    int close(int flags = 0) {
        if (!stream_) {
            return 0;
        }
        int r = ledger_close(stream_, flags, nullptr);
        if (r < 0) {
            LOG(ERROR, "ledger_close() failed: %d", r);
        }
        clearWriteError();
        stream_ = nullptr;
        bytesRead_ = 0;
        bytesWritten_ = 0;
        return r;
    }

    size_t bytesRead() const {
        return bytesRead_;
    }

    size_t bytesWritten() const {
        return bytesWritten_;
    }

    int error() const {
        return getWriteError(); // TODO: Rename to error() or add an alias
    }

private:
    ledger_instance* ledger_;
    ledger_stream* stream_;
    size_t bytesRead_;
    size_t bytesWritten_;

    void setError(int error) {
        setWriteError(error); // TODO: Rename to setError() or add an alias
    }
};

struct LedgerAppData {
    Ledger::OnSyncFunction onSync;
};

void destroyLedgerAppData(void* appData) {
    delete static_cast<LedgerAppData*>(appData);
}

LedgerAppData* getLedgerAppData(ledger_instance* ledger) {
    auto appData = static_cast<LedgerAppData*>(ledger_get_app_data(ledger, nullptr));
    if (!appData) {
        ledger_lock(ledger, nullptr);
        SCOPE_GUARD({
            ledger_unlock(ledger, nullptr);
        });
        appData = static_cast<LedgerAppData*>(ledger_get_app_data(ledger, nullptr));
        if (!appData) {
            appData = new(std::nothrow) LedgerAppData();
            if (appData) {
                ledger_set_app_data(ledger, appData, destroyLedgerAppData, nullptr);
            }
        }
    }
    return appData;
}

// Callback wrapper executed in the application thread
void syncCallbackApp(void* data) {
    auto ledger = static_cast<ledger_instance*>(data);
    SCOPE_GUARD({
        ledger_release(ledger, nullptr);
    });
    auto appData = getLedgerAppData(ledger);
    if (appData && appData->onSync) {
        appData->onSync(Ledger(ledger));
    }
}

// Callback wrapper executed in the system thread
void syncCallbackSystem(ledger_instance* ledger, void* appData) {
    // Dispatch the callback to the application thread
    ledger_add_ref(ledger, nullptr);
    int r = application_thread_invoke(syncCallbackApp, ledger, nullptr);
    if (r != 0) { // FIXME: application_thread_invoke() doesn't really handle errors as of now
        ledger_release(ledger, nullptr);
    }
}

// TODO: Generalize this code when there are more callbacks supported
int setSyncCallback(ledger_instance* ledger, Ledger::OnSyncFunction callback) {
    ledger_lock(ledger, nullptr);
    SCOPE_GUARD({
        ledger_unlock(ledger, nullptr);
    });
    auto appData = getLedgerAppData(ledger);
    if (!appData) {
        return Error::NO_MEMORY;
    }
    bool hadCallback = !!appData->onSync;
    appData->onSync = std::move(callback);
    if (appData->onSync) {
        if (!hadCallback) {
            ledger_callbacks callbacks = {};
            callbacks.version = LEDGER_API_VERSION;
            callbacks.sync = syncCallbackSystem;
            ledger_set_callbacks(ledger, &callbacks, nullptr);
            // Keep the ledger instance around until the callback is cleared
            ledger_add_ref(ledger, nullptr);
        }
    } else if (hadCallback) {
        ledger_set_callbacks(ledger, nullptr, nullptr); // Clear the callback
        ledger_release(ledger, nullptr);
    }
    return 0;
}

int getLedgerInfo(ledger_instance* ledger, ledger_info& info) {
    info.version = LEDGER_API_VERSION;
    int r = ledger_get_info(ledger, &info, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_get_info() failed: %d", r);
        return r;
    }
    return 0;
}

int setLedgerData(ledger_instance* ledger, const LedgerData& data) {
    LedgerStream stream(ledger);
    CHECK(stream.open(LEDGER_STREAM_MODE_WRITE));
    int r = encodeToCBOR(data.variant(), stream);
    if (r < 0) {
        // encodeToCBOR() can't forward stream errors
        int err = stream.error();
        if (err < 0) {
            r = err;
        }
        LOG(ERROR, "Failed to encode ledger data: %d", r);
        return r;
    }
    CHECK(stream.close()); // Flush the data
    return 0;
}

int getLedgerData(ledger_instance* ledger, LedgerData& data) {
    LedgerStream stream(ledger);
    CHECK(stream.open(LEDGER_STREAM_MODE_READ));
    Variant v;
    int r = decodeFromCBOR(v, stream);
    if (r < 0) {
        // decodeFromCBOR() can't forward stream errors
        int err = stream.error();
        if (err < 0) {
            r = err;
        }
        if (r == Error::END_OF_STREAM && !stream.bytesRead()) {
            // Treat empty data as an empty map
            data = LedgerData();
            return 0;
        }
        LOG(ERROR, "Failed to decode ledger data: %d", r);
        return r;
    }
    if (!v.isMap()) {
        LOG(ERROR, "Unexpected type of ledger data");
        return Error::BAD_DATA;
    }
    data = std::move(v);
    return 0;
}

} // namespace

int Ledger::set(const LedgerData& data, SetMode mode) {
    if (!isValid()) {
        return Error::INVALID_STATE;
    }
    if (mode == Ledger::REPLACE) {
        CHECK(setLedgerData(instance_, data));
    } else {
        LedgerData d;
        CHECK(getLedgerData(instance_, d));
        for (auto& e: data.variantMap()) {
            if (!d.set(e.first, e.second)) {
                return Error::NO_MEMORY;
            }
        }
        CHECK(setLedgerData(instance_, d));
    }
    return 0;
}

LedgerData Ledger::get() const {
    if (!isValid()) {
        return LedgerData();
    }
    LedgerData data;
    if (getLedgerData(instance_, data) < 0) {
        return LedgerData();
    }
    return data;
}

int64_t Ledger::lastUpdated() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return 0;
    }
    return info.last_updated;
}

int64_t Ledger::lastSynced() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return 0;
    }
    return info.last_synced;
}

size_t Ledger::dataSize() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return 0;
    }
    return info.data_size;
}

const char* Ledger::name() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return "";
    }
    return info.name;
}

LedgerScope Ledger::scope() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return LedgerScope::UNKNOWN;
    }
    return static_cast<LedgerScope>(info.scope);
}

bool Ledger::isWritable() const {
    ledger_info info = {};
    if (!isValid() || getLedgerInfo(instance_, info) < 0) {
        return false;
    }
    // It's allowed to write to a ledger while its sync direction is unknown
    return info.sync_direction == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD ||
            info.sync_direction == LEDGER_SYNC_DIRECTION_UNKNOWN;
}

int Ledger::onSync(OnSyncCallback callback, void* arg) {
    if (!isValid()) {
        return Error::INVALID_STATE;
    }
    if (!callback) {
        return onSync(Ledger::OnSyncFunction());
    }
    return onSync([callback, arg](Ledger ledger) {
        callback(std::move(ledger), arg);
    });
}

int Ledger::onSync(OnSyncFunction callback) {
    if (!isValid()) {
        return Error::INVALID_STATE;
    }
    return setSyncCallback(instance_, std::move(callback));
}

int Ledger::getNames(Vector<String>& namesVec) {
    char** names = nullptr;
    size_t count = 0;
    int r = ledger_get_names(&names, &count, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_get_names() failed: %d", r);
        return r;
    }
    SCOPE_GUARD({
        for (size_t i = 0; i < count; ++i) {
            std::free(names[i]);
        }
        std::free(names);
    });
    namesVec.clear();
    if (!namesVec.reserve(count)) {
        return Error::NO_MEMORY;
    }
    for (size_t i = 0; i < count; ++i) {
        String name(names[i]);
        if (!name.length()) {
            return Error::NO_MEMORY;
        }
        namesVec.append(std::move(name));
    }
    return 0;
}

int Ledger::remove(const char* name) {
    int r = ledger_purge(name, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_purge() failed: %d", r);
        return r;
    }
    return 0;
}

int Ledger::removeAll() {
    int r = ledger_purge_all(nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_purge_all() failed: %d", r);
        return r;
    }
    return 0;
}

} // namespace particle

#endif // Wiring_Ledger
