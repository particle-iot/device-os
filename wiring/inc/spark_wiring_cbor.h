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

class Print;
class Stream;
class Variant;

namespace particle {

class CBORReader {
public:
    explicit CBORReader(Stream& stream);

    int readType(Variant::Type& type);
    int readValue(Variant& val);
    int skipValue();

    int enterArray();
    int leaveArray();

    int enterMap();
    int leaveMap();

    int count() const;

    bool hasMore();

    size_t bytesRead() const;

private:
    // ...
};

class CBORWriter {
public:
    explicit CBORWriter(Print& stream);

    int writeValue(const Variant& val);
    int writeKey(const Variant& val);

    int beginArray(int count = -1);
    int endArray();

    int beginMap(int count = -1);
    int endMap();

    size_t bytesWritten() const;

private:
    // ...
};

} // namespace particle
