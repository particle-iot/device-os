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

#include <ArduinoJson.hpp>

#include "spark_wiring_error.h"

namespace particle {

class LedgerPage;
class LedgerEntry;
class LedgerSyncOptions;

/**
 * Ledger scope.
 */
enum class LedgerScope {
    UNKNOWN, ///< Unknown scope. A sync is required to get the actual scope of the ledger.
    DEVICE, ///< Device scope.
    PRODUCT, ///< Product scope.
    ORG ///< Organization scope.
};

/**
 * A ledger.
 *
 * Instances of this class cannot be created directly. Use the `Particle::ledger()` method to get
 * an instance of this class.
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
     * A callback invoked when a page has changed in the Cloud.
     *
     * The application needs to synchronize the changed page in order for the local contents of the
     * page to be updated accordingly (see `LedgerPage::sync()` and `Ledger::sync()`).
     *
     * @param pageName Page name.
     * @param userData User data.
     */
    typedef void (*OnPageChangeCallback)(const char* pageName, void* userData);
    /**
     * A callback invoked when a ledger error occurs.
     *
     * @param error Error info.
     * @param userData User data.
     */
    typedef void (*OnErrorCallback)(Error error, void* userData);

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
    LedgerPage page(std::string_view name);
    /**
     * Delete a page.
     *
     * The page is deleted both locally and, after synchronization, in the Cloud.
     *
     * @param name Page name.
     */
    void removePage(std::string_view name);
    /**
     * Unlink a page.
     *
     * Unlinking a page causes the device to unsubscribe from notifications for that page and delete
     * its contents locally. The contents of the page in the Cloud is not affected.
     *
     * @param name Page name.
     */
    void unlinkPage(std::string_view name);
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
    bool isPageLinked(std::string_view name) const;
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
    /**
     * Set a callback to be invoked when a ledger error occurs.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onError(OnErrorCallback callback, void* userData = nullptr);
    // TODO: Add overloads taking a functor object

private:
    Ledger();
};

/**
 * A ledger page.
 */
class LedgerPage {
public:
    /**
     * Get a page entry.
     *
     * The requested entry is created if it doesn't exist.
     *
     * @param name Entry name.
     * @return Entry object.
     */
    LedgerEntry entry(std::string_view name);
    /**
     * Set the value of a page entry.
     *
     * @param name Entry name.
     * @param value Entry value
     * @return `true` if the value was set, or `false` on a memory allocation error.
     */
    bool set(std::string_view name, int value); // Alias for entry(name).assign(value)
    bool set(std::string_view name, const char* value);
    // TODO: Overloads for all supported types
    bool set(std::string_view name, const JsonDocument& value);
    /**
     * Get the value of a page entry.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    JsonVariantConst get(std::string_view name) const; // Alias for entry(name).value()
    /**
     * Remove a page entry.
     *
     * @param name Entry name.
     */
    void remove(std::string_view name);
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
    bool has(std::string_view name) const;
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
    DynamicJsonDocument toJson() const;
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

private:
    LedgerPage();
};

/**
 * A ledger entry.
 *
 * Instances of this class cannot be created directly. Use one of the methods of the `LedgerPage`
 * class to get an instance of this class.
 */
class LedgerEntry {
public:
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
    bool assign(const JsonDocument& value);

    /**
     * Get the entry value.
     *
     * @return Entry value.
     */
    JsonVariantConst value() const; // TODO: Use a custom variant class
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
    DynamicJsonDocument toJson() const; // Alias for as<DynamicJsonDocument>()

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
    JsonVariantConst front() const;
    /**
     * Get the last element of the array.
     *
     * @return Value of the last element.
     */
    JsonVariantConst last() const;
    /**
     * Get the element at the given index of the array.
     *
     * @param index Index of the element.
     * @return Value of the element.
     */
    JsonVariantConst at(int index) const;

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
    bool set(std::string_view name, int value);
    bool set(std::string_view name, const char* value);
    // TODO: Overloads for all supported types
    /**
     * Get a property of the object.
     *
     * @param name Property name.
     * @return Property value.
     */
    JsonVariantCont get(std::string_view name) const;
    /**
     * Remove a property of the object.
     *
     * Note that this operation is not atomic and the resulting value of this entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     *
     * @param name Property name.
     */
    void remove(std::string_view name);
    /**
     * Check if the object has a property with the given name.
     *
     * @param name Property name.
     * @return `true` if the property exists, otherwise `false`.
     */
    bool has(std::string_view name) const;

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
    operator DynamicJsonDocument() const;

    LedgerEntry& operator=(int value);
    LedgerEntry& operator=(const char* value);
    // TODO: Overloads for all supported types
    LedgerEntry& operator=(const JsonDocument& value);

private:
    LedgerEntry();
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
            strategy_(Strategy::DEFAULT) {
    }
    /**
     * Indicate whether the local changes should be preferred when synchronizing the page contents.
     *
     * @param enabled `true` if local changes should be preferred, otherwise `false`.
     * @return A reference to this options object.
     */
    LedgerSyncOptions& preferLocalChanges(bool enabled) {
        strategy_ = enabled ? Strategy::PREFER_LOCAL_CHANGES : Strategy::PREFER_REMOTE_CHANGES;
        return *this;
    }
    /**
     * Check whether the local changes should be preferred when synchronizing the page contents.
     *
     * @return `true` if local changes should be preferred, otherwise `false`.
     */
    bool preferLocalChanges() const {
        return strategy_ == Strategy::PREFER_LOCAL_CHANGES;
    }
    /**
     * Indicate whether the remote changes should be preferred when synchronizing the page contents.
     *
     * @param enabled `true` if remote changes should be preferred, otherwise `false`.
     * @return A reference to this options object.
     */
    LedgerSyncOptions& preferRemoteChanges(bool enabled) {
        strategy_ = enabled ? Strategy::PREFER_REMOTE_CHANGES : Strategy::PREFER_LOCAL_CHANGES;
        return *this;
    }
    /**
     * Check whether the remote changes should be preferred when synchronizing the page contents.
     *
     * @return `true` if remote changes should be preferred, otherwise `false`.
     */
    bool preferRemoteChanges() const {
        return strategy_ == Strategy::PREFER_REMOTE_CHANGES;
    }

private:
    enum class Strategy {
        DEFAULT = 0,
        PREFER_LOCAL_CHANGES = 1,
        PREFER_REMOTE_CHANGES = 2
    };

    Strategy strategy_;
};

} // namespace particle
