/**
 ******************************************************************************
 * @file    spark_wiring_print.h
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_print.c module
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

#ifndef __SPARK_WIRING_PRINT_
#define __SPARK_WIRING_PRINT_

#include <type_traits>
#include <stddef.h>
#include <string.h>
#include <stdint.h> // for uint8_t
#include "system_tick_hal.h"

#include "spark_wiring_string.h"
#include "spark_wiring_printable.h"
#include "spark_wiring_fixed_point.h"
#include <climits>

const unsigned char DEC = 10;
const unsigned char HEX = 16;
const unsigned char OCT = 8;
const unsigned char BIN = 2;

class String;
class __FlashStringHelper;

class Print
{
  private:
    int write_error;

    template <typename T>
    struct PrintNumberTypeSelector {
    using type = typename std::conditional<
        particle::bits_fit_in_type<false, sizeof(T) * CHAR_BIT, unsigned long>::value, unsigned long, unsigned long long>::type;
    };

    size_t printNumber(unsigned long, uint8_t);
    size_t printNumber(unsigned long long, uint8_t);
    size_t printFloat(double, uint8_t);
  protected:
    void setWriteError(int err = 1) { write_error = err; }

  public:
    Print() : write_error(0) {}
    virtual ~Print() {}

    int getWriteError() { return write_error; }
    void clearWriteError() { setWriteError(0); }

    virtual size_t write(uint8_t) = 0;
    size_t write(const char *str) {
      if (str == NULL) return 0;
      return write((const uint8_t *)str, strlen(str));
    }
    virtual size_t write(const uint8_t *buffer, size_t size);

    size_t print(const char[]);
    size_t print(char);
    template <typename T, std::enable_if_t<!std::is_base_of<Printable, T>::value && (std::is_integral<T>::value || std::is_convertible<T, unsigned long long>::value ||
        std::is_convertible<T, long long>::value), int> = 0>
    size_t print(T, int = DEC);
    size_t print(float, int = 2);
    size_t print(double, int = 2);
    size_t print(const Printable&);
    size_t print(const __FlashStringHelper*);

    size_t println(const char[]);
    size_t println(char);
    template <typename T, std::enable_if_t<!std::is_base_of<Printable, T>::value && (std::is_integral<T>::value || std::is_convertible<T, unsigned long long>::value ||
        std::is_convertible<T, long long>::value), int> = 0>
    size_t println(T b, int base = DEC) {
        size_t n = print(b, base);
        n += println();
        return n;
    }
    size_t println(float, int = 2);
    size_t println(double, int = 2);
    size_t println(const Printable&);
    size_t println(void);
    size_t println(const __FlashStringHelper*);

    size_t printf(const char* format, ...) __attribute__ ((format(printf, 2, 3)))
    {
        va_list args;
        va_start(args, format);
        auto r = this->vprintf(false, format, args);
        va_end(args);
        return r;
    }

    size_t printlnf(const char* format, ...) __attribute__ ((format(printf, 2, 3)))
    {
        va_list args;
        va_start(args, format);
        auto r = this->vprintf(true, format, args);
        va_end(args);
        return r;
    }

    size_t vprintf(bool newline, const char* format, va_list args) __attribute__ ((format(printf, 3, 0)));
};

template <typename T, std::enable_if_t<!std::is_base_of<Printable, T>::value && (std::is_integral<T>::value || std::is_convertible<T, unsigned long long>::value ||
    std::is_convertible<T, long long>::value), int>>
size_t Print::print(T n, int base)
{
    if (base == 0) {
        return write(n);
    } else {
        size_t t = 0;
        using PrintNumberType = typename PrintNumberTypeSelector<T>::type;
        PrintNumberType val;
// FIXME: avoids 'comparison of constant '0' with boolean expression is always false'
#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-compare"
#endif // __GNUC__ >= 9
        if (n < 0 && base == 10) {
            t = print('-');
            val = -n;
        } else {
            val = n;
        }
#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif // __GNUC__ >= 9
        return printNumber(val, base) + t;
    }
}
#endif
