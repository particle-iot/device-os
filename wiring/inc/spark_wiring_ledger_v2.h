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
#include "spark_wiring_print.h"

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
     * Set the ledger data.
     *
     * This method replaces the current contents of the ledger.
     *
     * @param data New ledger data.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int set(const LedgerData& data);

    /**
     * Update the ledger data.
     *
     * This method replaces the current contents of the ledger.
     *
     * @param data New ledger data.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int update(const LedgerData& data);

    /**
     * Get the ledger data.
     *
     * @return Ledger data.
     */
    LedgerData get() const;

    /**
     * Get the last time, in milliseconds since the Unix epoch, the ledger was synchronized with the Cloud.
     *
     * @return Time the ledger was synchronized with the Cloud, or 0 if the ledger was never synchronized.
     */
    time64_t lastSyncedAt() const;

    /**
     * Get the ledger name.
     *
     * @return Ledger name.
     */
    const char* name() const;

    /**
     * Set a callback to be invoked when the ledger data has been synchronized with the Cloud.
     *
     * @param callback Callback.
     * @param userData User data.
     */
    void onSync(OnSyncCallback callback, void* userData = nullptr);
    // TODO: Add an overload taking a functor object
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
    bool set(const char* name, Variant value);

    /**
     * Get the value of an entry.
     *
     * @param name Entry name.
     * @return Entry value.
     */
    Variant get(const char* name) const;

    /**
     * Check if an entry with a given name exists.
     *
     * @param name Entry name.
     * @return `true` if the entry exists, otherwise `false`.
     */
    bool has(const char* name) const;

    /**
     * Remove an entry.
     *
     * @param name Entry name.
     * @return `true` if the entry was removed, otherwise `false`.
     */
    bool remove(const char* name);

private:
    enum EntryFlag {
        SET = 0x01,
        REMOVED = 0x02
    };

    struct Entry {
        Variant data;
        int flags;
    };

    Map<String, Entry> entries_;
};

} // namespace particle
