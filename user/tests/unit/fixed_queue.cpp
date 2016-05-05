/**
 ******************************************************************************
 * @file    fixed_queue.cpp
 * @authors Julien Vanier
 * @date    7 January 2016
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

#include "catch.hpp"
#include "fixed_queue.h"
#include <string>

SCENARIO("Queue is empty after creation", "[fixed_queue]") {
  FixedQueue<char> q(10);
  CHECK(q.empty());
}

SCENARIO("Queue reports its capacity", "[fixed_queue]") {
  FixedQueue<char> q(10);
  CHECK(q.capacity() == 10);
}

SCENARIO("Clear empties the queue", "[fixed_queue]") {
  FixedQueue<char> q(2);
  q.push(5);
  q.clear();
  CHECK(q.size() == 0);
}

SCENARIO("Pop returns the pushed value", "[fixed_queue]") {
  FixedQueue<char> q(10);
  q.push(5);
  CHECK(q.pop() == 5);
}

SCENARIO("Push when full returns false", "[fixed_queue]") {
  FixedQueue<char> q(2);
  CHECK(q.push(1) == true);
  CHECK(q.push(2) == true);
  CHECK(q.push(3) == false);
}

SCENARIO("Queue rotates through buffer", "[fixed_queue]") {
  FixedQueue<char> q(2);
  for(int i = 0; i < 5; i++) {
    CHECK(q.push(i) == true);
    CHECK(q.pop() == i);
  }
}

struct Item {
  char buffer[3];
  int size;
};

SCENARIO("Queue copies all data for structs", "[fixed_queue]") {
  FixedQueue<Item> q(2);
  q.push(Item{ {1, 2}, 2});
  Item item = q.pop();
  CHECK(item.buffer[0] == 1);
  CHECK(item.buffer[1] == 2);
  CHECK(item.size == 2);
}
