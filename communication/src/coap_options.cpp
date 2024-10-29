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

#include <algorithm>

#include "coap_options.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"

#include "check.h"

namespace particle::protocol::experimental {

namespace detail {

unsigned CoapOption::toUint() const {
    unsigned v = 0;
    auto d = (size_ <= sizeof(char*)) ? data_ : dataPtr_;
    CoapMessageDecoder::decodeUintOptionValue(d, size_, v); // Ignore error
    return v;
}

int CoapOption::init(unsigned num, const char* data, size_t size) {
    char* d = data_;
    if (size > sizeof(char*)) {
        d = new(std::nothrow) char[size];
        if (!d) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        dataPtr_ = d;
    }
    std::memcpy(d, data, size);
    size_ = size;
    num_ = num;
    return 0;
}

void swap(CoapOption& opt1, CoapOption& opt2) {
    // Instances of this class are trivially swappable despite not being copy-assignable
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    char tmp[sizeof(CoapOption)];
    std::memcpy(tmp, &opt1, sizeof(CoapOption));
    std::memcpy(&opt1, &opt2, sizeof(CoapOption));
    std::memcpy(&opt2, tmp, sizeof(CoapOption));
#pragma GCC diagnostic pop
}

} // namespace detail

int CoapOptions::add(unsigned num, unsigned val) {
    char d[CoapMessageEncoder::MAX_UINT_OPTION_VALUE_SIZE] = {};
    size_t n = CoapMessageEncoder::encodeUintOptionValue(d, sizeof(d), val);
    CHECK(add(num, d, n));
    return 0;
}

int CoapOptions::add(unsigned num, const char* data, size_t size) {
    Option opt;
    CHECK(opt.init(num, data, size));
    auto it = std::upper_bound(opts_.begin(), opts_.end(), num, [](unsigned num, const Option& opt) {
        return num < opt.number();
    });
    it = opts_.insert(it, std::move(opt));
    if (it == opts_.end()) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (it != opts_.begin()) {
        auto prev = std::prev(it);
        prev->next(&*it);
    }
    auto next = std::next(it);
    if (next != opts_.end()) {
        it->next(&*next);
    }
    return 0;
}

const CoapOptions::Option* CoapOptions::findFirst(unsigned num) const {
    auto it = std::lower_bound(opts_.begin(), opts_.end(), num, [](const Option& opt, unsigned num) {
        return opt.number() < num;
    });
    if (it == opts_.end() || it->number() != num) {
        return nullptr;
    }
    return &*it;
}

const CoapOptions::Option* CoapOptions::findNext(unsigned num) const {
    auto it = std::upper_bound(opts_.begin(), opts_.end(), num, [](unsigned num, const Option& opt) {
        return num < opt.number();
    });
    if (it == opts_.end()) {
        return nullptr;
    }
    return &*it;
}

} // namespace particle::protocol::experimental
