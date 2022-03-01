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

#pragma once

#include <cstddef>

namespace particle {

class InputStream;

int readLine(InputStream* strm, char* data, size_t size, unsigned timeout = 0);

int skipAll(InputStream* strm, unsigned timeout = 0);
int skipNonPrintable(InputStream* strm, unsigned timeout = 0);
int skipWhitespace(InputStream* strm, unsigned timeout = 0);

} // particle
