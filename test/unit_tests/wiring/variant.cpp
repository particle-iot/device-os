#include <type_traits>

#include "spark_wiring_variant.h"

#include "util/catch.h"

#include <iostream> // FIXME
#include <limits> // FIXME

using namespace particle;

namespace {

void checkNull(const Variant& v) {
    CHECK(v.type() == Variant::NULL_);
    CHECK((v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
}

template<typename T>
void check(const Variant& v, const T& expectedValue) {
    REQUIRE(v.is<T>());
    CHECK(v.value<T>() == expectedValue);
    CHECK(v.isConvertibleTo<T>());
    CHECK(v.to<T>() == expectedValue);
    if constexpr (std::is_same_v<T, bool>) {
        CHECK(v.type() == Variant::BOOL);
        CHECK((!v.isNull() && v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, int>) {
        CHECK(v.type() == Variant::INT);
        CHECK((!v.isNull() && !v.isBool() && v.isInt() && !v.isInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.type() == Variant::INT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && v.isInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, double>) {
        CHECK(v.type() == Variant::DOUBLE);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, String>) {
        CHECK(v.type() == Variant::STRING);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.type() == Variant::ARRAY);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.type() == Variant::MAP);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && v.isMap()));
    } else {
        FAIL("Unexpected type");
    }
}

template<typename T>
void check(Variant& v, const T& expectedValue) {
    check<T>(v, expectedValue);
    CHECK(v.as<T>() == expectedValue);
}

} // namespace

TEST_CASE("Variant") {
    SECTION("Variant()") {
        Variant v;
        checkNull(v);
    }

    SECTION("Variant(int)") {
        Variant v(123);
        check<int>(v, 123);
    }
/*
    SECTION("Variant(double)") {
        Variant v(123.4);
        check<double>(v, 123.4);
    }

    SECTION("Variant(const char*)") {
        Variant v("abc");
        check<String>(v, "abc");
    }

    SECTION("Variant(String)") {
        Variant v(String("abc"));
        check<String>(v, "abc");
    }
*/
}
