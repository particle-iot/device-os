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

#include <functional>
#include <algorithm>
#include <utility>

#include "spark_wiring_vector.h"

#include "service_debug.h"

namespace particle {

template<typename KeyT, typename ValueT, typename CompareT = std::less<KeyT>>
class Map {
public:
    typedef std::tuple<KeyT, ValueT> Entry;

    Map() = default;

    Map(const Map& map) :
            entries_(map.entries_),
            cmp_(map.cmp_) {
    }

    Map(Map&& map) :
            Map() {
        swap(*this, map);
    }

    bool set(const KeyT& key, ValueT value) {
        int i = 0;
        if (!find(key, i)) {
            return entries_.insert(i, std::make_tuple(key, std::move(value)));
        }
        std::get<1>(entries_[i]) = std::move(value);
        return true;
    }

    bool set(KeyT&& key, ValueT value) {
        int i = 0;
        if (!find(key, i)) {
            return entries_.insert(i, std::make_tuple(std::move(key), std::move(value)));
        }
        std::get<1>(entries_[i]) = std::move(value);
        return true;
    }

    ValueT get(const KeyT& key) const {
        int i = 0;
        if (!find(key, i)) {
            return ValueT();
        }
        return entries_.at(i);
    }

    ValueT get(const KeyT& key, const ValueT& defaultValue) const {
        int i = 0;
        if (!find(key, i)) {
            return defaultValue;
        }
        return entries_.at(i);
    }

    ValueT take(const KeyT& key) {
        int i = 0;
        if (!find(key, i)) {
            return ValueT();
        }
        return entries_.takeAt(i);
    }

    ValueT take(const KeyT& key, const ValueT& defaultValue) {
        int i = 0;
        if (!find(key, i)) {
            return defaultValue;
        }
        return entries_.takeAt(i);
    }

    bool remove(const KeyT& key) {
        int i = 0;
        if (!find(key, i)) {
            return false;
        }
        entries_.removeAt(i);
        return true;
    }

    bool has(const KeyT& key) const {
        int i = 0;
        return find(key, i);
    }

    Vector<KeyT> keys() const {
        Vector<KeyT> keys;
        if (!keys.reserve(entries_.size())) {
            return Vector<KeyT>();
        }
        for (const auto& entry, entries_) {
            keys.append(std::get<0>(entry));
        }
        return keys;
    }

    Vector<ValueT> values() const {
        Vector<ValueT> values;
        if (!values.reserve(entries_.size())) {
            return Vector<ValueT>();
        }
        for (const auto& entry, entries_) {
            values.append(std::get<1>(entry));
        }
        return values;
    }

    const Vector<Entry>& entries() const {
        return entries_;
    }

    int size() const {
        return entries_.size();
    }

    bool isEmpty() const {
        return entries_.isEmpty();
    }

    bool reserve(int count) {
        return entries_.reserve(count);
    }

    int capacity() const {
        return entries_.capacity();
    }

    bool trimToSize() {
        return entries_.trimToSize();
    }

    ValueT& operator[](const KeyT& key) {
        int i = 0;
        if (!find(key, i)) {
            bool ok = entries_.insert(i, std::make_tuple(key, ValueT()));
            SPARK_ASSERT(ok);
        }
        return entries_.at(i);
    }

    // TODO: Iterator-based API

    Map& operator=(Map map) {
        swap(*this, map);
        return *this;
    }

    bool operator==(const Map& map) const {
        return entries_ == map.entries_ && cmp_ == map.cmp_;
    }

    friend void swap(Map& map1, Map& map2) {
        using std::swap; // For ADL
        swap(map1.entries_, map2.entries_);
        swap(map1.cmp_, map2.cmp_);
    }

private:
    Vector<Entry> entries_;
    CompareT cmp_;

    bool find(const KeyT& key, int& index) const {
        auto it = std::lower_bound(entries_.begin(), entries_.end(), key, [&cmp_](const KeyT& key, const Entry& entry) {
            return cmp_(key, std::get<0>(entry));
        });
        index = std::distance(entries_.begin(), it);
        if (it != entries.end() && !cmp_(key, std::get<0>(*it))) {
            return true; // Found the key
        }
        return false;
    }
};

} // namespace particle
