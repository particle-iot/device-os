/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "logging.h"

/**
 * A simple append-only list. The elements are never deallocated.
 */
// todo - perhaps replace this with our vector implementation?
template <typename T> class append_list
{
    uint8_t count;
    uint8_t capacity;
    uint8_t block_size;
    T* store;

    bool expand(unsigned capacity) {
        if (capacity>255)
            return false;

        T* new_store = (T*)realloc(store, sizeof(T)*capacity);
        if (new_store) {
            store = new_store;
            this->capacity = capacity;
        }
        return new_store!=NULL;
    }

public:

    append_list(unsigned block=5) : count(0), capacity(0), block_size(block), store(NULL) {}

    T* add() {
    	return add(T());
    }

    T* add(const T& item) {
        bool space = (count<capacity || expand(count+block_size));
        T* result = nullptr;
        if (space) {
        	result = store + count;
            store[count++] = item;
        }
        return result;
    }

    bool removeAt(unsigned int i) {
    	if (i<count) {
			T* const p = store + i;
			memmove(p, p + 1, count - i - 1);
			count--;
    	}
        return true;
    }

    T& operator[](unsigned index) { return store[index]; }
    unsigned size() { return count; }
};


