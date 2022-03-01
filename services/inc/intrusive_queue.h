/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_interrupts.h"

namespace particle {

// Template class implementing an intrusive queue container
template<typename ItemT>
class IntrusiveQueue {
public:
    typedef ItemT ItemType;

    IntrusiveQueue() :
            front_(nullptr),
            back_(nullptr) {
    }

    void pushBack(ItemT* item) {
        if (back_) {
            back_->next = item;
        } else { // The queue is empty
            front_ = item;
        }
        item->next = nullptr;
        back_ = item;
    }

    ItemT* popFront() {
        if (!front_) {
            return nullptr;
        }
        const auto item = front_;
        front_ = static_cast<ItemT*>(front_->next);
        if (!front_) {
            back_ = nullptr;
        }
        return item;
    }

    ItemT* front() const {
        return front_;
    }

    ItemT* back() const {
        return back_;
    }

private:
    ItemT* front_;
    ItemT* back_;
};

// TODO: Use a known good lockless queue implementation
template<typename ItemT>
class AtomicIntrusiveQueue {
public:
    typedef ItemT ItemType;

    AtomicIntrusiveQueue() :
            front_(nullptr),
            back_(nullptr) {
    }

    void pushBack(ItemT* item) {
        ATOMIC_BLOCK() {
            if (back_) {
                back_->next = item;
            } else { // The queue is empty
                front_ = item;
            }
            item->next = nullptr;
            back_ = item;
        }
    }

    ItemT* popFront() {
        if (!front_) {
            return nullptr;
        }
        ItemT* item = nullptr;
        ATOMIC_BLOCK() {
            item = front_;
            front_ = static_cast<ItemT*>(item->next);
            if (!front_) {
                back_ = nullptr;
            }
        }
        return item;
    }

    ItemT* front() const {
        return front_;
    }

    ItemT* back() const {
        return back_;
    }

private:
    ItemT* volatile front_;
    ItemT* volatile back_;
};

} // particle
