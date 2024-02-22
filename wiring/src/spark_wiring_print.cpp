/**
 ******************************************************************************
 * @file    spark_wiring_print.cpp
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring print
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2010 David A. Mellis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include "spark_wiring_print.h"
#include "spark_wiring_json.h"
#include "spark_wiring_variant.h"
#include "spark_wiring_string.h"
#include "spark_wiring_error.h"

using namespace particle;

namespace {

void writeVariant(const Variant& var, JSONStreamWriter& writer) {
    switch (var.type()) {
    case Variant::NULL_: {
        writer.nullValue();
        break;
    }
    case Variant::BOOL: {
        writer.value(var.value<bool>());
        break;
    }
    case Variant::INT: {
        writer.value(var.value<int>());
        break;
    }
    case Variant::UINT: {
        writer.value(var.value<unsigned>());
        break;
    }
    case Variant::INT64: {
        writer.value(var.value<int64_t>());
        break;
    }
    case Variant::UINT64: {
        writer.value(var.value<uint64_t>());
        break;
    }
    case Variant::DOUBLE: {
        writer.value(var.value<double>());
        break;
    }
    case Variant::STRING: {
        writer.value(var.value<String>());
        break;
    }
    case Variant::ARRAY: {
        writer.beginArray();
        for (auto& v: var.value<VariantArray>()) {
            writeVariant(v, writer);
        }
        writer.endArray();
        break;
    }
    case Variant::MAP: {
        writer.beginObject();
        for (auto& e: var.value<VariantMap>().entries()) {
            writer.name(e.first);
            writeVariant(e.second, writer);
        }
        writer.endObject();
        break;
    }
    default:
        break;
    }
}

} // namespace

// Public Methods //////////////////////////////////////////////////////////////

/* default implementation: may be overridden */
size_t Print::write(const uint8_t *buffer, size_t size)
{
  size_t n = 0;
  while (size--) {
     int chunk = write(*buffer++);
     if (chunk>=0)
         n += chunk;
     else {
         if (n==0)
             n = chunk;
         break;
     }
  }
  return n;
}

size_t Print::print(const char str[])
{
  return write(str);
}

size_t Print::print(char c)
{
  return write(c);
}

 size_t Print::print(const Printable& x)
 {
   return x.printTo(*this);
 }

size_t Print::print(const __FlashStringHelper* str)
{
  return print(reinterpret_cast<const char*>(str));
}

size_t Print::println(void)
{
  size_t n = print('\r');
  n += print('\n');
  return n;
}

size_t Print::println(const char c[])
{
  size_t n = print(c);
  n += println();
  return n;
}

size_t Print::println(char c)
{
  size_t n = print(c);
  n += println();
  return n;
}

 size_t Print::println(const Printable& x)
 {
   size_t n = print(x);
   n += println();
   return n;
 }

size_t Print::println(const __FlashStringHelper* str)
{
  return println(reinterpret_cast<const char*>(str));
}

// Private Methods /////////////////////////////////////////////////////////////

size_t Print::printNumber(unsigned long n, uint8_t base) {
  char buf[8 * sizeof(n) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do {
   decltype(n) m = n;
   n /= base;
   char c = m - base * n;
   *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  return write(str);
}
 
 size_t Print::printNumber(unsigned long long n, uint8_t base) {
  char buf[8 * sizeof(n) + 1]; // Assumes 8-bit chars plus zero byte.

  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do {
    decltype(n) m = n;
    n /= base;
    char c = m - base * n;
    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  return write(str);
}

size_t Print::printVariant(const Variant& var) {
    JSONStreamWriter writer(*this);
    writeVariant(var, writer);
    return writer.bytesWritten();
}

size_t Print::vprintf(bool newline, const char* format, va_list args)
{
    const int bufsize = 20;
    char test[bufsize];
    va_list args2;
    va_copy(args2, args);
    size_t n = vsnprintf(test, bufsize, format, args);

    if (n<bufsize)
    {
        n = print(test);
    }
    else
    {
        char bigger[n+1];
        n = vsnprintf(bigger, n+1, format, args2);
        n = print(bigger);
    }
    if (newline)
        n += println();

    va_end(args2);
    return n;
}

namespace particle {

size_t OutputStringStream::write(const uint8_t* data, size_t size) {
    if (getWriteError()) {
        return 0;
    }
    size_t newSize = s_.length() + size;
    if (s_.capacity() < newSize && !s_.reserve(std::max<size_t>({ newSize, s_.capacity() * 3 / 2, 20 }))) {
        setWriteError(Error::NO_MEMORY);
        return 0;
    }
    s_.concat((const char*)data, size);
    return size;
}

} // namespace particle
