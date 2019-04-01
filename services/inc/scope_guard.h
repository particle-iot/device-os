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
 * @file scope_guard.h
 *
 * @section scope_guards Scope guards
 *
 * This header file defines a set of macros that simplify RAII-style resource management in the
 * Device OS code.
 *
 * Consider the following example:
 * ```
 * int parseStream(Stream* stream) {
 *     // Allocate a buffer
 *     const size_t bufferSize = 1024;
 *     char* buffer = (char*)malloc(bufferSize);
 *     if (!buffer) {
 *         return SYSTEM_ERROR_NO_MEMORY;
 *     }
 *     // Read the first chunk of the data
 *     int result = stream->read(buffer, bufferSize);
 *     if (result < 0) {
 *         free(buffer);
 *         return result;
 *     }
 *     ...
 *     // Read the second chunk of the data
 *     result = stream->read(buffer, bufferSize);
 *     if (result < 0) {
 *         free(buffer);
 *         return result;
 *     }
 *     ...
 *     // Free the buffer
 *     free(buffer);
 *     return 0;
 * }
 * ```
 * Using the `SCOPE_GUARD()` macro together with the @ref check_macros "check macros", the above
 * code can be rewritten in a less error-prone way:
 * ```
 * int parseStream(Stream* stream) {
 *     // Allocate a buffer
 *     const size_t bufferSize = 1024;
 *     char* buffer = (char*)malloc(bufferSize);
 *     CHECK_TRUE(buffer, SYSTEM_ERROR_NO_MEMORY);
 *     // Declare a scope guard
 *     SCOPE_GUARD({
 *         free(buffer);
 *     });
 *     // Read the first chunk of the data
 *     CHECK(stream->read(buffer, bufferSize));
 *     ...
 *     // Read the second chunk of the data
 *     CHECK(stream->read(buffer, bufferSize));
 *     ...
 *     return 0;
 * }
 * ```
 * Internally, `SCOPE_GUARD()` declares an object that schedules an arbitrary user-provided code
 * for execution at the end of the enclosing scope of the object. In the above example, the buffer
 * allocated via `malloc()` will be automatically freed by the scope guard when the function returns
 * either successfully or with an error.
 *
 * It is possible to assign a name to a scope guard object by using the `NAMED_SCOPE_GUARD()` macro.
 * Named scope guards are useful when the ownership over a managed resource needs to be transferred
 * to another function:
 * ```
 * int openResourceFile(lfs_t *lfs, lfs_file_t* file, const char* name) {
 *     // Open the resource file
 *     auto result = lfs_file_open(lfs, file, name, LFS_O_RDONLY);
 *     if (result < 0) {
 *         LOG(ERROR, "Unable to open resource file");
 *         return SYSTEM_ERROR_FILE;
 *     }
 *     // Declare a scope guard
 *     NAMED_SCOPE_GUARD(guard, {
 *         lfs_file_close(lfs, file);
 *     });
 *     // Check the header signature
 *     const headerSize = 3;
 *     char header[headerSize + 1] = {}; // Reserve one character for the term. null
 *     result = lfs_file_read(lfs, file, header, headerSize);
 *     if (result < 0) {
 *         LOG(ERROR, "Unable to read resource file");
 *         return SYSTEM_ERROR_IO;
 *     }
 *     if (result != headerSize || strcmp(header, "RES") != 0) {
 *         LOG(ERROR, "Invalid header signature");
 *         return SYSTEM_ERROR_BAD_DATA;
 *     }
 *     ...
 *     // Dismiss the scope guard and let the calling code read the remaining sections of the file
 *     // and manage the lifetime of the file descriptor
 *     guard.dismiss();
 *     return 0;
 * }
 * ```
 */

#pragma once

#include "preprocessor.h"

#include <utility>

/**
 * Declare a scope guard.
 *
 * @see @ref scope_guards
 */
// TODO: Add a separate class for unnamed scope guards
#define SCOPE_GUARD(_func) \
        NAMED_SCOPE_GUARD(PP_CAT(_scope_guard_, __COUNTER__), _func)

/**
 * Declare a named scope guard.
 *
 * @see @ref scope_guards
 */
#define NAMED_SCOPE_GUARD(_name, _func) \
        auto _name = ::particle::makeNamedScopeGuard([&] _func)

namespace particle {

template<typename FuncT>
class NamedScopeGuard {
public:
    NamedScopeGuard(FuncT&& func) :
            func_(std::move(func)),
            dismissed_(false) {
    }

    NamedScopeGuard(NamedScopeGuard&& guard) :
            func_(std::move(guard.func_)),
            dismissed_(guard.dismissed_) {
        guard.dismissed_ = true;
    }

    ~NamedScopeGuard() {
        if (!dismissed_) {
            func_();
        }
    }

    void dismiss() {
        dismissed_ = true;
    }

    NamedScopeGuard(const NamedScopeGuard&) = delete;
    NamedScopeGuard& operator=(const NamedScopeGuard&) = delete;

private:
    const FuncT func_;
    bool dismissed_;
};

template<typename FuncT>
inline NamedScopeGuard<FuncT> makeNamedScopeGuard(FuncT&& func) {
    return NamedScopeGuard<FuncT>(std::move(func));
}

} // particle
