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

#include "spark_wiring_variant.h"

#include "system_ledger.h"

namespace particle {

class LedgerData;

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
        SET, ///< Replace the current ledger data.
        MERGE ///< Set selected entries of the ledger data.
    };

    /**
     * A callback invoked when the ledger data has been synchronized with the Cloud.
     *
     * @param ledger Ledger instance.
     * @param userData User data.
     */
    typedef void (*OnSyncCallback)(Ledger ledger, void* userData);

    /**
     * Default constructor.
     *
     * Constructs an invalid ledger instance.
     */
    Ledger();

    /**
     * Copy constructor.
     *
     * @param ledger Ledger instance to copy.
     */
    Ledger(const Ledger& ledger);

    /**
     * Move constructor.
     *
     * @param ledger Ledger instance to move from.
     */
    Ledger(Ledger&& ledger);

    /**
     * Destructor.
     */
    ~Ledger();

    /**
     * Set the ledger data.
     *
     * @param data New ledger data.
     * @param mode Mode of operation.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int set(const LedgerData& data, SetMode mode = SetMode::SET);

    /**
     * Get the ledger data.
     *
     * @return Ledger data.
     */
    LedgerData get() const;

    /**
     * Get the last time, in milliseconds since the Unix epoch, when the ledger was synchronized with the Cloud.
     *
     * @return Time the ledger was synchronized with the Cloud, or 0 if the ledger was never synchronized.
     */
    time64_t lastSynced() const;

    /**
     * Get the ledger name.
     *
     * @return Ledger name.
     */
    const char* name() const;

    /**
     * Check if the ledger instance is valid.
     *
     * @return `true` if the instance is valid, otherwise `false`.
     */
    bool isValid() const;

    /**
     * Set a callback to be invoked when the ledger data has been synchronized with the Cloud.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onSync(OnSyncCallback callback, void* userData = nullptr);
    // TODO: Add an overload taking a functor object

    /**
     * Assignment operator.
     *
     * @param ledger Ledger instance to assign from.
     */
    Ledger& operator=(Ledger ledger);

    friend void swap(Ledger& ledger1, Ledger& ledger2);

private:
    ledger_instance* lr_;
};

/**
 * Ledger data.
 */
class LedgerData {
public:
    /**
     * Set the value of an entry.
     *
     * @param name Entry name.
     * @param value Entry value.
     * @return `true` if the value was set, otherwise `false`.
     */
    bool set(const char* name, Variant value) {
        return entries_.set(name, std::move(value));
    }

    /**
     * Get the value of an entry.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    Variant get(const char* name) const {
        return entries_.get(name);
    }

    /**
     * Check if an entry with a given name exists.
     *
     * @param name Entry name.
     * @return `true` if the entry exists, otherwise `false`.
     */
    bool has(const char* name) const {
        return entries_.has(name);
    }

    /**
     * Remove an entry.
     *
     * @param name Entry name.
     * @return `true` if the entry was removed, otherwise `false`.
     */
    bool remove(const char* name) {
        return entries_.remove(name);
    }

    /**
     * Get the names of all entries.
     *
     * @return Entry names.
     */
    Vector<String> entryNames() const {
        return entries_.keys();
    }

    /**
     * Get the value of an entry.
     *
     * The entry is created if it doesn't exist.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    ValueT& operator[](const char* name) {
        return entries_[name];
    }

private:
    VariantMap entries_;
};

} // namespace particle
