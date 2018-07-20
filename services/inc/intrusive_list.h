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

#ifndef SERVICES_INTRUSIVE_LIST_H
#define SERVICES_INTRUSIVE_LIST_H

namespace particle {

// Template class implementing an intrusive list container
template<typename ItemT>
class IntrusiveList {
public:
    typedef ItemT ItemType;

    IntrusiveList() :
            front_(nullptr) {
    }

    ~IntrusiveList() {

    }

    void pushFront(ItemT* item) {
        item->next = front_;
        front_ = item;
    }

    ItemT* popFront() {
        if (!front_) {
            return nullptr;
        }
        const auto item = front_;
        front_ = static_cast<ItemT*>(front_->next);
        return item;
    }

    ItemT* pop(ItemT* item, ItemT* prev = nullptr) {
        if (!prev) {
            for (ItemT* i = front_, *p = static_cast<ItemT*>(nullptr); i != nullptr; p = i, i = i->next) {
                if (i == item) {
                    prev = p;
                    break;
                }
            }
        }

        if (prev) {
            prev->next = item->next;
            return item;
        } else if (item == front()) {
            return popFront();
        }

        return nullptr;
    }

    ItemT* front() const {
        return front_;
    }

private:
    ItemT* front_;
};

} // particle

#endif /* SERVICES_INTRUSIVE_LIST_H */
