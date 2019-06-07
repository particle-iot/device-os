#pragma once

/**
  ******************************************************************************
  Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.

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

#include <cstddef>

/**
 * @param T1 the type of the member pointer
 * @param The member pointer (essentially an offset.)
 */
template <typename T1, typename T2>
constexpr auto offset_of(T1 T2::* member) {
    // under
    constexpr T2 object {};
    return size_t(&(object.*member)) - size_t(&object);
}

/**
 * Generic template for retrieving the size from the same named property.
 * Allows specialization for types that use a different name.
 */
template <typename T>
inline auto constexpr dynamic_size(const T& t) -> decltype(t.size) {
    return t.size;
}

/**
 * Determines if a dynamically sized structure contains the given member. This should be used
 * firmware receiving a struct that may vary in length depending upon the client invoking it.
 * The struct type is expected to have a `size` member of a scalar type, plus any additional members required.
 * The struct is considered to contain the member when the size is greater than or equal to the
 * offset of the member in the struct plus the member's size.
 *
 * @param value     The struct to test.  Must have a size field.
 * @param member    The pointer-to-member for the member of the struct to check.
 * @returns true if the structure size is sufficient to contain the given member, false otherwise.
 *
 * See wiring/no_fixture/dynalib_abi.cpp for tests.
 * Looking at the compiler output of ARM GCC 5.3.1,
 * the template is as efficient as a hand-crafted test.
 */
template <typename T1, typename T2>
inline bool struct_contains(const T2& value, T1 T2::* member) {
    const auto size_needed = offset_of(member)+sizeof(T1);
    return dynamic_size(value) >= size_needed;
}

template <typename T1, typename T2>
inline bool struct_contains(const T2* value, T1 T2::* member) {
    return struct_contains(*value, member);
}

