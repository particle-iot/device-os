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
#include <limits>

#include "coap_options.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"

#include "check.h"

namespace particle::protocol::v2 {

unsigned CoapOptionEntry::toUint() const {
    unsigned v = 0;
    CoapMessageDecoder::decodeUintOptionValue(data_, size_, v); // Ignore error
    return v;
}

std::unique_ptr<CoapOptionEntry, CoapOptionEntry::Deleter> CoapOptionEntry::create(unsigned num, const char* data, size_t size) {
    auto p = (CoapOptionEntry*)std::malloc(sizeof(CoapOptionEntry) + size);
    if (!p) {
        return nullptr;
    }
    new(p) CoapOptionEntry(num, size);
    std::memcpy((char*)p + sizeof(CoapOptionEntry), data, size);
    return std::unique_ptr<CoapOptionEntry, Deleter>(p);
}

void CoapOptionEntry::destroy(CoapOptionEntry* entry) {
    if (entry) {
        entry->~CoapOptionEntry();
        std::free(entry);
    }
}

int CoapOptions::add(unsigned num, unsigned val) {
    char d[CoapMessageEncoder::MAX_UINT_OPTION_VALUE_SIZE] = {};
    size_t n = CHECK(CoapMessageEncoder::encodeUintOptionValue(d, sizeof(d), val));
    CHECK(add(num, d, n));
    return 0;
}

int CoapOptions::add(unsigned num, const char* data, size_t size) {
    auto opt = Entry::create(num, data, size);
    if (!opt) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto it = std::upper_bound(opts_.begin(), opts_.end(), num, [](unsigned num, const auto& opt) {
        return num < opt->number();
    });
    it = opts_.insert(it, std::move(opt));
    if (it == opts_.end()) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (it != opts_.begin()) {
        auto prev = std::prev(it);
        (*prev)->next(it->get());
    }
    auto next = std::next(it);
    if (next != opts_.end()) {
        (*it)->next(next->get());
    }
    return 0;
}

const CoapOptions::Entry* CoapOptions::findFirst(unsigned num) const {
    auto it = std::lower_bound(opts_.begin(), opts_.end(), num, [](const auto& opt, unsigned num) {
        return opt->number() < num;
    });
    if (it == opts_.end() || (*it)->number() != num) {
        return nullptr;
    }
    return it->get();
}

const CoapOptions::Entry* CoapOptions::findNext(unsigned num) const {
    auto it = std::upper_bound(opts_.begin(), opts_.end(), num, [](unsigned num, const auto& opt) {
        return num < opt->number();
    });
    if (it == opts_.end()) {
        return nullptr;
    }
    return it->get();
}

} // namespace particle::protocol::v2
