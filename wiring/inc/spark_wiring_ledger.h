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

/**
 * A ledger.
 *
 * Instances of this class cannot be created directly. Use the `Particle::ledger()` method to get
 * an instance of this class for a given ledger scope.
 */
class Ledger {
public:
    /**
     * Ledger scopes.
     */
    enum class Scope {
        DEVICE, ///< Device scope.
        PRODUCT, ///< Product scope.
        ORG ///< Organization scope.
    };

    /**
     * A callback invoked when all the linked pages of the ledger have been synchronized with the
     * Cloud or an error occured during the synchronization.
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
     * A callback invoked when a page has changed in the cloud replica of the ledger.
     *
     * The application needs to synchronize the changed page in order for the local replica to be
     * updated accordingly (see `LedgerPage::sync()` and `Ledger::sync()`).
     *
     * @param error Error info.
     * @param pageName Page name.
     * @param userData User data.
     */
    typedef void (*OnPageChangeCallback)(const char* pageName, void* userData);
    /**
     * A callback invoked when a page entry has changed in the cloud replica of the ledger.
     *
     * The application needs to synchronize the changed page in order for the local replica to be
     * updated accordingly (see `LedgerPage::sync()` and `Ledger::sync()`).
     *
     * @param error Error info.
     * @param pageName Page name.
     * @param userData User data.
     */
    typedef void (*OnEntryChangeCallback)(const char* pageName, const char* entryName, void* userData);
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
     * page contents in the cloud replica of the ledger (see `OnPageChangeCallback` and
     * `OnEntryChangeCallback`). Only linked pages are synchronized with the cloud.
     *
     * The requested page is created if it doesn't exist.
     *
     * @param name Page name.
     * @return Ledger page.
     */
    LedgerPage getPage(std::string_view name);
    /**
     * Delete a page.
     *
     * The page is deleted locally and, after synchronization, in the cloud replica of the ledger.
     *
     * @param name Page name.
     */
    void removePage(std::string_view name);
    /**
     * Unlink a page.
     *
     * Unlinking a page causes the device to unsubscribe from notifications for that page and delete
     * its contents locally. The contents of the page in the cloud replica of the ledger is not
     * affected.
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
     * This method writes the pending changes to flash and synchronizes them with the Cloud.
     *
     * @param updateCloud If `true`, the changes will be synchronized with the Cloud. If `false`, the
     *        changes will be only written to flash.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int sync(bool updateCloud = true);
    /**
     * Check if synchronization with the Cloud is in progress.
     *
     * @return `true` if synchronization is in progress, otherwise `false`.
     */
    bool isSyncing() const;
    /**
     * Get the scope of this ledger.
     *
     * @return Ledger scope.
     */
    Scope scope() const;
    /**
     * Set a callback to be invoked when all the linked pages of the ledger have been synchronized
     * with the Cloud
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
     * Set a callback to be invoked when a page has changed in the cloud replica of the ledger.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onPageChange(OnPageChangeCallback callback, void* userData = nullptr);
    /**
     * Set a callback to be invoked when a page entry has changed in the cloud replica of the ledger.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onEntryChange(OnEntryChangeCallback callback, void* userData = nullptr);
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
    LedgerEntry get(std::string_view name);
    /**
     * Set the value of a page entry.
     *
     * @param name Entry name.
     * @param value Entry value
     * @return `true` if the value was set, or `false` on a memory allocation error.
     */
    bool set(std::string_view name, int value); // Alias for get(name).setValue(value)
    bool set(std::string_view name, const char* value);
    // ... overloads for all supported types ...
    bool set(std::string_view name, const JsonDocument& value);
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
     * Convert the page to a JSON object.
     *
     * @return JSON object.
     */
    DynamicJsonDocument toJson() const;
    /**
     * Synchronize the page.
     *
     * This method writes the pending changes to flash and synchronizes them with the Cloud.
     *
     * @param updateCloud If `true`, the changes will be synchronized with the Cloud. If `false`, the
     *        changes will be only written to flash.
     *
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int sync(bool updateCloud = true);
    /**
     * Check if synchronization with the Cloud is in progress for this page.
     *
     * @return `true` if synchronization is in progress, otherwise `false`.
     */
    bool isSyncing() const;
    /**
     * Get the page name.
     *
     * @return Page name.
     */
    const char* name() const;
    /**
     * Get the ledger containing this page.
     *
     * @return Ledger object.
     */
    Ledger ledger() const;

    LedgerEntry operator[](std::string_view name); // Alias for get()

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
     * Get the entry name.
     *
     * @return Entry name.
     */
    const char* name() const;
    /**
     * Get the page containing this entry.
     *
     * @return Page object.
     */
    LedgerPage page() const;
    /**
     * Set the entry value.
     *
     * @param value Entry value.
     * @return `true` if the value was assigned, or `false` on a memory allocation error.
     */
    bool setValue(int value);
    bool setValue(const char* value);
    // ...
    bool setValue(const JsonDocument& value);

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
    // ...
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

    // Numeric operations

    /**
     * Increment the numeric value of the entry.
     *
     * If the entry is not numeric, it is set to 1 as if it was 0 prior to the operation.
     *
     * @param value Value to add to the current entry value.
     * @return New entry value.
     */
    int increment(int value = 1);
    /**
     * Decrement the numeric value of the entry.
     *
     * If the entry is not numeric, it is set to -1 as if it was 0 prior to the operation.
     *
     * @param value Value to substract from the current entry value.
     * @return New entry value.
     */
    int decrement(int value = 1);

    // Array operations

    /**
     * Prepend a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     * 
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    bool pushFront(int value);
    bool pushFront(const char* value);
    // ...
    /**
     * Append a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     * 
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    bool pushBack(int value);
    bool pushBack(const char* value);
    // ...
    /**
     * Remove the first element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     */
    void popFront();
    /**
     * Remove the last element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     */
    void popBack();
    /**
     * Get the first element of the array.
     *
     * If the entry is not an array, the returned value is null-initialized (TBD).
     */
    JsonVariantConst front() const; // TODO: Use a custom variant class
    /**
     * Get the last element of the array.
     *
     * If the entry is not an array, the returned value is null-initialized (TBD).
     */
    JsonVariantConst back() const;

    // Conversion operators

    operator int() const;
    operator String() const;
    // ...
    operator DynamicJsonDocument() const;

    LedgerEntry& operator=(int value);
    LedgerEntry& operator=(const char* value);
    // ...
    LedgerEntry& operator=(const JsonDocument& value);

private:
    LedgerEntry();
};

} // namespace particle
