/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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
#include <cstring>
#include <cstdint>

#include "coap_defs.h"

#include "spark_wiring_vector.h"

namespace particle::protocol::v2 {

class CoapOptions;

/**
 * A CoAP option.
 */
class CoapOptionEntry {
public:
    CoapOptionEntry(const CoapOptionEntry&) = delete; // Non-copyable

    /**
     * Get the option number.
     *
     * @return Option number.
     */
    unsigned number() const {
        return num_;
    }

    /**
     * Get the option data.
     *
     * @return Option data.
     */
    const char* data() const {
        return data_;
    }

    /**
     * Get the size of the option data.
     *
     * @return Data size.
     */
    size_t size() const {
        return size_;
    }

    /**
     * Get the value of the `uint` option.
     *
     * @return Option value.
     */
    unsigned toUint() const;

    /**
     * Get the next option in the list.
     *
     * @return Pointer to the next option object or `nullptr` if this is the last option in the list.
     */
    const CoapOptionEntry* next() const {
        return next_;
    }

    CoapOptionEntry& operator=(const CoapOptionEntry&) = delete; // Non-copyable

protected:
    struct Deleter {
        void operator()(CoapOptionEntry* entry) {
            CoapOptionEntry::destroy(entry);
        }
    };

    // Methods called by CoapOptions

    void next(const CoapOptionEntry* next) {
        next_ = next;
    }

    static std::unique_ptr<CoapOptionEntry, Deleter> create(unsigned num, const char* data, size_t size);

private:
    // Keep this structure as small as possible
    const CoapOptionEntry* next_;
    uint16_t num_;
    uint16_t size_;
    char data_[]; // Option data is allocated at the end of this structure

    CoapOptionEntry(unsigned num, size_t size) :
            next_(nullptr),
            num_(num),
            size_(size) {
    }

    ~CoapOptionEntry() = default;

    static void destroy(CoapOptionEntry* entry);

    friend class v2::CoapOptions;
};

/**
 * A container for CoAP options.
 *
 * Maintains a list of immutable CoAP option objects sorted by their numbers. Options can only be
 * added to the list but not removed.
 */
class CoapOptions {
public:
    using Entry = CoapOptionEntry;

    /**
     * Add an empty option.
     *
     * @param num Option number.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int add(unsigned num) {
        return add(num, nullptr /* data */, 0 /* size */);
    }

    /**
     * Add a `uint` option.
     *
     * @param num Option number.
     * @param val Option value.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int add(unsigned num, unsigned val);

    /**
     * Add a string option.
     *
     * @param num Option number.
     * @param val Option value.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int add(unsigned num, const char* val) {
        return add(num, val, std::strlen(val));
    }

    /**
     * Add an opaque option.
     *
     * @param num Option number.
     * @param data Option data.
     * @param size Data size.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int add(unsigned num, const char* data, size_t size);

    // An overload taking a CoapOption enum class value as the option number
    template<typename... ArgsT>
    int add(CoapOption num, ArgsT&&... args) {
        return add((unsigned)num, std::forward<ArgsT>(args)...);
    }

    /**
     * Find the first option with a given number.
     *
     * @param num Option number.
     * @return Pointer to the option object or `nullptr` if such an option cannot be found.
     */
    const Entry* findFirst(unsigned num) const;

    const Entry* findFirst(CoapOption num) const {
        return findFirst((unsigned)num);
    }

    /**
     * Find the first option with a number greater than a given one.
     *
     * @param num Option number.
     * @return Pointer to the option object or `nullptr` if such an option cannot be found.
     */
    const Entry* findNext(unsigned num) const;

    const Entry* findNext(CoapOption num) const {
        return findNext((unsigned)num);
    }

    /**
     * Get the first option in the list.
     *
     * @return Pointer to the option object or `nullptr` if the list is empty.
     */
    const Entry* first() const {
        return opts_.isEmpty() ? nullptr : opts_.first().get();
    }

    /**
     * Get the number of options stored in the list.
     *
     * @return Number of options.
     */
    size_t size() const {
        return opts_.size();
    }

    /**
     * Check if the list is empty.
     *
     * @return `true` if the list is empty, or `false` otherwise.
     */
    bool isEmpty() const {
        return opts_.isEmpty();
    }

private:
    Vector<std::unique_ptr<Entry, Entry::Deleter>> opts_; // TODO: Use a pool
};

} // namespace particle::protocol::v2
