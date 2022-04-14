/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"

test(JSON_01_can_parse_more_than_127_tokens) {
    const size_t bufferSize = 2048;
    const size_t num = 256;
    auto buf = std::make_unique<char[]>(bufferSize);
    assertTrue((bool)buf);
    memset(buf.get(), 0, bufferSize);
    JSONBufferWriter w(buf.get(), bufferSize - 1);
    w.beginArray();
    Random rand;
    char c;
    for (unsigned i = 0; i < num; i++) {
        rand.genBase32(&c, sizeof(c));
        w.value(&c, sizeof(c));
    }
    w.endArray();
    assertMore(w.dataSize(), bufferSize / 2);

    auto d = JSONValue::parse(buf.get(), w.dataSize());
    assertTrue(d.isArray());
    JSONArrayIterator iter(d);
    assertEqual(iter.count(), num);
}
