/**
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#pragma once

#include <memory>

/* Implements a queue with a fixed number of elements.
 * The buffer is allocated on the heap at construction time
 * or when calling reallocate().
 * When the queue is full, additional elements are discarded.
 *
 * WARNING: Only use this container for Plain Old Data (value objects
 * that don't manage memory/resources) since it builds an array of blank
 * objects to hold the capacity of the queue and doesn't destroy the
 * objects that are popped.
 */

template <typename T>
class FixedQueue {
  protected:

  typedef T ValueType;
  typedef std::unique_ptr<ValueType []> ArrayType;
  typedef std::size_t SizeType;

  public:

  FixedQueue()
    : _capacity(0),
      _size(0),
      _head(0),
      _tail(0),
      _data(nullptr)
  {
  }

  FixedQueue(SizeType capacity)
    : FixedQueue {}
  {
    reallocate(capacity);
  }

  void reallocate(SizeType capacity)
  {
    _capacity = capacity;
    _data = ArrayType { new ValueType[_capacity] };
    clear();
  }

  void clear() {
    _size = _head = _tail = 0;
  }

  SizeType size() const {
    return _size;
  }

  SizeType capacity() const {
    return _capacity;
  }

  bool empty() const {
    return size() == 0;
  }

  bool full() const {
    return size() >= capacity();
  }

  bool push(const ValueType &value) {
    if(full()) {
      return false;
    }

    _data[_tail] = value;
    _tail = (_tail + 1) % _capacity;
    ++_size;
    return true;
  }

  ValueType pop() {
    ValueType value = {};
    if(!empty()) {
      value = _data[_head];
      _head = (_head + 1) % _capacity;
      --_size;
    }

    return value;
  }

  protected:
  SizeType _capacity;
  SizeType _size;
  SizeType _head;
  SizeType _tail;
  ArrayType _data;
};
