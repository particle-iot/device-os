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

#include <memory>

#include <ArduinoJson.hpp> // TODO: Use a custom variant class

#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_error.h"

#include "system_ledger.h"

class CloudClass; // TODO: Move to the particle namespace

namespace particle {

class LedgerPage;
class LedgerEntry;

namespace detail {

class LedgerImpl;
class LedgerPageImpl;

} // namespace detail

/**
 * Ledger scope.
 */
enum class LedgerScope {
    INVALID = LEDGER_SCOPE_INVALID, ///< Invalid scope.
    DEVICE = LEDGER_SCOPE_DEVICE, ///< Device scope.
    PRODUCT = LEDGER_SCOPE_PRODUCT, ///< Product scope.
    OWNER = LEDGER_SCOPE_OWNER ///< User or organization scope.
};

/**
 * Ledger synchronization options.
 */
class LedgerSyncOptions {
public:
    /**
     * Construct an options object.
     */
    LedgerSyncOptions() :
            strategy_(LEDGER_SYNC_STRATEGY_DEFAULT) {
    }

    /**
     * Indicate whether the local changes should be preferred when synchronizing the page contents.
     *
     * @param enabled `true` if local changes should be preferred, otherwise `false`.
     * @return A reference to this options object.
     */
    LedgerSyncOptions& preferLocalChanges(bool enabled) {
        strategy_ = enabled ? LEDGER_SYNC_STRATEGY_PREFER_LOCAL : LEDGER_SYNC_STRATEGY_PREFER_REMOTE;
        return *this;
    }

    /**
     * Indicate whether the remote changes should be preferred when synchronizing the page contents.
     *
     * @param enabled `true` if remote changes should be preferred, otherwise `false`.
     * @return A reference to this options object.
     */
    LedgerSyncOptions& preferRemoteChanges(bool enabled) {
        strategy_ = enabled ? LEDGER_SYNC_STRATEGY_PREFER_REMOTE : LEDGER_SYNC_STRATEGY_PREFER_LOCAL;
        return *this;
    }

private:
    int strategy_;

    friend class detail::LedgerImpl;
    friend class detail::LedgerPageImpl;
};

/**
 * A ledger.
 */
class Ledger {
public:
    /**
     * A callback invoked when all linked pages of the ledger have been synchronized with the Cloud
     * or an error occured during the synchronization.
     *
     * @param error Error info.
     * @param userData User data.
     */
    typedef void (*OnLedgerSyncCallback)(Error error, void* userData);

    /**
     * A callback invoked when a page has been synchronized with the Cloud or an error occured
     * during the synchronization.
     *
     * @param error Error info.
     * @param pageName Page name.
     * @param userData User data.
     */
    typedef void (*OnPageSyncCallback)(Error error, const char* pageName, void* userData);

    /**
     * A callback invoked when a notification about page changes is received from the Cloud.
     *
     * The application needs to synchronize the changed page in order for the local contents of the
     * page to be updated accordingly (see `LedgerPage::sync()`).
     *
     * @param pageName Page name.
     * @param userData User data.
     */
    typedef void (*OnPageChangeCallback)(const char* pageName, void* userData);

    /**
     * Default constructor.
     *
     * Constructs an invalid ledger instance.
     */
    Ledger() :
            Ledger(nullptr) {
    }

    /**
     * Copy constructor.
     *
     * @param ledger Ledger instance to copy.
     */
    Ledger(const Ledger& ledger) :
            Ledger(ledger.lr_) {
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
        if (lr_) {
            ledger_release(lr_, nullptr);
        }
    }

    /**
     * Get a ledger page.
     *
     * When a page is accessed for the first time, it is _linked_ with its replica in the Cloud.
     *
     * For each linked page, the device will be receiving notifications about changes made to the
     * page contents in the Cloud (see `OnPageChangeCallback`).
     *
     * A page needs to be linked in order for it to be synchronized with the Cloud.
     *
     * The requested page is created if it doesn't exist.
     *
     * @param name Page name.
     * @return Ledger page.
     */
    LedgerPage page(const char* name);

    /**
     * Delete a page.
     *
     * The page is deleted both locally and, after synchronization, in the Cloud.
     *
     * @param name Page name.
     */
    void removePage(const char* name);

    /**
     * Unlink a page.
     *
     * Unlinking a page causes the device to unsubscribe from notifications for that page and delete
     * its contents locally. The contents of the page in the Cloud is not affected.
     *
     * @param name Page name.
     */
    void unlinkPage(const char* name);

    /**
     * Unlink all pages.
     *
     * @see `unlinkPage()`
     */
    void unlinkAllPages();

    /**
     * Check if a page is linked.
     *
     * @param name Page name.
     * @return `true` if the page is linked, otherwise `false`.
     */
    bool isPageLinked(const char* name) const;

    /**
     * Get the names of the linked pages.
     *
     * @return Page names.
     */
    Vector<String> linkedPageNames() const;

    /**
     * Synchronize all linked pages.
     *
     * This method writes the pending changes to the filesystem and synchronizes the changed pages
     * with the Cloud.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int sync();

    /**
     * Check if synchronization with the Cloud is in progress.
     *
     * @return `true` if synchronization is in progress, otherwise `false`.
     */
    bool isSyncing() const;

    /**
     * Save all local changes to the filesystem.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int save();

    /**
     * Check if the ledger can be modified locally.
     *
     * @return `true` if the ledger can be modified locally, otherwise `false`.
     */
    bool isMutable() const;

    /**
     * Set the default sync options.
     *
     * @param options Sync options.
     */
    void setDefaultSyncOptions(const LedgerSyncOptions& options);

    /**
     * Get the default sync options.
     *
     * @return Sync options.
     */
    LedgerSyncOptions defaultSyncOptions() const;

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
     * Check if the ledger instance is valid.
     *
     * @return `true` if the instance is valid, otherwise `false`.
     */
    bool isValid() const {
        return lr_;
    }

    /**
     * Set a callback to be invoked when all linked pages of the ledger have been synchronized with
     * the Cloud
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onLedgerSync(OnLedgerSyncCallback callback, void* userData = nullptr);

    /**
     * Set a callback to be invoked when a page has been synchronized with the Cloud.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onPageSync(OnPageSyncCallback callback, void* userData = nullptr);

    /**
     * Set a callback to be invoked when a page has changed in the Cloud.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onPageChange(OnPageChangeCallback callback, void* userData = nullptr);
    // TODO: Add overloads taking a functor object

    /**
     * Assignment operator.
     *
     * @param ledger Ledger instance to assign from.
     */
    Ledger& operator=(Ledger ledger) {
        swap(*this, ledger);
        return *this;
    }

    friend void swap(Ledger& ledger1, Ledger& ledger2) {
        auto lr = ledger1.lr_;
        ledger1.lr_ = ledger2.lr_;
        ledger2.lr_ = lr;
    }

private:
    ledger_instance* lr_;

    explicit Ledger(ledger_instance* instance, bool addRef = true) :
            lr_(instance) {
        if (addRef && lr_) {
            ledger_add_ref(lr_, nullptr);
        }
    }

    detail::LedgerImpl* impl() const {
        return static_cast<detail::LedgerImpl*>(ledger_get_app_data(lr_, nullptr));
    }

    static Ledger wrap(ledger_instance* instance) {
        return Ledger(instance, false);
    }

    static int getInstance(const char* name, LedgerScope scope, Ledger& ledger); // Called by CloudClass::ledger()
    static void destroyLedgerAppData(void* appData);

    friend class ::CloudClass;
};

/**
 * A ledger page.
 */
class LedgerPage {
public:
    /**
     * Default constructor.
     *
     * Constructs an invalid page instance.
     */
    LedgerPage() :
            LedgerPage(nullptr) {
    }

    /**
     * Copy constructor.
     *
     * @param page Page instance to copy.
     */
    LedgerPage(const LedgerPage& page) :
            LedgerPage(page.p_) {
    }

    /**
     * Move constructor.
     *
     * @param page Page instance to move from.
     */
    LedgerPage(LedgerPage&& page) :
            LedgerPage() {
        swap(*this, page);
    }

    /**
     * Destructor.
     */
    ~LedgerPage() {
        if (p_) {
            ledger_release_page(p_, nullptr);
        }
    }

    /**
     * Get a page entry.
     *
     * The requested entry is created if it doesn't exist.
     *
     * @param name Entry name.
     * @return Entry object.
     */
    LedgerEntry entry(const char* name);

    /**
     * Set the value of a page entry.
     *
     * @param name Entry name.
     * @param value Entry value
     * @return `true` if the value was set, or `false` on a memory allocation error.
     */
    bool set(const char* name, int value); // Alias for entry(name).assign(value)
    bool set(const char* name, const char* value);
    // TODO: Overloads for all supported types
    bool set(const char* name, const ArduinoJson::JsonDocument& value);

    /**
     * Get the value of a page entry.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    ArduinoJson::JsonVariantConst get(const char* name) const; // Alias for entry(name).value()

    /**
     * Remove a page entry.
     *
     * @param name Entry name.
     */
    void remove(const char* name);

    /**
     * Delete all entries.
     */
    void clear();

    /**
     * Check whether a page entry exists.
     *
     * @param name Entry name.
     * @return `true` if the entry exists, otherwise `false`.
     */
    bool has(const char* name) const;

    /**
     * Get the names of the page entries.
     *
     * @return Entry names.
     */
    Vector<String> entryNames() const;

    /**
     * Convert the page contents to a JSON object.
     *
     * @return JSON object.
     */
    ArduinoJson::DynamicJsonDocument toJson() const;

    /**
     * Synchronize the page.
     *
     * This method writes the pending changes to the filesystem and synchronizes the page with the
     * Cloud.
     *
     * @param options Sync options.
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int sync(const LedgerSyncOptions& options = LedgerSyncOptions());

    /**
     * Check if synchronization with the Cloud is in progress for this page.
     *
     * @return `true` if synchronization is in progress, otherwise `false`.
     */
    bool isSyncing() const;

    /**
     * Save the local changes to the filesystem.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int save();

    /**
     * Check if the page can be modified locally.
     *
     * @return `true` if the page can be modified locally, otherwise `false`.
     */
    bool isMutable() const;

    /**
     * Get the page name.
     *
     * @return Page name.
     */
    const char* name() const;

    /**
     * Get the ledger containing this page.
     *
     * @return Ledger instance.
     */
    Ledger ledger() const;

    /**
     * Check if the page instance is valid.
     *
     * @return `true` if the instance is valid, otherwise `false`.
     */
    bool isValid() const;

    /**
     * Assignment operator.
     *
     * @param page Page instance to assign from.
     */
    LedgerPage& operator=(LedgerPage page) {
        swap(*this, page);
        return *this;
    }

    friend void swap(LedgerPage& page1, LedgerPage& page2) {
        auto p = page1.p_;
        page1.p_ = page2.p_;
        page2.p_ = p;
    }

private:
    ledger_page* p_;

    explicit LedgerPage(ledger_page* instance, bool addRef = true) :
            p_(instance) {
        if (addRef && p_) {
            ledger_add_page_ref(p_, nullptr);
        }
    }

    detail::LedgerPageImpl* impl() const {
        return static_cast<detail::LedgerPageImpl*>(ledger_get_page_app_data(p_, nullptr));
    }

    static LedgerPage wrap(ledger_page* instance) {
        return LedgerPage(instance, false);
    }

    friend class detail::LedgerImpl;
};

/**
 * A ledger entry.
 */
class LedgerEntry {
public:
    /**
     * Default constructor.
     *
     * Constructs an invalid entry.
     */
    LedgerEntry();

    /**
     * Entry value types.
     */
    enum class Type {
        NULL_, ///< Null entry.
        BOOL, ///< Boolean entry.
        // TODO: It would make sense to have separate types for integer and floating point types
        // even though JSON doesn't have such a separation
        NUMBER, ///< Numeric entry.
        STRING, ///< String entry.
        ARRAY, ///< Array entry.
        OBJECT ///< Object entry.
    };

    /**
     * Set the entry value.
     *
     * @param value Entry value.
     * @return `true` if the value was assigned, or `false` on a memory allocation error.
     */
    bool assign(int value);
    bool assign(const char* value);
    // TODO: Overloads for all supported types
    bool assign(const ArduinoJson::JsonDocument& value);

    /**
     * Get the entry value.
     *
     * @return Entry value.
     */
    ArduinoJson::JsonVariantConst value() const;

    /**
     * Convert the entry value to a specific type.
     *
     * @return Entry value.
     */
    template<typename T>
    T as() const;

    // Convenience methods
    int toInt() const; // Alias for as<int>()
    String toString() const; // Alias for as<String>()
    // TODO: Overloads for all supported types
    ArduinoJson::DynamicJsonDocument toJson() const; // Alias for as<DynamicJsonDocument>()

    /**
     * Get the entry type.
     *
     * @return Entry type.
     */
    Type type() const;

    // Convenience methods
    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    /**
     * Get the length of the entry value.
     *
     * The value returned by this method depends on the type of the entry value:
     * 
     * - If the entry value is an array, the returned value is the number of elements in that array.
     * - If the entry value is an object, the returned value is the number of properties in that object.
     * - If the entry value is a string, the returned value is the length of the string.
     * - In all other cases this method returns 0.
     *
     * @return Value length.
     */
    int size() const;

    /**
     * Check whether the entry value is empty.
     *
     * @return `true` if the value is an empty array, object or string, otherwise `false`.
     * @see `size()`
     */
    bool isEmpty() const;

    // Array operations

    /**
     * Append a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     *
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    bool pushBack(int value);
    bool pushBack(const char* value);
    // TODO: Overloads for all supported types

    /**
     * Prepend a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     * 
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    void pushFront(int value);
    void pushFront(const char* value);
    // TODO: Overloads for all supported types

    /**
     * Remove the first element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     */
    void popBack();

    /**
     * Remove the last element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     */
    void popFront();

    /**
     * Get the first element of the array.
     *
     * @return Value of the first element.
     */
    ArduinoJson::JsonVariantConst front() const;

    /**
     * Get the last element of the array.
     *
     * @return Value of the last element.
     */
    ArduinoJson::JsonVariantConst last() const;

    /**
     * Get the element at the given index of the array.
     *
     * @param index Index of the element.
     * @return Value of the element.
     */
    ArduinoJson::JsonVariantConst at(int index) const;

    // Object operations

    /**
     * Set a property of the object.
     *
     * If the entry is not an object, it is set to an empty object prior to the operation.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     *
     * @param name Property name.
     * @param value Property value.
     * @return `true` if the property was set, or `false` on a memory allocation error.
     */
    bool set(const char* name, int value);
    bool set(const char* name, const char* value);
    // TODO: Overloads for all supported types

    /**
     * Get a property of the object.
     *
     * @param name Property name.
     * @return Property value.
     */
    ArduinoJson::JsonVariantConst get(const char* name) const;

    /**
     * Remove a property of the object.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     *
     * @param name Property name.
     */
    void remove(const char* name);

    /**
     * Check if the object has a property with the given name.
     *
     * @param name Property name.
     * @return `true` if the property exists, otherwise `false`.
     */
    bool has(const char* name) const;

    /**
     * Get the list of object properties.
     *
     * @return List of object properties.
     */
    Vector<String> propertyNames() const;

    /**
     * Get the entry name.
     *
     * @return Entry name.
     */
    const char* name() const;

    /**
     * Check if the entry can be modified locally.
     *
     * @return `true` if the entry can be modified locally, otherwise `false`.
     */
    bool isMutable() const;

    // Conversion operators

    operator int() const;
    operator String() const;
    // TODO: Overloads for all supported types
    operator ArduinoJson::DynamicJsonDocument() const;

    LedgerEntry& operator=(int value);
    LedgerEntry& operator=(const char* value);
    // TODO: Overloads for all supported types
    LedgerEntry& operator=(const ArduinoJson::JsonDocument& value);

private:
    std::shared_ptr<ArduinoJson::JsonDocument> doc_;
};

} // namespace particle
