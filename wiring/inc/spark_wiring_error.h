/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#ifndef SPARK_WIRING_ERROR_H
#define SPARK_WIRING_ERROR_H

#include "system_error.h"

namespace spark {

// TODO: Add support for custom error messages
class Error {
public:
    enum Type {
        // NONE = 0,
        // UNKNOWN = 100,
        // ...
        SYSTEM_ERROR_ENUM_VALUES()
    };

    Error();
    Error(Type type);

    Type type() const;
    const char* message() const;

private:
    Type type_;
};

} // namespace spark

// spark::Error
inline spark::Error::Error() :
        type_(NONE) {
}

inline spark::Error::Error(Type type) :
        type_(type) {
}

inline spark::Error::Type spark::Error::type() const {
    return type_;
}

inline const char* spark::Error::message() const {
    return system_error_message((system_error)type_, nullptr);
}

#endif // SPARK_WIRING_ERROR_H
