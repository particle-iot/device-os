/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include <random>
#include <string>

namespace particle {

namespace test {

std::string randString(size_t size);
std::default_random_engine& randGen();

template<typename T>
T randNumber() {
    std::uniform_int_distribution<T> dist;
    return dist(randGen());
}

template<typename T>
T randNumber(T ref) {
    std::uniform_int_distribution<T> dist;
    return dist(randGen());
}

template<typename T>
void setRandInt(T& v) {
    std::uniform_int_distribution<T> dist;
    v = dist(randGen());
}

} // namespace test

} // namespace particle
