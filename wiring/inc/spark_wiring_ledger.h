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

#include <type_traits>
#include <utility>
#include <memory>

#include <ArduinoJson.hpp> // TODO: Use a custom variant class

#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_error.h"

#include "system_ledger.h"

#include "c_string.h"

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
        using std::swap; // For ADL
        swap(ledger1.lr_, ledger2.lr_);
    }

private:
    ledger_instance* lr_;

    explicit Ledger(ledger_instance* ledger, bool addRef = true) :
            lr_(ledger) {
        if (addRef && lr_) {
            ledger_add_ref(lr_, nullptr);
        }
    }

    detail::LedgerImpl* impl() const {
        return static_cast<detail::LedgerImpl*>(ledger_get_app_data(lr_, nullptr));
    }

    static Ledger wrap(ledger_instance* ledger) {
        return Ledger(ledger, false);
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
    LedgerEntry entry(const char* name) const;

    /**
     * Set the value of a page entry.
     *
     * Calling the method is equivalent to the following code:
     * ```cpp
     * LedgerPage page = ...;
     * page.entry(name).setValue(value);
     * ```
     *
     * @param name Entry name.
     * @param value Entry value
     * @return `true` if the value was set, or `false` on a memory allocation error.
     */
    template<typename T>
    bool set(const char* name, const T& value);

    /**
     * Get the value of a page entry.
     *
     * Calling the method is equivalent to the following code:
     * ```cpp
     * LedgerPage page = ...;
     * auto value = page.entry(name).value();
     * ```
     *
     * @param name Entry name.
     * @return Entry value.
     */
    ArduinoJson::JsonVariantConst get(const char* name) const;

    /**
     * Check whether a page entry exists.
     *
     * @param name Entry name.
     * @return `true` if the entry exists, otherwise `false`.
     */
    bool has(const char* name) const;

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
     * Convert the contents of the page to a JSON document.
     *
     * @return JSON document.
     */
    ArduinoJson::DynamicJsonDocument toJsonDocument() const;

    /**
     * Convert the contents of the page to a JSON string.
     *
     * @return JSON string.
     */
    String toJson() const;

    /**
     * Synchronize the page.
     *
     * This method writes the pending changes to the filesystem and synchronizes the page with the
     * Cloud.
     *
     * @param options Sync options.
     * @return 0 on success, otherwise an error code defined by `Error::Code`.
     */
    int sync(const LedgerSyncOptions& options = LedgerSyncOptions()) {
        return save(); // TODO
    }

    /**
     * Check if synchronization with the Cloud is in progress for this page.
     *
     * @return `true` if synchronization is in progress, otherwise `false`.
     */
    bool isSyncing() const {
        return false; // TODO
    }

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
    bool isMutable() const; // TODO

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
    Ledger ledger() const; // TODO

    /**
     * Check if the page instance is valid.
     *
     * @return `true` if the instance is valid, otherwise `false`.
     */
    bool isValid() const {
        return p_;
    }

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
        using std::swap; // For ADL
        swap(page1.p_, page2.p_);
    }

private:
    ledger_page* p_;

    explicit LedgerPage(ledger_page* page, bool addRef = true) :
            p_(page) {
        if (addRef && p_) {
            ledger_add_page_ref(p_, nullptr);
        }
    }

    detail::LedgerPageImpl* impl() const {
        return static_cast<detail::LedgerPageImpl*>(ledger_get_page_app_data(p_, nullptr));
    }

    static LedgerPage wrap(ledger_page* page) {
        return LedgerPage(page, false);
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
    LedgerEntry() :
            p_(nullptr) {
    }

    /**
     * Copy constructor.
     *
     * @param entry Ledger entry to copy.
     */
    LedgerEntry(const LedgerEntry& entry) :
            name_(entry.name_),
            doc_(entry.doc_),
            val_(entry.val_),
            p_(entry.p_) {
        if (p_) {
            ledger_add_page_ref(p_, nullptr);
        }
    }

    /**
     * Move constructor.
     *
     * @param entry Ledger entry to copy.
     */
    LedgerEntry(LedgerEntry&& entry) :
            LedgerEntry() {
        swap(*this, entry);
    }

    /**
     * Destructor.
     */
    ~LedgerEntry() {
        if (p_) {
            ledger_release_page(p_, nullptr);
        }
    }

    /**
     * Set the entry value.
     *
     * @param value New value.
     * @return `true` if the value was set, otherwise `false`.
     */
    template<typename T>
    bool setValue(const T& value) {
        return update([&](auto& v) {
            v.set(value);
        });
    }

    /**
     * Get the entry value.
     *
     * @return Entry value.
     */
    ArduinoJson::JsonVariantConst value() const {
        return val_;
    }

    ///@{
    /**
     * Check if the entry value is of a specific type.
     *
     * @return `true` if the value is of the specified type, otherwise `false`.
     */
    bool isNull() const {
        return !val_.isUnbound() && val_.isNull();
    }

    bool isBool() const {
        return val_.is<bool>();
    }

    bool isInt() const {
        return val_.is<int>();
    }

    bool isFloat() const {
        return val_.is<float>();
    }

    bool isDouble() const {
        return val_.is<double>();
    }

    bool isString() const {
        return val_.is<ArduinoJson::JsonString>();
    }

    bool isArray() const {
        return val_.is<ArduinoJson::JsonArray>();
    }

    bool isObject() const {
        return val_.is<ArduinoJson::JsonObject>();
    }
    ///@}

    ///@{
    /**
     * Convert the entry value to a specific type.
     *
     * @return Entry value.
     */
    bool toBool() const {
        return val_.as<bool>();
    }

    int toInt() const {
        return val_.as<int>();
    }

    float toFloat() const {
        return val_.as<float>();
    }

    double toDouble() const {
        return val_.as<double>();
    }

    String toString() const {
        return val_.as<ArduinoJson::JsonString>().c_str();
    }

    ArduinoJson::JsonArray toArray() const {
        return val_.as<ArduinoJson::JsonArray>();
    }

    ArduinoJson::JsonObject toObject() const {
        return val_.as<ArduinoJson::JsonObject>();
    }
    ///@}

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
    int size() const {
        if (val_.is<ArduinoJson::JsonString>()) {
            return val_.as<ArduinoJson::JsonString>().size();
        }
        return val_.size();
    }

    /**
     * Check whether the entry value is empty.
     *
     * @return `true` if the value is an empty array, object or string, otherwise `false`.
     * @see `size()`
     */
    bool isEmpty() const {
        return size() == 0;
    }

    /**
     * Set the entry value to null.
     */
    void clear() {
        update([&](auto& v) {
            v.clear();
        });
    }

    /**
     * Array operations.
     *
     * Note that these operation are not atomic and the resulting value of the entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     */
    ///@{

    /**
     * Append a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     *
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    template<typename T>
    bool pushBack(const T& value) {
        return update([&](auto& v) {
            if (!v.template is<ArduinoJson::JsonArray>()) {
                v = ArduinoJson::JsonArray();
            }
            v.add(value);
        });
    }

    /**
     * Prepend a value to the array.
     *
     * If the entry is not an array, it is set to an empty array prior to the operation.
     *
     * @param value Value to add to the array.
     * @return `true` if the value was added, or `false` on a memory allocation error.
     */
    template<typename T>
    bool pushFront(const T& value); // FIXME: Not supported by ArduinoJson

    /**
     * Remove the first element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     */
    void popBack() {
        update([&](auto& v) {
            if (v.template is<ArduinoJson::JsonArray>()) {
                v.remove(v.size() - 1);
            } else {
                v = ArduinoJson::JsonArray();
            }
        });
    }

    /**
     * Remove the last element of the array.
     *
     * If the entry is not an array, it is set to an empty array.
     */
    void popFront() {
        update([&](auto& v) {
            if (v.template is<ArduinoJson::JsonArray>()) {
                v.remove(0);
            } else {
                v = ArduinoJson::JsonArray();
            }
        });
    }

    /**
     * Remove the element at a given index from the array.
     *
     * If the entry is not an array, it is set to an empty array.
     */
    void removeAt(int index) {
        update([&](auto& v) {
            if (v.template is<ArduinoJson::JsonArray>()) {
                v.remove(index);
            } else {
                v = ArduinoJson::JsonArray();
            }
        });
    }

    /**
     * Get the first element of the array.
     *
     * @return Value of the first element.
     */
    ArduinoJson::JsonVariantConst front() const {
        return val_[0];
    }

    /**
     * Get the last element of the array.
     *
     * @return Value of the last element.
     */
    ArduinoJson::JsonVariantConst back() const {
        return val_[val_.size() - 1];
    }

    /**
     * Get the element at the given index of the array.
     *
     * @param index Index of the element.
     * @return Value of the element.
     */
    ArduinoJson::JsonVariantConst at(int index) const {
        return val_[index];
    }
    ///@}

    /**
     * Object operations.
     *
     * Note that these operation are not atomic and the resulting value of the entry may be
     * overwritten during a synchronization as if the entry was assigned a new value rather than
     * modified in place.
     */
    ///@{

    /**
     * Set a property of the object.
     *
     * If the entry is not an object, it is set to an empty object prior to the operation.
     *
     * @param name Property name.
     * @param value Property value.
     * @return `true` if the property was set, or `false` on a memory allocation error.
     */
    template<typename T>
    bool set(const char* name, const T& value) {
        return update([&](auto& v) {
            if (!v.template is<ArduinoJson::JsonObject>()) {
                v = ArduinoJson::JsonObject();
            }
            v[name] = value;
        });
    }

    /**
     * Get a property of the object.
     *
     * @param name Property name.
     * @return Property value.
     */
    ArduinoJson::JsonVariantConst get(const char* name) const {
        return val_[name];
    }

    /**
     * Remove a property of the object.
     *
     * If the entry is not an object, it is set to an empty object.
     *
     * @param name Property name.
     */
    void remove(const char* name) {
        update([&](auto& v) {
            if (v.template is<ArduinoJson::JsonObject>()) {
                v.remove(name);
            } else {
                v = ArduinoJson::JsonObject();
            }
        });
    }

    /**
     * Check if the object has a property with the given name.
     *
     * @param name Property name.
     * @return `true` if the property exists, otherwise `false`.
     */
    bool has(const char* name) const {
        return val_.containsKey(name);
    }
    ///@}

    /**
     * Get the entry name.
     *
     * @return Entry name.
     */
    const char* name() const {
        return name_;
    }

    /**
     * Check if the entry can be modified locally.
     *
     * @return `true` if the entry can be modified locally, otherwise `false`.
     */
    bool isMutable() const; // TODO

    /**
     * Check if the entry is valid.
     *
     * @return `true` if the entry is valid, otherwise `false`.
     */
    bool isValid() const {
        return p_;
    }

    /**
     * Conversion operator.
     */
    template<typename T>
    operator T() const {
        return val_.as<T>();
    }

    ///@{
    /**
     * Assignment operator.
     */
    LedgerEntry& operator=(LedgerEntry entry) {
        swap(*this, entry);
        return *this;
    }

    template<typename T>
    LedgerEntry& operator=(const T& value) {
        setValue(value);
        return *this;
    }

    LedgerEntry& operator=(const String& value) {
        setValue(value.c_str());
        return *this;
    }
    ///@}

    friend void swap(LedgerEntry& entry1, LedgerEntry& entry2) {
        using std::swap; // For ADL
        swap(entry1.name_, entry2.name_);
        swap(entry1.doc_, entry2.doc_);
        swap(entry1.val_, entry2.val_);
        swap(entry1.p_, entry2.p_);
    }

private:
    CString name_;
    // Keep a reference to the original JSON document so that the JsonVariant that refers to the entry
    // value in that document remains valid if the document is changed elsewhere
    std::shared_ptr<ArduinoJson::DynamicJsonDocument> doc_;
    ArduinoJson::JsonVariant val_;
    ledger_page* p_;

    LedgerEntry(ledger_page* page, CString entryName);

    // TODO: DynamicJsonDocument can't grow dynamically and may update the underlying document partially
    // if there's no enough space in its internal buffer. Below is a quick workaround for those limitations
    // so that we can continue experimenting with the API
    template<typename F>
    bool update(const F& f) {
        if (!doc_) {
            return false;
        }
        size_t capacity = doc_->capacity();
        for (;;) {
            std::shared_ptr<ArduinoJson::DynamicJsonDocument> d(new(std::nothrow) ArduinoJson::DynamicJsonDocument(capacity));
            if (!d || d->capacity() == 0) {
                return false; // Memory allocation error
            }
            d->set(*doc_);
            auto v = (*d)[const_cast<char*>(static_cast<const char*>(name_))]; // Returns a MemberProxy
            f(v);
            if (!d->overflowed()) {
                doc_ = d;
                val_ = v;
                updatePage();
                break;
            }
            capacity = capacity * 3 / 2;
        }
        return true;
    }

    void updatePage();

    detail::LedgerPageImpl* pageImpl() const {
        return static_cast<detail::LedgerPageImpl*>(ledger_get_page_app_data(p_, nullptr));
    }

    friend class detail::LedgerPageImpl;
};

template<typename T>
inline bool LedgerPage::set(const char* name, const T& value) {
    return entry(name).setValue(value);
}

inline ArduinoJson::JsonVariantConst LedgerPage::get(const char* name) const {
    return entry(name).value();
}

inline bool LedgerPage::has(const char* name) const {
    return entry(name).isValid();
}

} // namespace particle
