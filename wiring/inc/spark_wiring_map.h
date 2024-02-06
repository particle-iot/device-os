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

/**
 * An ordered associative container with unique keys.
 *
 * Internally, `Map` stores its entries in a dynamically allocated array. The entries are sorted by
 * key and binary search is used for lookup.
 *
 * @tparam KeyT Key type.
 * @tparam ValueT Value type.
 * @tparam CompareT Comparator type.
 */
template<typename KeyT, typename ValueT, typename CompareT = std::less<KeyT>>
class Map {
public:
    /**
     * Key type.
     */
    typedef KeyT Key;

    /**
     * Value type.
     */
    typedef ValueT Value;

    /**
     * Comparator type.
     */
    typedef CompareT Compare;

    /**
     * Entry type.
     */
    typedef std::pair<const KeyT, ValueT> Entry;

    /**
     * Iterator type.
     */
    typedef typename Vector<Entry>::Iterator Iterator;

    /**
     * Constant interator type.
     */
    typedef typename Vector<Entry>::ConstIterator ConstIterator;

    /**
     * Construct an empty map.
     */
    Map() = default;

    /**
     * Construct a map from an initializer list.
     *
     * @param entries Entries.
     */
    Map(std::initializer_list<Entry> entries) :
            Map() {
        Map<KeyT, ValueT> map;
        if (!map.reserve(entries.size())) {
            return;
        }
        for (auto& e: entries) {
            if (!map.set(e.first, e.second)) {
                return;
            }
        }
        swap(*this, map);
    }

    /**
     * Copy constructor.
     *
     * @param map Map to copy.
     */
    Map(const Map& map) :
            entries_(map.entries_),
            cmp_(map.cmp_) {
    }

    /**
     * Move constructor.
     *
     * @param map Map to move from.
     */
    Map(Map&& map) :
            Map() {
        swap(*this, map);
    }

    ///@{
    /**
     * Add or update an entry.
     *
     * @param key Key.
     * @param val Value.
     * @return `true` if the entry was added or updated, or `false` on a memory allocation error.
     */
    template<typename T>
    bool set(const T& key, ValueT val) {
        auto r = insert(key, std::move(val));
        if (r.first == entries_.end()) {
            return false;
        }
        return true;
    }

    bool set(KeyT&& key, ValueT val) {
        auto r = insert(std::move(key), std::move(val));
        if (r.first == entries_.end()) {
            return false;
        }
        return true;
    }
    ///@}

    /**
     * Get the value of an entry.
     *
     * This method is inefficient for complex value types as it returns a copy of the entry value.
     * Use `operator[]` to get a reference to the entry value, or `find()` to get an iterator
     * pointing to the value.
     *
     * A default-constructed value is returned if an entry with the given key cannot be found.
     *
     * @param key Key.
     * @return Value.
     */
    template<typename T>
    ValueT get(const T& key) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return ValueT();
        }
        return it->second;
    }

    /**
     * Get the value of an entry.
     *
     * This method is inefficient for complex value types as it returns a copy of the entry value.
     * Use `operator[]` to get a reference to the entry value, or `find()` to get an iterator
     * pointing to the value.
     *
     * @param key Key.
     * @param defaultVal Value to return if an entry with the given key cannot be found.
     * @return Value.
     */
    template<typename T>
    ValueT get(const T& key, const ValueT& defaultVal) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return defaultVal;
        }
        return it->second;
    }

    /**
     * Remove an entry.
     *
     * @param key Key.
     * @return `true` if the entry was removed, otherwise `false`.
     */
    template<typename T>
    bool remove(const T& key) {
        auto it = find(key);
        if (it == entries_.end()) {
            return false;
        }
        entries_.erase(it);
        return true;
    }

    /**
     * Check if the map contains an entry.
     *
     * @param key Key.
     * @return `true` if an entry with the given key is found, otherwise `false`.
     */
    template<typename T>
    bool has(const T& key) const {
        auto it = find(key);
        if (it == entries_.end()) {
            return false;
        }
        return true;
    }

    /**
     * Get all entries of the map.
     *
     * @return Entries.
     */
    const Vector<Entry>& entries() const {
        return entries_;
    }

    /**
     * Get the number of entries in the map.
     *
     * @return Number of entries.
     */
    int size() const {
        return entries_.size();
    }

    /**
     * Check if the map is empty.
     *
     * @return `true` if the map is empty, otherwise `false`.
     */
    bool isEmpty() const {
        return entries_.isEmpty();
    }

    /**
     * Reserve memory for the specified number of entries.
     *
     * @param count Number of entries.
     * @return `true` on success, or `false` on a memory allocation error.
     */
    bool reserve(int count) {
        return entries_.reserve(count);
    }

    /**
     * Get the number of entries that can be stored without reallocating memory.
     *
     * @return Number of entries.
     */
    int capacity() const {
        return entries_.capacity();
    }

    /**
     * Reduce the capacity of the map to its actual size.
     *
     * @return `true` on success, or `false` on a memory allocation error.
     */
    bool trimToSize() {
        return entries_.trimToSize();
    }

    /**
     * Remove all entries.
     */
    void clear() {
        entries_.clear();
    }

    ///@{
    /**
     * Get an iterator pointing to the first entry of the map.
     *
     * @return Iterator.
     */
    Iterator begin() {
        return entries_.begin();
    }

    ConstIterator begin() const {
        return entries_.begin();
    }
    ///@}

    ///@{
    /**
     * Get an iterator pointing to the entry following the last entry of the map.
     *
     * @return Iterator.
     */
    Iterator end() {
        return entries_.end();
    }

    ConstIterator end() const {
        return entries_.end();
    }
    ///@}

    ///@{
    /**
     * Find an entry.
     *
     * If an entry with the given key cannot be found, an iterator pointing to the entry following
     * the last entry of the map is returned.
     *
     * @param key Key.
     * @return Iterator pointing to the entry.
     */
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
    ///@}

    ///@{
    /**
     * Add or update an entry.
     *
     * On a memory allocation error, an iterator pointing to the entry following the last entry of
     * the map is returned.
     *
     * @param key Key.
     * @param val Value.
     * @return `std::pair` where `first` is an iterator pointing to the entry, and `second` is set
     *         to `true` if the entry was inserted, or `false` if it was updated.
     */
    template<typename T>
    std::pair<Iterator, bool> insert(const T& key, ValueT val) {
        auto it = lowerBound(key);
        if (it != entries_.end() && !cmp_(key, it->first)) {
            it->second = std::move(val);
            return std::make_pair(it, false);
        }
        it = entries_.insert(it, std::make_pair(KeyT(key), std::move(val)));
        if (it == entries_.end()) {
            return std::make_pair(it, false);
        }
        return std::make_pair(it, true);
    }

    std::pair<Iterator, bool> insert(KeyT&& key, ValueT val) {
        auto it = lowerBound(key);
        if (it != entries_.end() && !cmp_(key, it->first)) {
            it->second = std::move(val);
            return std::make_pair(it, false);
        }
        it = entries_.insert(it, std::make_pair(std::move(key), std::move(val)));
        if (it == entries_.end()) {
            return std::make_pair(it, false);
        }
        return std::make_pair(it, true);
    }
    ///@}

    /**
     * Remove an entry.
     *
     * @param pos Iterator pointing to the entry to be removed.
     * @return Iterator pointing to the entry following the removed entry.
     */
    Iterator erase(ConstIterator pos) {
        return entries_.erase(pos);
    }

    ///@{
    /**
     * Get an iterator pointing to first entry of the map whose key compares not less than the
     * provided key.
     *
     * @param key Key.
     * @return Iterator.
     */
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
    ///@}

    ///@{
    /**
     * Get an iterator pointing to first entry of the map whose key compares greater than the
     * provided key.
     *
     * @param key Key.
     * @return Iterator.
     */
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
    ///@}

    ///@{
    /**
     * Get a reference to the value of an entry.
     *
     * The entry is created if it doesn't exist.
     *
     * @note The device will panic if it fails to allocate memory for the new entry. Use `set()` or
     * `insert()` if you need more control over how memory allocation errors are handled.
     *
     * @param key Key.
     * @return Value.
     */
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
            it = entries_.insert(it, std::make_pair(std::move(key), ValueT()));
            SPARK_ASSERT(it != entries_.end());
        }
        return it->second;
    }
    ///@}

    /**
     * Assignment operator.
     *
     * @param map Map to assign from.
     * @return This map.
     */
    Map& operator=(Map map) {
        swap(*this, map);
        return *this;
    }

    /**
     * Comparison operators.
     *
     * Two maps are equal if they contain equal sets of entries.
     */
    ///@{
    bool operator==(const Map& map) const {
        return entries_ == map.entries_;
    }

    bool operator!=(const Map& map) const {
        return entries_ != map.entries_;
    }
    ///@}

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
