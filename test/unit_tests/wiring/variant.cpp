#include <type_traits>
#include <cmath>

#include "spark_wiring_variant.h"

#include "util/stream.h"
#include "util/string.h"
#include "util/catch.h"

using namespace particle;

namespace {

template<typename T>
void checkVariant(const Variant& v, const T& expectedValue = T()) {
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
        CHECK((v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
    } else if constexpr (std::is_same_v<T, bool>) {
        CHECK(v.type() == Variant::BOOL);
        CHECK((!v.isNull() && v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toBool() == expectedValue);
        bool ok = false;
        CHECK(v.toBool(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, int>) {
        CHECK(v.type() == Variant::INT);
        CHECK((!v.isNull() && !v.isBool() && v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt() == expectedValue);
        bool ok = false;
        CHECK(v.toInt(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, unsigned>) {
        CHECK(v.type() == Variant::UINT);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toUInt() == expectedValue);
        bool ok = false;
        CHECK(v.toUInt(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        CHECK(v.type() == Variant::INT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && v.isInt64() && !v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toInt64() == expectedValue);
        bool ok = false;
        CHECK(v.toInt64(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        CHECK(v.type() == Variant::UINT64);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && v.isUInt64() && !v.isDouble() && v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toUInt64() == expectedValue);
        bool ok = false;
        CHECK(v.toUInt64(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, double>) {
        CHECK(v.type() == Variant::DOUBLE);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && v.isDouble() && v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toDouble() == expectedValue);
        bool ok = false;
        CHECK(v.toDouble(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, String>) {
        CHECK(v.type() == Variant::STRING);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && v.isString() && !v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toString() == expectedValue);
        bool ok = false;
        CHECK(v.toString(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, Buffer>) {
        CHECK(v.type() == Variant::BUFFER);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && v.isBuffer() && !v.isArray() && !v.isMap()));
        CHECK(v.toBuffer() == expectedValue);
        bool ok = false;
        CHECK(v.toBuffer(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.type() == Variant::ARRAY);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isBuffer() && v.isArray() && !v.isMap()));
        CHECK(v.toArray() == expectedValue);
        bool ok = false;
        CHECK(v.toArray(ok) == expectedValue);
        CHECK(ok);
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.type() == Variant::MAP);
        CHECK((!v.isNull() && !v.isBool() && !v.isInt() && !v.isUInt() && !v.isInt64() && !v.isUInt64() && !v.isDouble() && !v.isNumber() && !v.isString() && !v.isBuffer() && !v.isArray() && v.isMap()));
        CHECK(v.toMap() == expectedValue);
        bool ok = false;
        CHECK(v.toMap(ok) == expectedValue);
        CHECK(ok);
    } else {
        FAIL("Unexpected type");
    }
}

template<typename T>
void checkVariant(Variant& v, const T& expectedValue = T()) {
    // Test const methods
    checkVariant<T>(static_cast<const Variant&>(v), expectedValue);
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
    } else if constexpr (std::is_same_v<T, Buffer>) {
        CHECK(v.asBuffer() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantArray>) {
        CHECK(v.asArray() == expectedValue);
    } else if constexpr (std::is_same_v<T, VariantMap>) {
        CHECK(v.asMap() == expectedValue);
    } else if constexpr (!std::is_same_v<T, std::monostate>) {
        FAIL("Unexpected type");
    }
}

std::string toCbor(const Variant& v) {
    test::Stream s;
    REQUIRE(encodeToCBOR(v, s) == 0);
    return s.data();
}

Variant fromCbor(const std::string& data) {
    test::Stream s(data);
    Variant v;
    REQUIRE(decodeFromCBOR(v, s) == 0);
    return v;
}

} // namespace

TEST_CASE("Variant") {
    SECTION("can be constructed from a value of one of the supported types") {
        {
            Variant v;
            checkVariant<std::monostate>(v);
        }
        {
            Variant v(std::monostate{});
            checkVariant<std::monostate>(v);
        }
        {
            Variant v(true);
            checkVariant<bool>(v, true);
        }
        {
            Variant v((char)123);
            checkVariant<int>(v, 123);
        }
        {
            Variant v((unsigned char)123);
            checkVariant<int>(v, 123);
        }
        {
            Variant v((short)123);
            checkVariant<int>(v, 123);
        }
        {
            Variant v((unsigned short)123);
            checkVariant<int>(v, 123);
        }
        {
            Variant v(123);
            checkVariant<int>(v, 123);
        }
        {
            Variant v(123u);
            checkVariant<unsigned>(v, 123);
        }
#ifdef __LP64__
        {
            Variant v(123l);
            checkVariant<int64_t>(v, 123);
        }
        {
            Variant v(123ul);
            checkVariant<uint64_t>(v, 123);
        }
#else
        {
            Variant v(123l);
            checkVariant<int>(v, 123);
        }
        {
            Variant v(123ul);
            checkVariant<unsigned>(v, 123);
        }
#endif
        {
            Variant v(123ll);
            checkVariant<int64_t>(v, 123);
        }
        {
            Variant v(123ull);
            checkVariant<uint64_t>(v, 123);
        }
        {
            Variant v(123.0f);
            checkVariant<double>(v, 123.0);
        }
        {
            Variant v(123.0);
            checkVariant<double>(v, 123.0);
        }
        {
            Variant v("abc");
            checkVariant<String>(v, "abc");
        }
        {
            Variant v(String("abc"));
            checkVariant<String>(v, "abc");
        }
        {
            Buffer buf("abc", 3);
            Variant v(buf);
            checkVariant<Buffer>(v, buf);
        }
        {
            VariantArray arr({ 1, 2, 3 });
            Variant v(arr);
            checkVariant<VariantArray>(v, arr);
        }
        {
            VariantMap map({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
            Variant v(map);
            checkVariant<VariantMap>(v, map);
        }
    }

    SECTION("can be assigned a value of one of the supported types") {
        Variant v;

        v = std::monostate{};
        checkVariant<std::monostate>(v);

        v = true;
        checkVariant<bool>(v, true);

        v = (char)123;
        checkVariant<int>(v, 123);

        v = (unsigned char)123;
        checkVariant<int>(v, 123);

        v = (short)123;
        checkVariant<int>(v, 123);

        v = (unsigned short)123;
        checkVariant<int>(v, 123);

        v = 123;
        checkVariant<int>(v, 123);

        v = 123u;
        checkVariant<unsigned>(v, 123);
#ifdef __LP64__
        v = 123l;
        checkVariant<int64_t>(v, 123);

        v = 123ul;
        checkVariant<uint64_t>(v, 123);
#else
        v = 123l;
        checkVariant<int>(v, 123);

        v = 123ul;
        checkVariant<unsigned>(v, 123);
#endif
        v = 123ll;
        checkVariant<int64_t>(v, 123);

        v = 123ull;
        checkVariant<uint64_t>(v, 123);

        v = 123.0f;
        checkVariant<double>(v, 123.0);

        v = 123.0;
        checkVariant<double>(v, 123.0);

        v = "abc";
        checkVariant<String>(v, "abc");

        v = String("abc");
        checkVariant<String>(v, "abc");

        Buffer buf("abc", 3);
        v = buf;
        checkVariant<Buffer>(v, buf);

        VariantArray arr({ 1, 2, 3 });
        v = arr;
        checkVariant<VariantArray>(v, arr);

        VariantMap map({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        v = map;
        checkVariant<VariantMap>(v, map);
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

        v = Buffer("\x01\x02\x03", 3);
        CHECK(v.toJSON() == "\"010203\"");

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
        checkVariant<std::monostate>(v);

        v = Variant::fromJSON("true");
        checkVariant(v, true);

        v = Variant::fromJSON("123");
        checkVariant(v, 123);

        v = Variant::fromJSON("123.5");
        checkVariant(v, 123.5);

        v = Variant::fromJSON("\"abc\"");
        checkVariant(v, String("abc"));

        v = Variant::fromJSON("[123,\"abc\"]");
        checkVariant(v, VariantArray{ 123, "abc" });

        v = Variant::fromJSON("{\"a\":1,\"b\":2,\"c\":3}");
        checkVariant(v, VariantMap{ { "a", 1 }, { "b", 2 }, { "c", 3 } });
    }

    SECTION("encodeToCBOR()") {
        using test::toHex;

        SECTION("encodes a Variant to CBOR") {
            // See https://datatracker.ietf.org/doc/html/rfc8949#section-appendix.a
            CHECK(toHex(toCbor(0)) == "00");
            CHECK(toHex(toCbor(1)) == "01");
            CHECK(toHex(toCbor(10)) == "0a");
            CHECK(toHex(toCbor(23)) == "17");
            CHECK(toHex(toCbor(24)) == "1818");
            CHECK(toHex(toCbor(25)) == "1819");
            CHECK(toHex(toCbor(100)) == "1864");
            CHECK(toHex(toCbor(1000)) == "1903e8");
            CHECK(toHex(toCbor(1000000)) == "1a000f4240");
            CHECK(toHex(toCbor(1000000000000ull)) == "1b000000e8d4a51000");
            CHECK(toHex(toCbor(18446744073709551615ull)) == "1bffffffffffffffff");
            CHECK(toHex(toCbor(-9223372036854775807ll - 1)) == "3b7fffffffffffffff");
            CHECK(toHex(toCbor(-1)) == "20");
            CHECK(toHex(toCbor(-10)) == "29");
            CHECK(toHex(toCbor(-100)) == "3863");
            CHECK(toHex(toCbor(-1000)) == "3903e7");
            CHECK(toHex(toCbor(0.0)) == "fa00000000"); // Encoding half-precision floats is not supported
            CHECK(toHex(toCbor(-0.0)) == "fa80000000"); // ditto
            CHECK(toHex(toCbor(1.0)) == "fa3f800000"); // ditto
            CHECK(toHex(toCbor(1.1)) == "fb3ff199999999999a");
            CHECK(toHex(toCbor(1.5)) == "fa3fc00000"); // ditto
            CHECK(toHex(toCbor(100000.0)) == "fa47c35000");
            CHECK(toHex(toCbor(16777216.0)) == "fa4b800000");
            CHECK(toHex(toCbor(3.4028234663852886e+38)) == "fa7f7fffff");
            CHECK(toHex(toCbor(1.0e+300)) == "fb7e37e43c8800759c");
            CHECK(toHex(toCbor(1.401298464324817e-45)) == "fa00000001");
            CHECK(toHex(toCbor(1.1754943508222875e-38)) == "fa00800000");
            CHECK(toHex(toCbor(-4.0)) == "fac0800000"); // ditto
            CHECK(toHex(toCbor(-4.1)) == "fbc010666666666666");
            CHECK(toHex(toCbor(INFINITY)) == "fa7f800000"); // ditto
            CHECK(toHex(toCbor(-INFINITY)) == "faff800000"); // ditto
            CHECK(toHex(toCbor(NAN)) == "fb7ff8000000000000"); // For simplicity, NaN is always encoded as a double
            CHECK(toHex(toCbor(false)) == "f4");
            CHECK(toHex(toCbor(true)) == "f5");
            CHECK(toHex(toCbor(Variant())) == "f6");
            CHECK(toHex(toCbor(Buffer())) == "40");
            CHECK(toHex(toCbor(Buffer("\x01\x02\x03\x04", 4))) == "4401020304");
            CHECK(toHex(toCbor("")) == "60");
            CHECK(toHex(toCbor("a")) == "6161");
            CHECK(toHex(toCbor("IETF")) == "6449455446");
            CHECK(toHex(toCbor("\"\\")) == "62225c");
            CHECK(toHex(toCbor("\u00fc")) == "62c3bc");
            CHECK(toHex(toCbor("\u6c34")) == "63e6b0b4");
            CHECK(toHex(toCbor(VariantArray{})) == "80");
            CHECK(toHex(toCbor(VariantArray{1, 2, 3})) == "83010203");
            CHECK(toHex(toCbor(VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}})) == "8301820203820405");
            CHECK(toHex(toCbor(VariantArray{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25})) == "98190102030405060708090a0b0c0d0e0f101112131415161718181819");
            CHECK(toHex(toCbor(VariantMap{})) == "a0");
            CHECK(toHex(toCbor(VariantMap{{"a", 1}, {"b", VariantArray{2, 3}}})) == "a26161016162820203");
            CHECK(toHex(toCbor(VariantArray{"a", VariantMap{{"b", "c"}}})) == "826161a161626163");
            CHECK(toHex(toCbor(VariantMap{{"a", "A"}, {"b", "B"}, {"c", "C"}, {"d", "D"}, {"e", "E"}})) == "a56161614161626142616361436164614461656145");
        }
    }

    SECTION("decodeFromCBOR()") {
        using test::fromHex;

        SECTION("decodes a Variant from CBOR") {
            // See https://datatracker.ietf.org/doc/html/rfc8949#section-appendix.a
            CHECK(fromCbor(fromHex("00")) == Variant(0));
            CHECK(fromCbor(fromHex("01")) == Variant(1));
            CHECK(fromCbor(fromHex("0a")) == Variant(10));
            CHECK(fromCbor(fromHex("17")) == Variant(23));
            CHECK(fromCbor(fromHex("1818")) == Variant(24));
            CHECK(fromCbor(fromHex("1819")) == Variant(25));
            CHECK(fromCbor(fromHex("1864")) == Variant(100));
            CHECK(fromCbor(fromHex("1903e8")) == Variant(1000));
            CHECK(fromCbor(fromHex("1a000f4240")) == Variant(1000000));
            CHECK(fromCbor(fromHex("1b000000e8d4a51000")) == Variant(1000000000000ull));
            CHECK(fromCbor(fromHex("1bffffffffffffffff")) == Variant(18446744073709551615ull));
            CHECK(fromCbor(fromHex("3b7fffffffffffffff")) == Variant(-9223372036854775807ll - 1));
            CHECK(fromCbor(fromHex("20")) == Variant(-1));
            CHECK(fromCbor(fromHex("29")) == Variant(-10));
            CHECK(fromCbor(fromHex("3863")) == Variant(-100));
            CHECK(fromCbor(fromHex("3903e7")) == Variant(-1000));
            CHECK(fromCbor(fromHex("f90000")) == Variant(0.0));
            CHECK(fromCbor(fromHex("f98000")) == Variant(-0.0));
            CHECK(fromCbor(fromHex("f93c00")) == Variant(1.0));
            CHECK(fromCbor(fromHex("fb3ff199999999999a")) == Variant(1.1));
            CHECK(fromCbor(fromHex("f93e00")) == Variant(1.5));
            CHECK(fromCbor(fromHex("f97bff")) == Variant(65504.0));
            CHECK(fromCbor(fromHex("fa47c35000")) == Variant(100000.0));
            CHECK(fromCbor(fromHex("fa7f7fffff")) == Variant(3.4028234663852886e+38));
            CHECK(fromCbor(fromHex("fb7e37e43c8800759c")) == Variant(1.0e+300));
            CHECK(fromCbor(fromHex("f90001")) == Variant(5.960464477539063e-8));
            CHECK(fromCbor(fromHex("f90400")) == Variant(0.00006103515625));
            CHECK(fromCbor(fromHex("f9c400")) == Variant(-4.0));
            CHECK(fromCbor(fromHex("fbc010666666666666")) == Variant(-4.1));
            CHECK(fromCbor(fromHex("f97c00")) == Variant(INFINITY));
            CHECK(std::isnan(fromCbor(fromHex("f97e00")).asDouble()));
            CHECK(fromCbor(fromHex("f9fc00")) == Variant(-INFINITY));
            CHECK(fromCbor(fromHex("fa7f800000")) == Variant(INFINITY));
            CHECK(std::isnan(fromCbor(fromHex("fa7fc00000")).asDouble()));
            CHECK(fromCbor(fromHex("faff800000")) == Variant(-INFINITY));
            CHECK(fromCbor(fromHex("fb7ff0000000000000")) == Variant(INFINITY));
            CHECK(std::isnan(fromCbor(fromHex("fb7ff8000000000000")).asDouble()));
            CHECK(fromCbor(fromHex("fbfff0000000000000")) == Variant(-INFINITY));
            CHECK(fromCbor(fromHex("f4")) == Variant(false));
            CHECK(fromCbor(fromHex("f5")) == Variant(true));
            CHECK(fromCbor(fromHex("f6")) == Variant());
            CHECK(fromCbor(fromHex("c074323031332d30332d32315432303a30343a30305a")) == Variant("2013-03-21T20:04:00Z"));
            CHECK(fromCbor(fromHex("c11a514b67b0")) == Variant(1363896240));
            CHECK(fromCbor(fromHex("c1fb41d452d9ec200000")) == Variant(1363896240.5));
            CHECK(fromCbor(fromHex("d82076687474703a2f2f7777772e6578616d706c652e636f6d")) == Variant("http://www.example.com"));
            CHECK(fromCbor(fromHex("40")) == Variant(Buffer()));
            CHECK(fromCbor(fromHex("4401020304")) == Variant(Buffer("\x01\x02\x03\x04", 4)));
            CHECK(fromCbor(fromHex("60")) == Variant(""));
            CHECK(fromCbor(fromHex("6161")) == Variant("a"));
            CHECK(fromCbor(fromHex("6449455446")) == Variant("IETF"));
            CHECK(fromCbor(fromHex("62225c")) == Variant("\"\\"));
            CHECK(fromCbor(fromHex("62c3bc")) == Variant("\u00fc"));
            CHECK(fromCbor(fromHex("63e6b0b4")) == Variant("\u6c34"));
            CHECK(fromCbor(fromHex("80")) == VariantArray{});
            CHECK(fromCbor(fromHex("83010203")) == VariantArray{1, 2, 3});
            CHECK(fromCbor(fromHex("8301820203820405")) == VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}});
            CHECK(fromCbor(fromHex("98190102030405060708090a0b0c0d0e0f101112131415161718181819")) == VariantArray{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25});
            CHECK(fromCbor(fromHex("a0")) == VariantMap{});
            CHECK(fromCbor(fromHex("a26161016162820203")) == VariantMap{{"a", 1}, {"b", VariantArray{2, 3}}});
            CHECK(fromCbor(fromHex("826161a161626163")) == VariantArray{"a", VariantMap{{"b", "c"}}});
            CHECK(fromCbor(fromHex("a56161614161626142616361436164614461656145")) == VariantMap{{"a", "A"}, {"b", "B"}, {"c", "C"}, {"d", "D"}, {"e", "E"}});
            CHECK(fromCbor(fromHex("5f42010243030405ff")) == Variant(Buffer("\x01\x02\x03\x04\x05", 5)));
            CHECK(fromCbor(fromHex("7f657374726561646d696e67ff")) == Variant("streaming"));
            CHECK(fromCbor(fromHex("9fff")) == VariantArray{});
            CHECK(fromCbor(fromHex("9f018202039f0405ffff")) == VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}});
            CHECK(fromCbor(fromHex("9f01820203820405ff")) == VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}});
            CHECK(fromCbor(fromHex("83018202039f0405ff")) == VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}});
            CHECK(fromCbor(fromHex("83019f0203ff820405")) == VariantArray{1, VariantArray{2, 3}, VariantArray{4, 5}});
            CHECK(fromCbor(fromHex("9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff")) == VariantArray{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25});
            CHECK(fromCbor(fromHex("bf61610161629f0203ffff")) == VariantMap{{"a", 1}, {"b", VariantArray{2, 3}}});
            CHECK(fromCbor(fromHex("826161bf61626163ff")) == VariantArray{"a", VariantMap{{"b", "c"}}});
            CHECK(fromCbor(fromHex("bf6346756ef563416d7421ff")) == VariantMap{{"Fun", true}, {"Amt", -2}});
        }

        SECTION("parses a map with non-string keys as an array of key-value pairs") {
            CHECK(fromCbor(fromHex("a2616101026162")) == VariantArray{VariantArray{"a", 1}, VariantArray{2, "b"}}); // {"a": 1, 2: "b"}
            CHECK(fromCbor(fromHex("a181018102")) == VariantArray{VariantArray{VariantArray{1}, VariantArray{2}}}); // {[1]: [2]}
        }
    }

    SECTION("getCBORSize()") {
        SECTION("returns the size of a Variant in CBOR format") {
            Variant v = VariantMap{
                {"a", 1234},
                {"b", Buffer::fromHex("1234")},
                {"c", VariantArray{1, 2, 3, 4}}
            };
            auto s = toCbor(v);
            CHECK(test::toHex(s) == "a361611904d2616242123461638401020304");
            CHECK(getCBORSize(v) == s.size());
        }
    }
}
