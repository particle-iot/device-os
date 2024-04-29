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

#include "spark_wiring_stream.h"
#include "spark_wiring_cbor.h"

namespace particle {

class Event:
        public Stream,
        public CBORReader,
        public CBORWriter {
public:
    enum ContentType {
        AUTO,
        TEXT,
        BINARY,
        CBOR
    };

    enum Direction {
        IN,
        OUT
    };

    typedef void (*OnSent)(int error, Event event);

    Event();
    ~Event();

    const char* name() const;

    Event& contentType(ContentType type);
    ContentType contentType() const;

    Event& timestamp(uint64_t time);
    uint64_t timestamp() const;

    Direction direction() const;
    bool isWritable() const;

    Event& onSent(OnSent callback);

    int end();
    int cancel();
    int rewind();

    bool ok() const;
    int error() const;

    // Convenience overloads for CBORReader methods
    Variant nextValue();
    Variant nextKey();
    Variant::Type nextType();
    bool hasNext();

    // Convenience overloads for CBORWriter methods
    Event& value(const Variant& val);
    Event& key(const Variant& val);
    Event& beginArray(int count = -1);
    Event& endArray();
    Event& beginMap(int count = -1);
    Event& endMap();

private:
    // ...
};

} // namespace particle
