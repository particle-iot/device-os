/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#include <catch2/catch.hpp>

#include <charconv>
#include <cstring>
#include <climits>
#include <string>

/**
 * These tests validate the parsing/validation logic used by the env vars API.
 * The actual API functions (getEnv, envVar) require filesystem mocking for
 * full integration tests. These tests verify the validation rules in isolation.
 */

namespace {

// Helper functions that mirror the validation logic in env_vars.cpp and spark_wiring_system.cpp

/**
 * Validates a string as a boolean value.
 * Only "true" and "false" (case-sensitive, lowercase) are valid.
 *
 * @param str The string to validate
 * @param[out] val The parsed boolean value if valid
 * @return true if valid, false otherwise
 */
bool validateBool(const char* str, bool& val) {
    if (!str) {
        return false;
    }
    if (std::strcmp(str, "true") == 0) {
        val = true;
        return true;
    }
    if (std::strcmp(str, "false") == 0) {
        val = false;
        return true;
    }
    return false;
}

/**
 * Validates a string as a 32-bit signed integer.
 * Only decimal digits with optional leading minus sign are valid.
 * Overflow is detected and returns false.
 *
 * @param str The string to validate
 * @param len Length of the string
 * @param[out] val The parsed integer value if valid
 * @return true if valid, false otherwise
 */
bool validateInt(const char* str, size_t len, int& val) {
    if (!str || len == 0) {
        return false;
    }
    int parsed = 0;
    auto r = std::from_chars(str, str + len, parsed);
    if (r.ec != std::errc() || r.ptr != str + len) {
        return false;
    }
    val = parsed;
    return true;
}

/**
 * Validates a string value (non-empty check).
 *
 * @param str The string to validate
 * @return true if non-null and non-empty, false otherwise
 */
bool validateString(const char* str) {
    return str && str[0] != '\0';
}

} // anonymous namespace

TEST_CASE("Bool validation") {
    bool val = false;

    SECTION("valid lowercase 'true'") {
        CHECK(validateBool("true", val) == true);
        CHECK(val == true);
    }

    SECTION("valid lowercase 'false'") {
        val = true; // Set to opposite to verify it changes
        CHECK(validateBool("false", val) == true);
        CHECK(val == false);
    }

    SECTION("invalid - uppercase 'TRUE'") {
        val = false;
        CHECK(validateBool("TRUE", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - uppercase 'FALSE'") {
        val = true;
        CHECK(validateBool("FALSE", val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - mixed case 'True'") {
        val = false;
        CHECK(validateBool("True", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - mixed case 'False'") {
        val = true;
        CHECK(validateBool("False", val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - numeric '1'") {
        val = false;
        CHECK(validateBool("1", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - numeric '0'") {
        val = true;
        CHECK(validateBool("0", val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - 'yes'") {
        val = false;
        CHECK(validateBool("yes", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - 'no'") {
        val = true;
        CHECK(validateBool("no", val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - empty string") {
        val = true;
        CHECK(validateBool("", val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - null pointer") {
        val = true;
        CHECK(validateBool(nullptr, val) == false);
        CHECK(val == true); // Unchanged
    }

    SECTION("invalid - whitespace ' true'") {
        val = false;
        CHECK(validateBool(" true", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - whitespace 'true '") {
        val = false;
        CHECK(validateBool("true ", val) == false);
        CHECK(val == false); // Unchanged
    }

    SECTION("invalid - random string") {
        val = false;
        CHECK(validateBool("hello", val) == false);
        CHECK(val == false); // Unchanged
    }
}

TEST_CASE("Int validation") {
    int val = 0;

    SECTION("valid - zero") {
        CHECK(validateInt("0", 1, val) == true);
        CHECK(val == 0);
    }

    SECTION("valid - positive single digit") {
        CHECK(validateInt("5", 1, val) == true);
        CHECK(val == 5);
    }

    SECTION("valid - positive multi-digit") {
        CHECK(validateInt("12345", 5, val) == true);
        CHECK(val == 12345);
    }

    SECTION("valid - negative single digit") {
        CHECK(validateInt("-5", 2, val) == true);
        CHECK(val == -5);
    }

    SECTION("valid - negative multi-digit") {
        CHECK(validateInt("-12345", 6, val) == true);
        CHECK(val == -12345);
    }

    SECTION("valid - INT_MAX (2147483647)") {
        CHECK(validateInt("2147483647", 10, val) == true);
        CHECK(val == INT_MAX);
    }

    SECTION("valid - INT_MIN (-2147483648)") {
        CHECK(validateInt("-2147483648", 11, val) == true);
        CHECK(val == INT_MIN);
    }

    SECTION("invalid - overflow positive (2147483648)") {
        val = 42;
        CHECK(validateInt("2147483648", 10, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - overflow negative (-2147483649)") {
        val = 42;
        CHECK(validateInt("-2147483649", 11, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - large overflow (9999999999)") {
        val = 42;
        CHECK(validateInt("9999999999", 10, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - empty string") {
        val = 42;
        CHECK(validateInt("", 0, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - null pointer") {
        val = 42;
        CHECK(validateInt(nullptr, 0, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - floating point") {
        val = 42;
        CHECK(validateInt("12.34", 5, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - hex prefix '0x1F'") {
        val = 42;
        CHECK(validateInt("0x1F", 4, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - octal prefix '0777'") {
        // Note: from_chars treats this as decimal 777, which is valid
        // This test documents current behavior
        CHECK(validateInt("0777", 4, val) == true);
        CHECK(val == 777); // Parsed as decimal
    }

    SECTION("invalid - trailing characters '123abc'") {
        val = 42;
        CHECK(validateInt("123abc", 6, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - leading whitespace ' 123'") {
        val = 42;
        CHECK(validateInt(" 123", 4, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - trailing whitespace '123 '") {
        val = 42;
        CHECK(validateInt("123 ", 4, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - just minus sign '-'") {
        val = 42;
        CHECK(validateInt("-", 1, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - plus sign '+123'") {
        val = 42;
        // from_chars doesn't accept explicit plus sign
        CHECK(validateInt("+123", 4, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("invalid - letters only 'abc'") {
        val = 42;
        CHECK(validateInt("abc", 3, val) == false);
        CHECK(val == 42); // Unchanged
    }

    SECTION("valid - negative zero '-0'") {
        CHECK(validateInt("-0", 2, val) == true);
        CHECK(val == 0);
    }
}

TEST_CASE("String validation") {
    SECTION("valid - non-empty string") {
        CHECK(validateString("hello") == true);
    }

    SECTION("valid - single character") {
        CHECK(validateString("a") == true);
    }

    SECTION("valid - whitespace only") {
        // Whitespace-only strings are considered valid (non-empty)
        CHECK(validateString(" ") == true);
        CHECK(validateString("   ") == true);
        CHECK(validateString("\t") == true);
        CHECK(validateString("\n") == true);
    }

    SECTION("valid - special characters") {
        CHECK(validateString("!@#$%^&*()") == true);
    }

    SECTION("valid - unicode characters") {
        CHECK(validateString("こんにちは") == true);
        CHECK(validateString("🎉") == true);
    }

    SECTION("valid - very long string") {
        std::string longStr(1000, 'x');
        CHECK(validateString(longStr.c_str()) == true);
    }

    SECTION("invalid - empty string") {
        CHECK(validateString("") == false);
    }

    SECTION("invalid - null pointer") {
        CHECK(validateString(nullptr) == false);
    }
}

TEST_CASE("Bool validation - default value preservation") {
    SECTION("default true preserved on invalid input") {
        bool val = true;
        CHECK(validateBool("invalid", val) == false);
        CHECK(val == true); // Default preserved
    }

    SECTION("default false preserved on invalid input") {
        bool val = false;
        CHECK(validateBool("INVALID", val) == false);
        CHECK(val == false); // Default preserved
    }
}

TEST_CASE("Int validation - default value preservation") {
    SECTION("default preserved on invalid input") {
        int val = 999;
        CHECK(validateInt("not_a_number", 12, val) == false);
        CHECK(val == 999); // Default preserved
    }

    SECTION("default preserved on overflow") {
        int val = 999;
        CHECK(validateInt("99999999999999999999", 20, val) == false);
        CHECK(val == 999); // Default preserved
    }

    SECTION("default preserved on empty") {
        int val = 999;
        CHECK(validateInt("", 0, val) == false);
        CHECK(val == 999); // Default preserved
    }
}

TEST_CASE("Int validation - boundary values") {
    int val = 0;

    SECTION("INT_MAX boundary") {
        // INT_MAX = 2147483647
        CHECK(validateInt("2147483647", 10, val) == true);
        CHECK(val == 2147483647);
    }

    SECTION("INT_MAX + 1 overflow") {
        val = 0;
        CHECK(validateInt("2147483648", 10, val) == false);
    }

    SECTION("INT_MIN boundary") {
        // INT_MIN = -2147483648
        CHECK(validateInt("-2147483648", 11, val) == true);
        CHECK(val == -2147483648);
    }

    SECTION("INT_MIN - 1 overflow") {
        val = 0;
        CHECK(validateInt("-2147483649", 11, val) == false);
    }
}
