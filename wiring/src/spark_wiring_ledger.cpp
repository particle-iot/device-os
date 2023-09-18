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

#ifndef UNIT_TEST
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include "spark_wiring_platform.h"

#if Wiring_Ledger

#include <algorithm>
#include <type_traits>
#include <limits>
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

struct CborHead {
    uint64_t arg;
    int type;
    int detail;
};

int openLedger(ledger_instance* ledger, int mode, ledger_stream*& stream) {
    int r = ledger_open(&stream, ledger, mode, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_open() failed: %d", r);
    }
    return r;
}

int closeLedger(ledger_stream* stream, int flags = 0) {
    int r = ledger_close(stream, flags, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_close() failed: %d", r);
    }
    return r;
}

int readLedger(ledger_stream* stream, char* data, size_t size) {
    int r = ledger_read(stream, data, size, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_read() failed: %d", r);
    }
    return r;
}

int writeLedger(ledger_stream* stream, const char* data, size_t size) {
    int r = ledger_write(stream, data, size, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_write() failed: %d", r);
    }
    return r;
}

inline int readUint8(ledger_stream* stream, uint8_t& val) {
    CHECK(readLedger(stream, (char*)&val, sizeof(val)));
    return 0;
}

inline int writeUint8(ledger_stream* stream, uint8_t val) {
    CHECK(writeLedger(stream, (const char*)&val, sizeof(val)));
    return 0;
}

inline int readUint16Be(ledger_stream* stream, uint16_t& val) {
    CHECK(readLedger(stream, (char*)&val, sizeof(val)));
    val = bigEndianToNative(val);
    return 0;
}

inline int writeUint16Be(ledger_stream* stream, uint16_t val) {
    val = nativeToBigEndian(val);
    CHECK(writeLedger(stream, (const char*)&val, sizeof(val)));
    return 0;
}

inline int readUint32Be(ledger_stream* stream, uint32_t& val) {
    CHECK(readLedger(stream, (char*)&val, sizeof(val)));
    val = bigEndianToNative(val);
    return 0;
}

inline int writeUint32Be(ledger_stream* stream, uint32_t val) {
    val = nativeToBigEndian(val);
    CHECK(writeLedger(stream, (const char*)&val, sizeof(val)));
    return 0;
}

inline int readUint64Be(ledger_stream* stream, uint64_t& val) {
    CHECK(readLedger(stream, (char*)&val, sizeof(val)));
    val = bigEndianToNative(val);
    return 0;
}

inline int writeUint64Be(ledger_stream* stream, uint64_t val) {
    val = nativeToBigEndian(val);
    CHECK(writeLedger(stream, (const char*)&val, sizeof(val)));
    return 0;
}

int writeFloatBe(ledger_stream* stream, float val) {
    uint32_t v;
    static_assert(sizeof(v) == sizeof(val));
    std::memcpy(&v, &val, sizeof(val));
    v = nativeToBigEndian(v);
    CHECK(writeLedger(stream, (const char*)&v, sizeof(v)));
    return 0;
}

int writeDoubleBe(ledger_stream* stream, double val) {
    uint64_t v;
    static_assert(sizeof(v) == sizeof(val));
    std::memcpy(&v, &val, sizeof(val));
    v = nativeToBigEndian(v);
    CHECK(writeLedger(stream, (const char*)&v, sizeof(v)));
    return 0;
}

int readAndAppendToString(ledger_stream* stream, size_t size, String& str) {
    if (!str.reserve(str.length() + size)) {
        return Error::NO_MEMORY;
    }
    char buf[128];
    while (size > 0) {
        size_t n = std::min(size, sizeof(buf));
        CHECK(readLedger(stream, buf, n));
        str.concat(buf, n);
        size -= n;
    }
    return 0;
}

int readCborHead(ledger_stream* stream, CborHead& head) {
    uint8_t b;
    CHECK(readLedger(stream, (char*)&b, sizeof(b)));
    head.type = b >> 5;
    head.detail = b & 0x1f;
    if (head.detail < 24) {
        head.arg = head.detail;
    } else {
        switch (head.detail) {
        case 24: { // 1-byte argument
            uint8_t v;
            CHECK(readUint8(stream, v));
            head.arg = v;
            break;
        }
        case 25: { // 2-byte argument
            uint16_t v;
            CHECK(readUint16Be(stream, v));
            head.arg = v;
            break;
        }
        case 26: { // 4-byte argument
            uint32_t v;
            CHECK(readUint32Be(stream, v));
            head.arg = v;
            break;
        }
        case 27: { // 8-byte argument
            CHECK(readUint64Be(stream, head.arg));
            break;
        }
        case 31: { // Indefinite length indicator or stop code
            if (head.type == 0 /* Unsigned integer */ || head.type == 1 /* Negative integer */ || head.type == 6 /* Tagged item */) {
                return Error::BAD_DATA;
            }
            head.arg = 0;
            break;
        }
        default: // Reserved
            return Error::BAD_DATA;
        }
    }
    return 0;
}

int writeCborHeadWithArgument(ledger_stream* stream, int type, uint64_t arg) {
    type <<= 5;
    if (arg < 24) {
        CHECK(writeUint8(stream, arg | type));
    } else if (arg <= 0xff) {
        CHECK(writeUint8(stream, 24 /* 1-byte argument */ | type));
        CHECK(writeUint8(stream, arg));
    } else if (arg <= 0xffff) {
        CHECK(writeUint8(stream, 25 /* 2-byte argument */ | type));
        CHECK(writeUint16Be(stream, arg));
    } else if (arg <= 0xffffffffu) {
        CHECK(writeUint8(stream, 26 /* 4-byte argument */ | type));
        CHECK(writeUint32Be(stream, arg));
    } else {
        CHECK(writeUint8(stream, 27 /* 8-byte argument */ | type));
        CHECK(writeUint64Be(stream, arg));
    }
    return 0;
}

int writeCborUnsignedInteger(ledger_stream* stream, uint64_t val) {
    CHECK(writeCborHeadWithArgument(stream, 0 /* Unsigned integer */, val));
    return 0;
}

int writeCborSignedInteger(ledger_stream* stream, int64_t val) {
    if (val < 0) {
        val = -(val + 1);
        CHECK(writeCborHeadWithArgument(stream, 1 /* Negative integer */, val));
    } else {
        CHECK(writeCborHeadWithArgument(stream, 0 /* Unsigned integer */, val));
    }
    return 0;
}

int readCborString(ledger_stream* stream, const CborHead& head, String& str) {
    assert(head.type == 3 /* Text string */);
    String s;
    if (head.detail == 31 /* Indefinite length */) {
        for (;;) {
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                break;
            }
            if (h.type != 3 /* Text string */ || h.detail == 31 /* Indefinite length */) { // Chunks of indefinite length are not permitted
                return Error::BAD_DATA;
            }
            if (h.arg > std::numeric_limits<unsigned>::max()) {
                return Error::BAD_DATA;
            }
            CHECK(readAndAppendToString(stream, h.arg, s));
        }
    } else {
        if (head.arg > std::numeric_limits<unsigned>::max()) {
            return Error::BAD_DATA;
        }
        CHECK(readAndAppendToString(stream, head.arg, s));
    }
    str = std::move(s);
    return 0;
}

int writeCborString(ledger_stream* stream, const String& str) {
    CHECK(writeCborHeadWithArgument(stream, 3 /* Text string */, str.length()));
    CHECK(writeLedger(stream, str.c_str(), str.length()));
    return 0;
}

int encodeVariantToCbor(ledger_stream* stream, const Variant& var) {
    switch (var.type()) {
    case Variant::NULL_: {
        CHECK(writeUint8(stream, 0xf6 /* null */)); // See RFC 8949, Appendix B
        break;
    }
    case Variant::BOOL: {
        auto v = var.value<bool>();
        CHECK(writeUint8(stream, v ? 0xf5 /* true */ : 0xf4 /* false */));
        break;
    }
    case Variant::INT: {
        CHECK(writeCborSignedInteger(stream, var.value<int>()));
        break;
    }
    case Variant::UINT: {
        CHECK(writeCborUnsignedInteger(stream, var.value<unsigned>()));
        break;
    }
    case Variant::INT64: {
        CHECK(writeCborSignedInteger(stream, var.value<int64_t>()));
        break;
    }
    case Variant::UINT64: {
        CHECK(writeCborUnsignedInteger(stream, var.value<uint64_t>()));
        break;
    }
    case Variant::DOUBLE: {
        double d = var.value<double>();
        float f = d;
        if (f == d) {
            CHECK(writeUint8(stream, 0xfa /* Single-precision */));
            CHECK(writeFloatBe(stream, f));
        } else {
            CHECK(writeUint8(stream, 0xfb /* Double-precision */));
            CHECK(writeDoubleBe(stream, d));
        }
        break;
    }
    case Variant::STRING: {
        CHECK(writeCborString(stream, var.value<String>()));
        break;
    }
    case Variant::ARRAY: {
        auto& arr = var.value<VariantArray>();
        CHECK(writeCborHeadWithArgument(stream, 4 /* Array */, arr.size()));
        for (auto& v: arr) {
            CHECK(encodeVariantToCbor(stream, v));
        }
        break;
    }
    case Variant::MAP: {
        auto& entries = var.value<VariantMap>().entries();
        CHECK(writeCborHeadWithArgument(stream, 5 /* Map */, entries.size()));
        for (auto& e: entries) {
            CHECK(writeCborString(stream, e.first));
            CHECK(encodeVariantToCbor(stream, e.second));
        }
        break;
    }
    default:
        assert(false); // Unreachable
        break;
    }
    return 0;
}

// Calling code is expected to parse the head of the data item so that we don't need to peek the
// stream which is not always possible
int decodeVariantFromCbor(ledger_stream* stream, const CborHead& head, Variant& var) {
    switch (head.type) {
    case 0: { // Unsigned integer
        if (head.arg <= std::numeric_limits<unsigned>::max()) {
            var = (unsigned)head.arg; // 32-bit
        } else {
            var = head.arg; // 64-bit
        }
        break;
    }
    case 1: { // Negative integer
        if (head.arg > (uint64_t)std::numeric_limits<int64_t>::max()) {
            return Error::BAD_DATA;
        }
        int64_t v = -(int64_t)head.arg - 1;
        if (v >= std::numeric_limits<int>::min()) {
            var = (int)v; // 32-bit
        } else {
            var = v; // 64-bit
        }
        break;
    }
    case 2: { // Byte string
        return Error::BAD_DATA; // Not supported
    }
    case 3: { // Text string
        String s;
        CHECK(readCborString(stream, head, s));
        var = std::move(s);
        break;
    }
    case 4: { // Array
        VariantArray arr;
        int len = -1;
        if (head.detail != 31 /* Indefinite length */) {
            if (head.arg > (uint64_t)std::numeric_limits<int>::max()) {
                return Error::BAD_DATA;
            }
            len = head.arg;
            if (!arr.reserve(len)) {
                return Error::NO_MEMORY;
            }
        }
        for (;;) {
            if (len >= 0 && arr.size() == len) {
                break;
            }
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                if (len >= 0) {
                    return Error::BAD_DATA; // Unexpected stop code
                }
                break;
            }
            Variant v;
            CHECK(decodeVariantFromCbor(stream, h, v));
            if (!arr.append(std::move(v))) {
                return Error::NO_MEMORY;
            }
        }
        var = std::move(arr);
        break;
    }
    case 5: { // Map
        VariantMap map;
        int len = -1;
        if (head.detail != 31 /* Indefinite length */) {
            if (head.arg > (uint64_t)std::numeric_limits<int>::max()) {
                return Error::BAD_DATA;
            }
            len = head.arg;
            if (!map.reserve(len)) {
                return Error::NO_MEMORY;
            }
        }
        for (;;) {
            if (len >= 0 && map.size() == len) {
                break;
            }
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                if (len >= 0) {
                    return Error::BAD_DATA; // Unexpected stop code
                }
                break;
            }
            if (h.type != 3 /* Text string */) {
                return Error::BAD_DATA; // Non-string keys are not supported
            }
            String k;
            CHECK(readCborString(stream, h, k));
            Variant v;
            CHECK(readCborHead(stream, h));
            CHECK(decodeVariantFromCbor(stream, h, v));
            if (!map.set(std::move(k), std::move(v))) {
                return Error::NO_MEMORY;
            }
        }
        var = std::move(map);
        break;
    }
    case 6: { // Tagged item
        // Skip all tags
        CborHead h;
        do {
            CHECK(readCborHead(stream, h));
        } while (h.type == 6 /* Tagged item */);
        CHECK(decodeVariantFromCbor(stream, h, var));
        break;
    }
    case 7: { // Misc. items
        switch (head.detail) {
        case 20: { // false
            var = false;
            break;
        }
        case 21: { // true
            var = true;
            break;
        }
        case 22: { // null
            var = Variant();
            break;
        }
        case 25: { // Half-precision
            // This code is taken from RFC 8949, Appendix D
            uint16_t half = head.arg;
            unsigned exp = (half >> 10) & 0x1f;
            unsigned mant = half & 0x03ff;
            double val = 0;
            if (exp == 0) {
                val = std::ldexp(mant, -24);
            } else if (exp != 31) {
                val = std::ldexp(mant + 1024, exp - 25);
            } else {
                val = (mant == 0) ? INFINITY : NAN;
            }
            if (half & 0x8000) {
                val = -val;
            }
            var = val;
            break;
        }
        case 26: { // Single-precision
            uint32_t v = head.arg;
            float val;
            static_assert(sizeof(val) == sizeof(v));
            std::memcpy(&val, &v, sizeof(v));
            var = val;
            break;
        }
        case 27: { // Double-precision
            double val;
            static_assert(sizeof(val) == sizeof(head.arg));
            std::memcpy(&val, &head.arg, sizeof(head.arg));
            var = val;
            break;
        }
        default:
            return Error::BAD_DATA; // undefined, reserved, unassigned, or stop code
        }
        break;
    }
    default:
        assert(false); // Unreachable
        break;
    }
    return 0;
}

int decodeVariantFromCbor(ledger_stream* stream, Variant& var) {
    CborHead h;
    CHECK(readCborHead(stream, h));
    CHECK(decodeVariantFromCbor(stream, h, var));
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
    CHECK(openLedger(ledger, LEDGER_STREAM_MODE_WRITE, stream));
    NAMED_SCOPE_GUARD(closeLedgerGuard, {
        closeLedger(stream, LEDGER_STREAM_CLOSE_DISCARD);
    });
    int r = encodeVariantToCbor(stream, data.variant());
    if (r < 0) {
        LOG(ERROR, "Failed to encode ledger data: %d", r);
        return r;
    }
    closeLedgerGuard.dismiss();
    CHECK(closeLedger(stream));
    return 0;
}

int getLedgerData(ledger_instance* ledger, LedgerData& data) {
    ledger_stream* stream = nullptr;
    CHECK(openLedger(ledger, LEDGER_STREAM_MODE_READ, stream));
    NAMED_SCOPE_GUARD(closeLedgerGuard, {
        closeLedger(stream);
    });
    Variant v;
    int r = decodeVariantFromCbor(stream, v);
    if (r < 0) {
        LOG(ERROR, "Failed to decode ledger data: %d", r);
        return r;
    }
    CHECK(closeLedger(stream));
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

} // namespace particle

#endif // Wiring_Ledger
