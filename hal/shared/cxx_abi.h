/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#ifdef __cplusplus
#include <cxxabi.h>
extern "C" {
using __cxxabiv1::__guard;
#else
typedef void __guard;
#endif // __cplusplus   

int __cxa_guard_acquire(__guard* g);
void __cxa_guard_release(__guard* g);
void __cxa_guard_abort(__guard* g);

#ifdef __cplusplus
}
#endif // __cplusplus
