#include <type_traits>
#include <limits>

#include "spark_wiring_variant.h"

#include "util/catch.h"

using namespace particle;

namespace {

template<typename T>
void check(const Variant& v, const T& expectedValue = T()) {
    REQUIRE(v.is<T>());
    CHECK(v.value<T>() == expectedValue);
    CHECK(v.isConvertibleTo<T>());
    CHECK(v.to<T>() == expectedValue);
    if constexpr (std::is_same_v<T, std::monostate>) {
        CHECK(v.type() == Variant::NULL_);
        CHECK((v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, bool>) {
        CHECK(v.type() == Variant::BOOL);
        CHECK((!v.isNull() && v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toBool() == expectedValue);
    } else if constexpr (std::is_same_v<T, int>) {
        CHECK(v.type() == Variant::INT);
        CHECK((!v.isNull() && !v.isBool() && v.isInt() && !v.isInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt() == expectedValue);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.type() == Variant::INT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && v.isInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt64() == expectedValue);
    } else if constexpr (std::is_same_v<T, double>) {
        CHECK(v.type() == Variant::DOUBLE);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toDouble() == expectedValue);
    } else if constexpr (std::is_same_v<T, String>) {
        CHECK(v.type() == Variant::STRING);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toString() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.type() == Variant::ARRAY);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && v.isArray() && !v.isMap()));
        CHECK(v.toArray() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.type() == Variant::MAP);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && v.isMap()));
        CHECK(v.toMap() == expectedValue);
    } else {
        FAIL("Unexpected type");
    }
}

template<typename T>
void check(Variant& v, const T& expectedValue = T()) {
    check<T>(static_cast<const Variant&>(v), expectedValue);
    // Test non-const methods
    CHECK(v.as<T>() == expectedValue);
    if constexpr (std::is_same_v<T, bool>) {
        CHECK(v.asBool() == expectedValue);
    } else if constexpr (std::is_same_v<T, int>) {
        CHECK(v.asInt() == expectedValue);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.asInt64() == expectedValue);
    } else if constexpr (std::is_same_v<T, double>) {
        CHECK(v.asDouble() == expectedValue);
    } else if constexpr (std::is_same_v<T, String>) {
        CHECK(v.asString() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.asArray() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.asMap() == expectedValue);
    } else if constexpr (!std::is_same_v<T, std::monostate>) {
        FAIL("Unexpected type");
    }
}

} // namespace

TEST_CASE("Variant") {
    SECTION("can be constructed from a value of one of the supported types") {
        {
            Variant v;
            check<std::monostate>(v);
        }
        {
            Variant v(std::monostate{});
            check<std::monostate>(v);
        }
        {
            Variant v(true);
            check<bool>(v, true);
        }
        {
            Variant v((char)123);
            check<int>(v, 123);
        }
        {
            Variant v((unsigned char)123);
            check<int>(v, 123);
        }
        {
            Variant v((short)123);
            check<int>(v, 123);
        }
        {
            Variant v((unsigned short)123);
            check<int>(v, 123);
        }
        {
            Variant v(123);
            check<int>(v, 123);
        }
        {
            Variant v((unsigned)123);
            check<int64_t>(v, 123);
        }
        {
            Variant v(INT64_C(123));
            check<int64_t>(v, 123);
        }
        {
            Variant v(123.0);
            check<double>(v, 123.0);
        }
        {
            Variant v("abc");
            check<String>(v, "abc");
        }
        {
            Variant v(String("abc"));
            check<String>(v, "abc");
        }
        {
            VariantArray arr({ 1, 2, 3 });
            Variant v(arr);
            check<VariantArray>(v, arr);
        }
        {
            VariantMap map({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
            Variant v(map);
            check<VariantMap>(v, map);
        }
    }
}
