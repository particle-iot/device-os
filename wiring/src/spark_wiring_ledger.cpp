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

#include <type_traits>
#include <memory>
#include <cmath>
#include <cstdint>
#include <cassert>

#include "spark_wiring_ledger.h"
#include "spark_wiring_print.h"

#include "spark_wiring_error.h"

#include "system_task.h"

#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace {

int writeUint8(uint8_t val, ledger_stream* stream) {
    CHECK(ledger_write(stream, (const char*)&val, sizeof(val), nullptr));
    return 0;
}

int writeUint16Be(uint16_t val, ledger_stream* stream) {
    val = nativeToBigEndian(val);
    CHECK(ledger_write(stream, (const char*)&val, sizeof(val), nullptr));
    return 0;
}

int writeUint32Be(uint32_t val, ledger_stream* stream) {
    val = nativeToBigEndian(val);
    CHECK(ledger_write(stream, (const char*)&val, sizeof(val), nullptr));
    return 0;
}

int writeUint64Be(uint64_t val, ledger_stream* stream) {
    val = nativeToBigEndian(val);
    CHECK(ledger_write(stream, (const char*)&val, sizeof(val), nullptr));
    return 0;
}

int writeDoubleBe(double val, ledger_stream* stream) {
    uint64_t v;
    std::memcpy(&v, &val, 8);
    v = nativeToBigEndian(v);
    CHECK(ledger_write(stream, (const char*)&v, sizeof(v), nullptr));
    return 0;
}

int writeCborArgument(uint64_t val, int type, ledger_stream* stream) {
    type <<= 5;
    if (val <= 0x17) {
        CHECK(writeUint8(val | type, stream));
    } else if (val <= 0xff) {
        CHECK(writeUint8(0x18 | type, stream));
        CHECK(writeUint8(val, stream));
    } else if (val <= 0xffff) {
        CHECK(writeUint8(0x19 | type, stream));
        CHECK(writeUint16Be(val, stream));
    } else if (val <= 0xffffffffu) {
        CHECK(writeUint8(0x1a | type, stream));
        CHECK(writeUint32Be(val, stream));
    } else {
        CHECK(writeUint8(0x1b | type, stream));
        CHECK(writeUint64Be(val, stream));
    }
    return 0;
}

template<typename T, typename std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
int writeIntegerAsCbor(T val, ledger_stream* stream) {
    CHECK(writeCborArgument(val, 0 /* Unsigned integer */, stream));
    return 0;
}

template<typename T, typename std::enable_if_t<std::is_signed_v<T>, int> = 0>
int writeIntegerAsCbor(T val, ledger_stream* stream) {
    if (val < 0) {
        val = std::abs(val + 1);
        CHECK(writeCborArgument(val, 1 /* Negative integer */, stream));
    } else {
        CHECK(writeCborArgument(val, 0 /* Unsigned integer */, stream));
    }
    return 0;
}

int writeStringAsCbor(const String& str, ledger_stream* stream) {
    CHECK(writeCborArgument(str.length(), 3 /* Text string */, stream));
    CHECK(ledger_write(stream, str.c_str(), str.length(), nullptr));
    return 0;
}

int writeVariantAsCbor(const Variant& var, ledger_stream* stream) {
    switch (var.type()) {
    case Variant::NULL_: {
        CHECK(writeUint8(0xf6, stream));
        break;
    }
    case Variant::BOOL: {
        auto v = var.value<bool>();
        CHECK(writeUint8(v ? 0xf5 : 0xf4, stream));
        break;
    }
    case Variant::INT: {
        CHECK(writeIntegerAsCbor(var.value<int>(), stream));
        break;
    }
    case Variant::UINT: {
        CHECK(writeIntegerAsCbor(var.value<unsigned>(), stream));
        break;
    }
    case Variant::INT64: {
        CHECK(writeIntegerAsCbor(var.value<int64_t>(), stream));
        break;
    }
    case Variant::UINT64: {
        CHECK(writeIntegerAsCbor(var.value<uint64_t>(), stream));
        break;
    }
    case Variant::DOUBLE: {
        CHECK(writeUint8(0xfb, stream));
        CHECK(writeDoubleBe(var.value<double>(), stream));
        break;
    }
    case Variant::STRING: {
        CHECK(writeStringAsCbor(var.value<String>(), stream));
        break;
    }
    case Variant::ARRAY: {
        auto& arr = var.value<VariantArray>();
        CHECK(writeCborArgument(arr.size(), 4 /* Array */, stream));
        for (auto& v: arr) {
            CHECK(writeVariantAsCbor(v, stream));
        }
        break;
    }
    case Variant::MAP: {
        auto& entries = var.value<VariantMap>().entries();
        CHECK(writeCborArgument(entries.size(), 5 /* Map */, stream));
        for (auto& e: entries) {
            CHECK(writeStringAsCbor(e.first, stream));
            CHECK(writeVariantAsCbor(e.second, stream));
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

struct LedgerAppData {
    Ledger::OnSyncFunction onSync;
};

void destroyAppData(void* appData) {
    delete static_cast<LedgerAppData*>(appData);
}

// Callback wrapper executed in the application thread
void syncCallbackApp(void* data) {
    auto ledger = static_cast<ledger_instance*>(data);
    ledger_lock(ledger, nullptr);
    SCOPE_GUARD({
        ledger_unlock(ledger, nullptr);
        ledger_release(ledger, nullptr);
    });
    auto appData = static_cast<LedgerAppData*>(ledger_get_app_data(ledger, nullptr));
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

// TODO: Generalize this code when there are more callbacks
int setSyncCallback(ledger_instance* ledger, Ledger::OnSyncFunction callback) {
    ledger_lock(ledger, nullptr);
    SCOPE_GUARD({
        ledger_unlock(ledger, nullptr);
    });
    std::unique_ptr<LedgerAppData> newAppData;
    auto appData = static_cast<LedgerAppData*>(ledger_get_app_data(ledger, nullptr));
    if (!appData) {
        if (!callback) {
            return 0; // Nothing to clear
        }
        newAppData.reset(new(std::nothrow) LedgerAppData());
        if (!newAppData) {
            return Error::NO_MEMORY;
        }
        appData = newAppData.get();
    } else if (!callback) {
        ledger_set_callbacks(ledger, nullptr, nullptr); // Clear the callback
        ledger_set_app_data(ledger, nullptr, nullptr, nullptr); // Clear the app data
        delete appData; // Destroy the app data
        ledger_release(ledger, nullptr); // See below
        return 0;
    }
    appData->onSync = std::move(callback);
    if (newAppData) {
        ledger_set_app_data(ledger, newAppData.release(), destroyAppData, nullptr); // Transfer ownership over the app data to the system
        ledger_callbacks callbacks = {};
        callbacks.version = LEDGER_API_VERSION;
        callbacks.sync = syncCallbackSystem;
        ledger_set_callbacks(ledger, &callbacks, nullptr);
        // Keep the ledger instance around until the callback is cleared
        ledger_add_ref(ledger, nullptr);
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
    ledger_stream* stream = nullptr;
    int r = ledger_open(&stream, ledger, LEDGER_STREAM_MODE_WRITE, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_open() failed: %d", r);
        return r;
    }
    NAMED_SCOPE_GUARD(g, {
        ledger_close(stream, LEDGER_STREAM_CLOSE_DISCARD, nullptr);
    });
    r = writeVariantAsCbor(data.variant(), stream);
    if (r < 0) {
        LOG(ERROR, "ledger_write() failed: %d", r);
        return r;
    }
    g.dismiss();
    r = ledger_close(stream, 0, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_close() failed: %d", r);
        return r;
    }
    return 0;
}

int getLedgerData(ledger_instance* ledger, LedgerData& data) {
    ledger_stream* stream = nullptr;
    int r = ledger_open(&stream, ledger, LEDGER_STREAM_MODE_READ, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_open() failed: %d", r);
        return r;
    }
    NAMED_SCOPE_GUARD(g, {
        ledger_close(stream, 0, nullptr);
    });
    // TODO: Use a binary format
    String str;
    OutputStringStream strStream(str);
    char buf[128];
    for (;;) {
        r = ledger_read(stream, buf, sizeof(buf), nullptr);
        if (r < 0) {
            if (r == SYSTEM_ERROR_END_OF_STREAM) {
                break;
            }
            LOG(ERROR, "ledger_read() failed: %d", r);
            return r;
        }
        strStream.write((uint8_t*)buf, r);
    }
    g.dismiss();
    r = ledger_close(stream, 0, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_close() failed: %d", r);
        return r;
    }
    data = Variant::fromJSON(str);
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

} // namespace particle

#endif // Wiring_Ledger
