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

#include "debug.h"

namespace particle {

template<typename KeyT, typename ValueT, typename CompareT = std::less<KeyT>>
class Map {
public:
    typedef KeyT Key;
    typedef ValueT Value;
    typedef CompareT Compare;
    typedef std::pair<KeyT, ValueT> Entry;
    typedef typename Vector<Entry>::Iterator Iterator;
    typedef typename Vector<Entry>::ConstIterator ConstIterator;

    Map() = default;

    Map(std::initializer_list<Entry> entries) :
            entries_(entries) {
        std::sort(entries_.begin(), entries_.end(), [this](const Entry& entry1, const Entry& entry2) {
            return this->cmp_(entry1.first, entry2.first);
        });
    }

    Map(const Map& map) :
            entries_(map.entries_),
            cmp_(map.cmp_) {
    }

    Map(Map&& map) :
            Map() {
        swap(*this, map);
    }

    template<typename T>
    bool set(const T& key, ValueT value) {
        auto r = insert(key, std::move(value));
        if (r.first == entries_.end()) {
            return false;
        }
        return true;
    }

    bool set(KeyT&& key, ValueT value) {
        auto r = insert(std::move(key), std::move(value));
        if (r.first == entries_.end()) {
            return false;
        }
        return true;
    }

    template<typename T>
    ValueT get(const T& key) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return ValueT();
        }
        return it->second;
    }

    template<typename T>
    ValueT get(const T& key, const ValueT& defaultValue) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return defaultValue;
        }
        return it->second;
    }

    template<typename T>
    bool remove(const T& key) {
        auto it = find(key);
        if (it == entries_.end()) {
            return false;
        }
        entries_.erase(it);
        return true;
    }

    template<typename T>
    bool has(const T& key) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return false;
        }
        return true;
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

    void clear() {
        entries_.clear();
    }

    Iterator begin() {
        return entries_.begin();
    }

    ConstIterator begin() const {
        return entries_.begin();
    }

    Iterator end() {
        return entries_.end();
    }

    ConstIterator end() const {
        return entries_.end();
    }

    template<typename T>
    Iterator find(const T& key) {
        auto it = lowerBound(key);
        if (it != entries_.end() && cmp_(key, it->first)) {
            return entries_.end();
        }
        return it;
    }

    template<typename T>
    ConstIterator find(const T& key) const {
        auto it = lowerBound(key);
        if (it != entries_.end() && cmp_(key, it->first)) {
            return entries_.end();
        }
        return it;
    }

    template<typename T>
    std::pair<Iterator, bool> insert(const T& key, ValueT value) {
        auto it = lowerBound(key);
        if (it != entries_.end() && !cmp_(key, it->first)) {
            it->second = std::move(value);
            return std::make_pair(it, false);
        }
        it = entries_.insert(it, std::make_pair(KeyT(key), std::move(value)));
        if (it == entries_.end()) {
            return std::make_pair(it, false);
        }
        return std::make_pair(it, true);
    }

    std::pair<Iterator, bool> insert(KeyT&& key, ValueT value) {
        auto it = lowerBound(key);
        if (it != entries_.end() && !cmp_(key, it->first)) {
            it->second = std::move(value);
            return std::make_pair(it, false);
        }
        it = entries_.insert(it, std::make_pair(std::move(key), std::move(value)));
        if (it == entries_.end()) {
            return std::make_pair(it, false);
        }
        return std::make_pair(it, true);
    }

    Iterator erase(ConstIterator pos) {
        return entries_.erase(pos);
    }

    template<typename T>
    Iterator lowerBound(const T& key) {
        return std::lower_bound(entries_.begin(), entries_.end(), key, [this](const Entry& entry, const T& key) {
            return this->cmp_(entry.first, key);
        });
    }

    template<typename T>
    ConstIterator lowerBound(const T& key) const {
        return std::lower_bound(entries_.begin(), entries_.end(), key, [this](const Entry& entry, const T& key) {
            return this->cmp_(entry.first, key);
        });
    }

    template<typename T>
    Iterator upperBound(const T& key) {
        return std::upper_bound(entries_.begin(), entries_.end(), key, [this](const Entry& entry, const T& key) {
            return this->cmp_(entry.first, key);
        });
    }

    template<typename T>
    ConstIterator upperBound(const T& key) const {
        return std::upper_bound(entries_.begin(), entries_.end(), key, [this](const Entry& entry, const T& key) {
            return this->cmp_(entry.first, key);
        });
    }

    Map& operator=(Map map) {
        swap(*this, map);
        return *this;
    }

    bool operator==(const Map& map) const {
        return entries_ == map.entries_;
    }

    bool operator!=(const Map& map) const {
        return !operator==(map);
    }

    template<typename T>
    ValueT& operator[](const T& key) {
        auto it = lowerBound(key);
        if (it == entries_.end() || cmp_(key, it->first)) {
            it = entries_.insert(it, std::make_pair(KeyT(key), ValueT()));
            SPARK_ASSERT(it != entries_.end());
        }
        return it->second;
    }

    ValueT& operator[](KeyT&& key) {
        auto it = lowerBound(key);
        if (it == entries_.end() || cmp_(key, it->first)) {
            it = entries_.insert(it, std::make_pair(key, ValueT()));
            SPARK_ASSERT(it != entries_.end());
        }
        return it->second;
    }

    friend void swap(Map& map1, Map& map2) {
        using std::swap; // For ADL
        swap(map1.entries_, map2.entries_);
        swap(map1.cmp_, map2.cmp_);
    }

private:
    Vector<Entry> entries_;
    CompareT cmp_;
};

} // namespace particle
