/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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
#include <cmath>

namespace {

template <typename T>
struct FormatTest {
    const char* fmt;
    T value;
    const char* expected;
};

} // anonymous

test(STRFORMAT_01_PRINTF_SCANF_INT_UINT) {
    constexpr FormatTest<uint64_t> u64Test[] = {
        {"%llu", UINT64_MAX, "18446744073709551615"},
        {"%llx", UINT64_MAX, "ffffffffffffffff"},
        {"%llX", UINT64_MAX, "FFFFFFFFFFFFFFFF"},
        {"%" PRIu64, UINT64_MAX, "18446744073709551615"},
        {"%" PRIx64, UINT64_MAX, "ffffffffffffffff"},
        {"%" PRIX64, UINT64_MAX, "FFFFFFFFFFFFFFFF"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<uint32_t> u32Test[] = {
        {"%lu", UINT32_MAX, "4294967295"},
        {"%lx", UINT32_MAX, "ffffffff"},
        {"%lX", UINT32_MAX, "FFFFFFFF"},
        {"%u", UINT32_MAX, "4294967295"},
        {"%x", UINT32_MAX, "ffffffff"},
        {"%X", UINT32_MAX, "FFFFFFFF"},
        {"%" PRIu32, UINT32_MAX, "4294967295"},
        {"%" PRIx32, UINT32_MAX, "ffffffff"},
        {"%" PRIX32, UINT32_MAX, "FFFFFFFF"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<uint16_t> u16Test[] = {
        {"%hu", UINT16_MAX, "65535"},
        {"%hx", UINT16_MAX, "ffff"},
        {"%hX", UINT16_MAX, "FFFF"},
        {"%" PRIu16, UINT16_MAX, "65535"},
        {"%" PRIx16, UINT16_MAX, "ffff"},
        {"%" PRIX16, UINT16_MAX, "FFFF"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<int64_t> i64Test[] = {
        {"%lld", INT64_MAX, "9223372036854775807"},
        {"%lld", INT64_MIN, "-9223372036854775808"},
        {"%" PRId64, INT64_MAX, "9223372036854775807"},
        {"%" PRId64, INT64_MIN, "-9223372036854775808"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<int32_t> i32Test[] = {
        {"%ld", INT32_MAX, "2147483647"},
        {"%ld", INT32_MIN, "-2147483648"},
        {"%d", INT32_MAX, "2147483647"},
        {"%d", INT32_MIN, "-2147483648"},
        {"%" PRId32, INT32_MAX, "2147483647"},
        {"%" PRId32, INT32_MIN, "-2147483648"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<int16_t> i16Test[] = {
        {"%hd", INT16_MAX, "32767"},
        {"%hd", INT16_MIN, "-32768"},
        {"%" PRId16, INT16_MAX, "32767"},
        {"%" PRId16, INT16_MIN, "-32768"},
        {nullptr, 0, nullptr}
    };

    constexpr FormatTest<double> dTest[] = {
        {"%4.2f", M_PI, "3.14"},
        {"%+.0e", M_PI, "+3e+00"},
        {"%E", M_PI, "3.141593E+00"},
        {"%g", M_PI, "3.14159"},
        {"%G", M_PI, "3.14159"},
        {nullptr, 0, nullptr}
    };

    const auto runTest = [&](const auto& test) -> void {
        for (int i = 0; test[i].fmt != nullptr; i++) {
            char tmp[256] = {};
            snprintf(tmp, sizeof(tmp), test[i].fmt, test[i].value);
            decltype(test[i].value) parsed = 0;
            const char* fmt = test[i].fmt;
            bool fp = false;
            if (std::is_floating_point<decltype(parsed)>::value) {
                fmt = "%lf";
                fp = true;
            }
            int scanfRes = sscanf(tmp, fmt, &parsed);
            assertEqual(String(tmp), String(test[i].expected));
            assertEqual(scanfRes, 1);
            if (!fp) {
                assertEqual(parsed, test[i].value);
            } else {
                memset(tmp, 0, sizeof(tmp));
                snprintf(tmp, sizeof(tmp), test[i].fmt, parsed);
                assertEqual(String(tmp), String(test[i].expected));
            }
        }
    };
    auto tuple = std::make_tuple(u64Test, u32Test, u16Test, i64Test, i32Test, i16Test, dTest);
    std::apply([runTest](auto&&... args) {
        ((runTest(args)), ...);
    }, tuple);
}
