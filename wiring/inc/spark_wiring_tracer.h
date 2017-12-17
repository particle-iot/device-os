/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#ifndef SPARK_WIRING_TRACER_H
#define SPARK_WIRING_TRACER_H

#include "tracer_service.h"

#define _TRACER_VA_NARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define _TRACER_VA_NARGS(...) _TRACER_VA_NARGS_IMPL(X,##__VA_ARGS__, 4, 3, 2, 1, 0)
#define _TRACER_VARARG_IMPL2(base, count, ...) base##count(__VA_ARGS__)
#define _TRACER_VARARG_IMPL(base, count, ...) _TRACER_VARARG_IMPL2(base, count, __VA_ARGS__)
#define _TRACER_VARARG(base, ...) _TRACER_VARARG_IMPL(base, _TRACER_VA_NARGS(__VA_ARGS__), __VA_ARGS__)

#if defined(TRACER_ELF_AVAILABLE) && TRACER_ELF_AVAILABLE == 1
# define CHECKPOINT0() TRACER_CHECKPOINT()
# define _LOG_CHECKPOINT() TRACER_INSTRUCTION_CHECKPOINT(platform_get_return_address(0))
#else
# define CHECKPOINT0() TRACER_TEXT_CHECKPOINT()
# define _LOG_CHECKPOINT()
#endif /* defined(TRACER_ELF_AVAILABLE) && TRACER_ELF_AVAILABLE == 1 */

#define CHECKPOINT1(txt) TRACER_TEXT_CHECKPOINT_C(txt)
#define CHECKPOINT(...) _TRACER_VARARG(CHECKPOINT, __VA_ARGS__)

#endif /* SPARK_WIRING_TRACER_H */
