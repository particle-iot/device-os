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

#include "spark_wiring_vector.h"

namespace particle::protocol::experimental {

class CoapOptions;

namespace detail {

class CoapOption {
public:
    CoapOption() :
            dataPtr_(nullptr),
            next_(nullptr),
            size_(0),
            num_(0) {
    }

    CoapOption(const CoapOption&) = delete; // Non-copyable

    CoapOption(CoapOption&& opt) :
            CoapOption() {
        swap(*this, opt);
    }

    ~CoapOption() {
        if (size_ > sizeof(char*)) {
            delete[] dataPtr_;
        }
    }

    unsigned number() const {
        return num_;
    }

    const char* data() const {
        return (size_ <= sizeof(char*)) ? data_ : dataPtr_;
    }

    size_t size() const {
        return size_;
    }

    unsigned toUint() const;

    const CoapOption* next() const {
        return next_;
    }

    CoapOption& operator=(const CoapOption&) = delete; // Non-copyable

    CoapOption& operator=(CoapOption&& opt) {
        swap(*this, opt);
        return *this;
    }

protected:
    int init(unsigned num, const char* data, size_t size); // Called by CoapOptions

    void next(const CoapOption* next) { // ditto
        next_ = next;
    }

private:
    union {
        char* dataPtr_;
        char data_[sizeof(char*)]; // Small object optimization
    };
    const CoapOption* next_;
    size_t size_;
    unsigned num_;

    friend void swap(CoapOption& opt1, CoapOption& opt2);

    friend class experimental::CoapOptions;
};

} // namespace detail

/**
 * A container for CoAP options.
 *
 * Maintains a list of immutable CoAP option objects sorted by their numbers. Options can only be
 * added to the list but not removed.
 */
class CoapOptions {
public:
    using Option = detail::CoapOption;

    int add(unsigned num) {
        return add(num, nullptr /* data */, 0 /* size */);
    }

    int add(unsigned num, unsigned val);
    int add(unsigned num, const char* data, size_t size);

    /**
     * Find the first option with a given number.
     *
     * @param num Option number.
     * @return Pointer to the option object or `nullptr` if such an option cannot be found.
     */
    const Option* findFirst(unsigned num) const;

    /**
     * Find the first option with a number greater than a given one.
     *
     * @param num Option number.
     * @return Pointer to the option object or `nullptr` if such an option cannot be found.
     */
    const Option* findNext(unsigned num) const;

    /**
     * Get the first option in the list.
     *
     * @return Pointer to the option object or `nullptr` if the list is empty.
     */
    const Option* first() const {
        return opts_.isEmpty() ? nullptr : &opts_.first();
    }

    size_t size() const {
        return opts_.size();
    }

    bool isEmpty() const {
        return opts_.isEmpty();
    }

private:
    Vector<Option> opts_; // TODO: Use a pool
};

} // namespace particle::protocol::experimental
