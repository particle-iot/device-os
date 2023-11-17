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

#include "spark_wiring_platform.h"

#if Wiring_Ledger

#include <functional>
#include <initializer_list>
#include <utility>

#include "spark_wiring_variant.h"

#include "system_ledger.h"

namespace particle {

class LedgerData;

/**
 * Ledger scope.
 */
enum class LedgerScope {
    UNKNOWN = LEDGER_SCOPE_UNKNOWN, ///< Unknown scope.
    DEVICE = LEDGER_SCOPE_DEVICE, ///< Device scope.
    PRODUCT = LEDGER_SCOPE_PRODUCT, ///< Product scope.
    OWNER = LEDGER_SCOPE_OWNER ///< User or organization scope.
};

/**
 * A ledger.
 *
 * Use `Particle.ledger()` to create an instance of this class.
 */
class Ledger {
public:
    /**
     * Mode of operation of the `set()` method.
     */
    enum SetMode {
        REPLACE, ///< Replace the current ledger data.
        MERGE ///< Update some of the entries of the ledger data.
    };

    /**
     * A callback invoked when the ledger data has been synchronized with the Cloud.
     *
     * @param ledger Ledger instance.
     * @param arg Callback argument.
     */
    typedef void (*OnSyncCallback)(Ledger ledger, void* arg);

    /**
     * A callback invoked when the ledger data has been synchronized with the Cloud.
     *
     * @param ledger Ledger instance.
     */
    typedef std::function<void(Ledger)> OnSyncFunction;

    /**
     * Default constructor.
     *
     * Constructs an invalid ledger instance.
     */
    Ledger() :
            Ledger(nullptr) {
    }

    // This constructor is for internal use only
    explicit Ledger(ledger_instance* instance, bool addRef = true) :
            instance_(instance) {
        if (instance_ && addRef) {
            ledger_add_ref(instance_, nullptr);
        }
    }

    /**
     * Copy constructor.
     *
     * @param ledger Ledger instance to copy.
     */
    Ledger(const Ledger& ledger) :
            Ledger(ledger.instance_) {
    }

    /**
     * Move constructor.
     *
     * @param ledger Ledger instance to move from.
     */
    Ledger(Ledger&& ledger) :
            Ledger() {
        swap(*this, ledger);
    }

    /**
     * Destructor.
     */
    ~Ledger() {
        if (instance_) {
            ledger_release(instance_, nullptr);
        }
    }

    /**
     * Set the ledger data.
     *
     * @param data New ledger data.
     * @param mode Mode of operation.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int set(const LedgerData& data, SetMode mode = SetMode::REPLACE);

    /**
     * Get the ledger data.
     *
     * @return Ledger data.
     */
    LedgerData get() const;

    /**
     * Get the time the ledger was last updated, in milliseconds since the Unix epoch.
     *
     * @return Time the ledger was updated, or 0 if the time is unknown.
     */
    int64_t lastUpdated() const;

    /**
     * Get the time the ledger was last synchronized with the Cloud, in milliseconds since the Unix epoch.
     *
     * @return Time the ledger was synchronized, or 0 if the ledger has never been synchronized.
     */
    int64_t lastSynced() const;

    /**
     * Get the size of the ledger data in bytes.
     *
     * @return Data size.
     */
    size_t dataSize() const;

    /**
     * Get the ledger name.
     *
     * @return Ledger name.
     */
    const char* name() const;

    /**
     * Get the ledger scope.
     *
     * @return Ledger scope.
     */
    LedgerScope scope() const;

    /**
     * Check if the ledger data can be modified by the application.
     *
     * @return `true` if the ledger data can be modified, otherwise `false`.
     */
    bool isWritable() const;

    /**
     * Check if the ledger instance is valid.
     *
     * @return `true` if the instance is valid, otherwise `false`.
     */
    bool isValid() const {
        return instance_;
    }

    /**
     * Set a callback to be invoked when the ledger data has been synchronized with the Cloud.
     *
     * @note Setting a callback will keep an instance of this ledger around until the callback is
     * cleared, even if the original instance is no longer referenced in the application code.
     *
     * The callback can be cleared by calling `onSync()` with a `nullptr`.
     *
     * @param callback Callback.
     * @param arg Argument to pass to the callback.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int onSync(OnSyncCallback callback, void* arg = nullptr);

    /**
     * Set a callback to be invoked when the ledger data has been synchronized with the Cloud.
     *
     * @note Setting a callback will keep an instance of this ledger around until the callback is
     * cleared, even if the original instance is no longer referenced in the application code.
     *
     * The callback can be cleared by calling `onSync()` with a `nullptr`.
     *
     * @param callback Callback.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int onSync(OnSyncFunction callback);

    /**
     * Assignment operator.
     *
     * @param ledger Ledger instance to assign from.
     * @return This ledger instance.
     */
    Ledger& operator=(Ledger ledger) {
        swap(*this, ledger);
        return *this;
    }

    /**
     * Comparison operators.
     *
     * Two ledger instances are considered equal if they were created for the same ledger:
     * ```
     * Ledger myLedger = Particle.ledger("my-ledger");
     *
     * void onSyncCallback(Ledger ledger) {
     *     if (ledger == myLedger) {
     *         Log.info("my-ledger synchronized")
     *     }
     * }
     * ```
     */
    ///@{
    bool operator==(const Ledger& ledger) const {
        return instance_ == ledger.instance_;
    }

    bool operator!=(const Ledger& ledger) const {
        return instance_ != ledger.instance_;
    }
    ///@}

    /**
     * Get the names of the ledgers stored on the device.
     *
     * @param[out] names Ledger names.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    static int getNames(Vector<String>& names);

    /**
     * Remove any data associated with a ledger from the device.
     *
     * The device must not be connected to the Cloud. The operation will fail if the ledger with the
     * given name is in use.
     *
     * @note The data is not guaranteed to be removed in an irrecoverable way.
     *
     * @param name Ledger name.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    static int remove(const char* name);

    /**
     * Remove any ledger data from the device.
     *
     * The device must not be connected to the Cloud. The operation will fail if any of the ledgers
     * is in use.
     *
     * @note The data is not guaranteed to be removed in an irrecoverable way.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    static int removeAll();

    friend void swap(Ledger& ledger1, Ledger& ledger2) {
        using std::swap;
        swap(ledger1.instance_, ledger2.instance_);
    }

private:
    ledger_instance* instance_;
};

/**
 * Ledger data.
 *
 * This class provides a subset of methods of the `Variant` class that are relevant to map operations.
 */
class LedgerData {
public:
    /**
     * Data entry.
     *
     * A data entry is an `std::pair` where `first` is the entry name (a `String`) and `second` is
     * the entry value (a `Variant`).
     */
    using Entry = VariantMap::Entry;

    /**
     * Construct empty ledger data.
     */
    LedgerData() :
            v_(VariantMap()) {
    }

    /**
     * Construct ledger data from a `Variant`.
     *
     * If the `Variant` is not a map, empty ledger data is constructed.
     *
     * @param var `Variant` value.
     */
    LedgerData(Variant var) {
        if (var.isMap()) {
            v_ = std::move(var);
        } else {
            v_ = var.toMap();
        }
    }

    /**
     * Construct ledger data from an initializer list.
     *
     * Example usage:
     * ```
     * LedgerData data = { { "key1", "value1" }, { "key2", 2 } };
     * ```
     *
     * @param entries Entries.
     */
    LedgerData(std::initializer_list<Entry> entries) :
            v_(VariantMap(entries)) {
    }

    /**
     * Copy constructor.
     *
     * @param data Ledger data to copy.
     */
    LedgerData(const LedgerData& data) :
            v_(data.v_) {
    }

    /**
     * Move constructor.
     *
     * @param data Ledger data to move from.
     */
    LedgerData(LedgerData&& data) :
            LedgerData() {
        swap(*this, data);
    }

    ///@{
    /**
     * Set the value of an entry.
     *
     * @param name Entry name.
     * @param val Entry value.
     * @return `true` if the entry value was set, or `false` on a memory allocation error.
     */
    bool set(const char* name, Variant val) {
        return v_.set(name, std::move(val));
    }

    bool set(const String& name, Variant val) {
        return v_.set(name, std::move(val));
    }

    bool set(String&& name, Variant val) {
        return v_.set(std::move(name), std::move(val));
    }
    ///@}

    ///@{
    /**
     * Remove an entry.
     *
     * @param name Entry name.
     * @return `true` if the entry was removed, otherwise `false`.
     */
    bool remove(const char* name) {
        return v_.remove(name);
    }

    bool remove(const String& name) {
        return v_.remove(name);
    }
    ///@}

    ///@{
    /**
     * Get the value of an entry.
     *
     * This method is inefficient for complex value types, such as `String`, as it returns a copy of
     * the value. Use `operator[]` to get a reference to the value.
     *
     * A null `Variant` is returned if the entry doesn't exist.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    Variant get(const char* name) const {
        return v_.get(name);
    }

    Variant get(const String& name) const {
        return v_.get(name);
    }
    ///@}

    ///@{
    /**
     * Check if an entry with the given name exists.
     *
     * @param name Entry name.
     * @return `true` if the entry exists, otherwise `false`.
     */
    bool has(const char* name) const {
        return v_.has(name);
    }

    bool has(const String& name) const {
        return v_.has(name);
    }
    ///@}

    /**
     * Get all entries of the ledger data.
     *
     * Example usage:
     * ```
     * LedgerData data = myLedger.get();
     * for (const LedgerData::Entry& entry: data.entries()) {
     *     const String& name = entry.first;
     *     const Variant& value = entry.second;
     *     Log.info("%s: %s", name.c_str(), value.toString().c_str());
     * }
     * ```
     *
     * @return Entries.
     */
    const Vector<Entry>& entries() const {
        return variantMap().entries();
    }

    /**
     * Get the number of entries stored in the ledger data.
     *
     * @return Number of entries.
     */
    int size() const {
        return v_.size();
    }

    /**
     * Get the number of entries that can be stored without reallocating memory.
     *
     * @return Number of entries.
     */
    int capacity() const {
        return variantMap().capacity();
    }

    /**
     * Reserve memory for the specified number of entries.
     *
     * @return `true` on success, or `false` on a memory allocation error.
     */
    bool reserve(int capacity) {
        return variantMap().reserve(capacity);
    }

    /**
     * Reduce the capacity of the ledger data to its actual size.
     *
     * @return `true` on success, or `false` on a memory allocation error.
     */
    bool trimToSize() {
        return variantMap().trimToSize();
    }

    /**
     * Check if the ledger data is empty.
     *
     * @return `true` if the data is empty, otherwise `false`.
     */
    bool isEmpty() const {
        return v_.isEmpty();
    }

    /**
     * Serialize the ledger data as JSON.
     *
     * @return JSON document.
     */
    String toJSON() const {
        return v_.toJSON();
    }

    ///@{
    /**
     * Get a reference to the `VariantMap` containing the entries of the ledger data.
     *
     * @return Reference to the `VariantMap`.
     */
    VariantMap& variantMap() {
        return v_.value<VariantMap>();
    }

    const VariantMap& variantMap() const {
        return v_.value<VariantMap>();
    }
    ///@}

    ///@{
    /**
     * Get a reference to the underlying `Variant`.
     *
     * @return Reference to the `Variant`.
     */
    Variant& variant() {
        return v_;
    }

    const Variant& variant() const {
        return v_;
    }
    ///@}

    ///@{
    /**
     * Get a reference to the entry value.
     *
     * The entry is created if it doesn't exist.
     *
     * @note The device will panic if it fails to allocate memory for the new entry. Use `set()`
     * or the methods provided by `VariantMap` if you need more control over how memory allocation
     * errors are handled.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    Variant& operator[](const char* name) {
        return v_[name];
    }

    Variant& operator[](const String& name) {
        return v_[name];
    }

    Variant& operator[](String&& name) {
        return v_[std::move(name)];
    }
    ///@}

    /**
     * Assignment operator.
     *
     * @param data Ledger data to assign from.
     * @return This ledger data.
     */
    LedgerData& operator=(LedgerData data) {
        swap(*this, data);
        return *this;
    }

    /**
     * Comparison operators.
     *
     * Two instances of ledger data are equal if they contain equal sets of entries.
     */
    ///@{
    bool operator==(const LedgerData& data) const {
        return v_ == data.v_;
    }

    bool operator!=(const LedgerData& data) const {
        return v_ != data.v_;
    }

    bool operator==(const Variant& var) const {
        return v_ == var;
    }

    bool operator!=(const Variant& var) const {
        return v_ != var;
    }
    ///@}

    /**
     * Parse ledger data from JSON.
     *
     * If the root element of the JSON document is not an object, empty ledger data is returned.
     *
     * @param json JSON document.
     * @return Ledger data.
     */
    static LedgerData fromJSON(const char* json) {
        return Variant::fromJSON(json);
    }

    /**
     * Convert a JSON value to ledger data.
     *
     * If the JSON value is not an object, empty ledger data is returned.
     *
     * @param Val JSON value.
     * @return Ledger data.
     */
    static LedgerData fromJSON(const JSONValue& val) {
        return Variant::fromJSON(val);
    }

    friend void swap(LedgerData& data1, LedgerData& data2) {
        using std::swap; // For ADL
        swap(data1.v_, data2.v_);
    }

private:
    Variant v_;
};

inline bool operator==(const Variant& var, const LedgerData& data) {
    return data == var;
}

inline bool operator!=(const Variant& var, const LedgerData& data) {
    return data != var;
}

} // namespace particle

#endif // Wiring_Ledger
