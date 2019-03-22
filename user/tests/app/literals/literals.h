/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include <chrono>
#include <ratio>
#include "system_tick_hal.h"

namespace particle {

typedef std::ratio<60> minutes;
typedef std::ratio<3600> hours;
typedef std::ratio<86400> days;
typedef std::ratio<604800> weeks;
typedef std::ratio<2629746> months;
typedef std::ratio<31556952> years;

namespace chrono {

typedef std::chrono::duration<system_tick_t, std::micro> microseconds;
typedef std::chrono::duration<system_tick_t, std::milli> milliseconds;
typedef std::chrono::duration<system_tick_t> seconds;
typedef std::chrono::duration<system_tick_t, particle::minutes> minutes;
typedef std::chrono::duration<system_tick_t, particle::hours> hours;
typedef std::chrono::duration<system_tick_t, particle::days> days;
typedef std::chrono::duration<system_tick_t, particle::weeks> weeks;
typedef std::chrono::duration<system_tick_t, particle::months> months;
typedef std::chrono::duration<system_tick_t, particle::years> years;

} // namespace chrono

namespace literals {
namespace chrono_literals {

constexpr particle::chrono::microseconds operator "" _us(unsigned long long us)
{
    return particle::chrono::microseconds(us);
}

constexpr particle::chrono::milliseconds operator "" _ms(unsigned long long ms)
{
    return particle::chrono::milliseconds(ms);
}

constexpr particle::chrono::seconds operator "" _s(unsigned long long s)
{
    return particle::chrono::seconds(s);
}

constexpr particle::chrono::minutes operator "" _min(unsigned long long min)
{
    return particle::chrono::minutes(min);
}

constexpr particle::chrono::hours operator "" _h(unsigned long long h)
{
    return particle::chrono::hours(h);
}

} // namespace chrono_literals
} // namespace literals

} // namespace particle
