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

/**
 * @file check.h
 *
 * @section check_macros Check macros
 *
 * This header file defines a set of macros that simplify error handling in the Device OS code.
 *
 * Device OS doesn't use C++ exceptions, and, by convention, its internal and public API functions
 * return a negative value in case of an error. All error codes are defined in the system_error.h
 * file.
 *
 * Consider the following example:
 * ```
 * int foo() {
 *     void* buffer = malloc(1024);
 *     if (!buffer) {
 *         return SYSTEM_ERROR_NO_MEMORY; // -260
 *     }
 *     ...
 *     free(buffer);
 *     return 0; // No error
 * }
 * ```
 * When invoking such a function, the calling function should properly handle all possible errors.
 * In the most typical scenario, it simply forwards the error to its own calling function:
 * ```
 * int fooBarBaz() {
 *     int result = foo();
 *     if (result == 0) {
 *         result = bar();
 *         if (result == 0) {
 *             result = baz();
 *         }
 *     }
 *     return result;
 * }
 * ```
 * When many functions are involved, writing the code like in the above example becomes tedious.
 * Given that all the functions follow the convention, the code can be rewritten as follows using
 * the `CHECK()` macro:
 * ```
 * int fooBarBaz() {
 *     // This macro makes fooBarBaz() return SYSTEM_ERROR_NO_MEMORY if foo() fails to allocate its
 *     // internal buffer
 *     CHECK(foo());
 *     CHECK(bar());
 *     CHECK(baz());
 *     return 0;
 * }
 * ```
 * The `CHECK()` macro expands to a code that evaluates the expression passed to the macro. If the
 * expression yields a negative value then the function that invokes the macro (`fooBarBaz()` in the
 * above example) will return that value and thus finish its execution with an error. Non-negative
 * values get passed back to the calling code as if the checked expression was evaluated directly:
 * ```
 * int parseStream(Stream* stream) {
 *     char buffer[128];
 *     size_t size = CHECK(stream->read(buffer, sizeof(buffer));
 *     LOG(TRACE, "Read %u bytes", (unsigned)size);
 *     ...
 *     return 0;
 * }
 * ```
 * In the above example, it is safe to assume that the value returned by `Stream::read()`, which is
 * wrapped into the `CHECK()` macro, is greater than or equal to 0, since, otherwise, `parseStream()`
 * would have returned before the initialization of the `size` variable. Internally, this is
 * implemented using GCC's statement expressions:
 *
 * https://gcc.gnu.org/onlinedocs/gcc-3.2.3/gcc/Statement-Exprs.html
 *
 * It is important to note that a Device OS function should never return a negative result code
 * from a third party library as is, otherwise, it will be impossible to decode that error in the
 * dependent code. The following code is incorrect:
 * ```
 * int receiveMessage(int socket) {
 *     char buffer[128];
 *     // It is an error to use non-Device OS functions with CHECK()
 *     size_t size = CHECK(lwip_read(socket, buffer, sizeof(buffer)));
 *     LOG(TRACE, "Read %u bytes", (unsigned)size);
 *     ...
 *     return 0;
 * }
 * ```
 * This header file also defines the `CHECK_TRUE()` and `CHECK_FALSE()` macros, which can be used
 * with predicate expressions:
 * ```
 * int foo() {
 *     std::unique_ptr<char[]> buffer(new(std::nothrow) char[1024]);
 *     CHECK_TRUE(buffer, SYSTEM_ERROR_NO_MEMORY);
 *     ...
 *     return 0;
 * }
 *
 * int bar(int value) {
 *     static Vector<int> values;
 *     CHECK_FALSE(values.contains(value), SYSTEM_ERROR_ALREADY_EXISTS);
 *     ...
 *     return 0;
 * }
 * ```
 */

#pragma once

#include "system_error.h"
#include "logging.h"

#ifndef LOG_CHECKED_ERRORS
#define LOG_CHECKED_ERRORS 0
#endif

#if LOG_CHECKED_ERRORS
#define _LOG_CHECKED_ERROR(_expr, _ret) \
        LOG(ERROR, #_expr " failed: %d", (int)_ret)
#else
#define _LOG_CHECKED_ERROR(_expr, _ret)
#endif

/**
 * Check the result of an expression.
 *
 * @see @ref check_macros
 */
#define CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _ret; \
            } \
            _ret; \
        })

/**
 * Check the result of a predicate expression.
 *
 * @see @ref check_macros
 */
#define CHECK_TRUE(_expr, _ret) \
        do { \
            const bool _ok = (bool)(_expr); \
            if (!_ok) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _ret; \
            } \
        } while (false)

/**
 * Check the result of a predicate expression.
 *
 * @see @ref check_macros
 */
#define CHECK_FALSE(_expr, _ret) \
        CHECK_TRUE(!(_expr), _ret)

// Deprecated
#define CHECK_RETURN(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _val; \
            } \
            _ret; \
        })

// Deprecated
#define CHECK_TRUE_RETURN(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            const bool _ok = (bool)(_ret); \
            if (!_ok) { \
                _LOG_CHECKED_ERROR(_expr, _val); \
                return _val; \
            } \
            _ret; \
        })
