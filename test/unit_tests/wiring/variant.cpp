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
    CHECK(v.to<T>() == expectedValue);
    bool ok = false;
    CHECK(v.to<T>(ok) == expectedValue);
    CHECK(ok);
    CHECK(v == expectedValue);
    CHECK(expectedValue == v);
    CHECK_FALSE(v != expectedValue);
    CHECK_FALSE(expectedValue != v);
    if constexpr (std::is_same_v<T, std::monostate>) {
        CHECK(v.type() == Variant::NULL_);
        CHECK((v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, bool>) {
        CHECK(v.type() == Variant::BOOL);
        CHECK((!v.isNull() && v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toBool() == expectedValue);
    } else if constexpr (std::is_same_v<T, int>) {
        CHECK(v.type() == Variant::INT);
        CHECK((!v.isNull() && !v.isBool() && v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt() == expectedValue);
    } else if constexpr (std::is_same_v<T, unsigned>) {
        CHECK(v.type() == Variant::UINT);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toUInt() == expectedValue);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.type() == Variant::INT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt64() == expectedValue);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        CHECK(v.type() == Variant::UINT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toUInt64() == expectedValue);
    } else if constexpr (std::is_same_v<T, double>) {
        CHECK(v.type() == Variant::DOUBLE);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && v.isDouble() && v.isNumber() && !v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toDouble() == expectedValue);
    } else if constexpr (std::is_same_v<T, String>) {
        CHECK(v.type() == Variant::STRING);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && v.isString() && !v.isArray() && !v.isMap()));
        CHECK(v.toString() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.type() == Variant::ARRAY);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && v.isArray() && !v.isMap()));
        CHECK(v.toArray() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.type() == Variant::MAP);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isArray() && v.isMap()));
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
    } else if constexpr (std::is_same_v<T, unsigned>) {
        CHECK(v.asUInt() == expectedValue);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.asInt64() == expectedValue);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        CHECK(v.asUInt64() == expectedValue);
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
            Variant v(123u);
            check<unsigned>(v, 123);
        }
#ifdef __LP64__
        {
            Variant v(123l);
            check<int64_t>(v, 123);
        }
        {
            Variant v(123ul);
            check<uint64_t>(v, 123);
        }
#else
        {
            Variant v(123l);
            check<int>(v, 123);
        }
        {
            Variant v(123ul);
            check<unsigned>(v, 123);
        }
#endif
        {
            Variant v(123ll);
            check<int64_t>(v, 123);
        }
        {
            Variant v(123ull);
            check<uint64_t>(v, 123);
        }
        {
            Variant v(123.0f);
            check<double>(v, 123.0);
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

    SECTION("can be assigned a value of one of the supported types") {
        Variant v;

        v = std::monostate{};
        check<std::monostate>(v);

        v = true;
        check<bool>(v, true);

        v = (char)123;
        check<int>(v, 123);

        v = (unsigned char)123;
        check<int>(v, 123);

        v = (short)123;
        check<int>(v, 123);

        v = (unsigned short)123;
        check<int>(v, 123);

        v = 123;
        check<int>(v, 123);

        v = 123u;
        check<unsigned>(v, 123);
#ifdef __LP64__
        v = 123l;
        check<int64_t>(v, 123);

        v = 123ul;
        check<uint64_t>(v, 123);
#else
        v = 123l;
        check<int>(v, 123);

        v = 123ul;
        check<unsigned>(v, 123);
#endif
        v = 123ll;
        check<int64_t>(v, 123);

        v = 123ull;
        check<uint64_t>(v, 123);

        v = 123.0f;
        check<double>(v, 123.0);

        v = 123.0;
        check<double>(v, 123.0);

        v = "abc";
        check<String>(v, "abc");

        v = String("abc");
        check<String>(v, "abc");

        VariantArray arr({ 1, 2, 3 });
        v = arr;
        check<VariantArray>(v, arr);

        VariantMap map({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        v = map;
        check<VariantMap>(v, map);
    }

    SECTION("can be converted to JSON") {
        Variant v;
        CHECK(v.toJSON() == "null");

        v = true;
        CHECK(v.toJSON() == "true");

        v = 123;
        CHECK(v.toJSON() == "123");

        v = 123.5;
        CHECK(v.toJSON() == "123.5");

        v = "abc";
        CHECK(v.toJSON() == "\"abc\"");

        v.append(123);
        v.append("abc");
        CHECK(v.toJSON() == "[123,\"abc\"]");

        v.set("a", 1);
        v.set("b", 2);
        v.set("c", 3);
        CHECK(v.toJSON() == "{\"a\":1,\"b\":2,\"c\":3}");
    }

    SECTION("can be converted from JSON") {
        Variant v = Variant::fromJSON("null");
        check<std::monostate>(v);

        v = Variant::fromJSON("true");
        check(v, true);

        v = Variant::fromJSON("123");
        check(v, 123);

        v = Variant::fromJSON("123.5");
        check(v, 123.5);

        v = Variant::fromJSON("\"abc\"");
        check(v, String("abc"));

        v = Variant::fromJSON("[123,\"abc\"]");
        check(v, VariantArray{ 123, "abc" });

        v = Variant::fromJSON("{\"a\":1,\"b\":2,\"c\":3}");
        check(v, VariantMap{ { "a", 1 }, { "b", 2 }, { "c", 3 } });
    }
}
