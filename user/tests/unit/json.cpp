#include "spark_wiring_json.h"
#include "spark_wiring_print.h"

#include "tools/stream.h"
#include "tools/buffer.h"

#include <boost/variant.hpp>

#include <deque>
#include <string>
#include <cstdlib>

namespace {

using namespace spark;

class Checker {
public:
    Checker() {
    }

    explicit Checker(const JSONValue &val) {
        v_.push_back(val);
    }

    Checker& invalid() {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_INVALID);
        CHECK(v.isValid() == false);
        return *this;
    }

    Checker& null() {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_NULL);
        CHECK(v.isNull() == true);
        return *this;
    }

    Checker& boolean(bool val) {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_BOOL);
        CHECK(v.isBool() == true);
        CHECK(v.toBool() == val);
        return *this;
    }

    Checker& number(int val) {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_NUMBER);
        CHECK(v.isNumber());
        CHECK(v.toInt() == val);
        return *this;
    }

    Checker& number(double val) {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_NUMBER);
        CHECK(v.isNumber());
        CHECK(v.toDouble() == val);
        return *this;
    }

    Checker& string(const std::string &str) {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_STRING);
        CHECK(v.isString() == true);
        const JSONString s = v.toString();
        CHECK(std::string(s.data(), s.size()) == str);
        return *this;
    }

    Checker& beginArray() {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_ARRAY);
        CHECK(v.isArray() == true);
        v_.push_back(JSONArrayIterator(v));
        return *this;
    }

    Checker& endArray() {
        const Variant v = v_.back();
        if (v.which() != ARRAY) {
            FAIL("Unexpected checker state");
        }
        v_.pop_back();
        JSONArrayIterator it = boost::get<JSONArrayIterator>(v);
        REQUIRE(it.count() == 0);
        CHECK(it.next() == false);
        return *this;
    }

    Checker& beginObject() {
        const JSONValue v = value();
        REQUIRE(v.type() == JSON_TYPE_OBJECT);
        CHECK(v.isObject() == true);
        v_.push_back(JSONObjectIterator(v));
        return *this;
    }

    Checker& endObject() {
        const Variant v = v_.back();
        if (v.which() != OBJECT) {
            FAIL("Unexpected checker state");
        }
        v_.pop_back();
        JSONObjectIterator it = boost::get<JSONObjectIterator>(v);
        REQUIRE(it.count() == 0);
        CHECK(it.next() == false);
        return *this;
    }

    Checker& name(const std::string &str) {
        Variant &v = v_.back();
        if (v.which() != OBJECT) {
            FAIL("Unexpected checker state");
        }
        JSONObjectIterator it = boost::get<JSONObjectIterator>(v);
        const size_t n = it.count();
        REQUIRE(it.next() == true);
        CHECK(it.count() == n - 1);
        const JSONString s = it.name();
        CHECK(std::string(s.data(), s.size()) == str);
        v = it; // Update iterator
        return *this;
    }

private:
    enum Which {
        BLANK,
        VALUE,
        ARRAY,
        OBJECT
    };

    typedef boost::variant<boost::blank, JSONValue, JSONArrayIterator, JSONObjectIterator> Variant;

    std::deque<Variant> v_;

    JSONValue value() {
        if (v_.empty()) {
            FAIL("Unexpected checker state");
        }
        Variant &v = v_.back();
        switch (v.which()) {
        case VALUE: {
            return boost::get<JSONValue>(v);
        }
        case ARRAY: { // JSONArrayIterator
            JSONArrayIterator it = boost::get<JSONArrayIterator>(v);
            const size_t n = it.count();
            REQUIRE(it.next() == true);
            CHECK(it.count() == n - 1);
            v = it; // Update iterator
            return it.value();
        }
        case OBJECT: { // JSONObjectIterator
            const JSONObjectIterator it = boost::get<JSONObjectIterator>(v);
            return it.value();
        }
        default:
            FAIL("Unexpected checker state");
            return JSONValue();
        }
    }
};

inline JSONValue parse(const std::string &json) {
    return JSONValue::parseCopy(json.data(), json.size());
}

inline Checker check(const JSONValue &val) {
    return Checker(val);
}

inline Checker check(const char *json) {
    return Checker(parse(json));
}

} // namespace

namespace spark {

inline std::ostream& operator<<(std::ostream &strm, const JSONString &str) {
    return strm << '"' << str.data() << '"';
}

} // namespace spark

TEST_CASE("Parsing JSON") {
    SECTION("null") {
        check("null").null();
    }

    SECTION("bool") {
        check("true").boolean(true);
        check("false").boolean(false);
    }

    SECTION("number") {
        SECTION("int") {
            check("0").number(0);
            check("1").number(1);
            check("-1").number(-1);
            check("12345").number(12345);
            check("-12345").number(-12345);
            check("-2147483648").number((int)-2147483648); // INT_MIN
            check("2147483647").number((int)2147483647); // INT_MAX
        }
        SECTION("float") {
            check("0.0").number(0.0);
            check("1.0").number(1.0);
            check("-1.0").number(-1.0);
            check("0.5").number(0.5);
            check("-0.5").number(-0.5);
            check("3.1416").number(3.1416);
            check("-3.1416").number(-3.1416);
            check("1.17549e-38").number(1.17549e-38); // ~FLT_MIN
            check("3.40282e+38").number(3.40282e+38); // ~FLT_MAX
        }
    }

    SECTION("string") {
        check("\"\"").string(""); // Empty string
        check("\"abc\"").string("abc");
        check("\"a\"").string("a"); // Single character
        check("\"\\\"\"").string("\""); // Single escaped character
        check("\"\\\"\\/\\\\\\b\\f\\n\\r\\t\"").string("\"/\\\b\f\n\r\t"); // Named escaped characters
        check("\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\u0008\\u0009\\u000a\\u000b"
                "\\u000c\\u000d\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019"
                "\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\"").string(
                std::string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15"
                        "\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", 32)); // Control characters
        check("\"\\u2014\"").string("\\u2014"); // Unicode characters are not processed
    }

    SECTION("array") {
        SECTION("empty") {
            check("[]").beginArray().endArray();
        }
        SECTION("single element") {
            check("[null]").beginArray()
                    .null()
                    .endArray();
        }
        SECTION("primitive elements") {
            check("[null,true,2,3.14,\"abcd\"]").beginArray()
                    .null()
                    .boolean(true)
                    .number(2)
                    .number(3.14)
                    .string("abcd")
                    .endArray();
        }
        SECTION("nested array") {
            check("[1.1,[2.1,2.2,2.3],1.3]").beginArray()
                    .number(1.1)
                    .beginArray()
                            .number(2.1)
                            .number(2.2)
                            .number(2.3)
                            .endArray()
                    .number(1.3)
                    .endArray();
        }
        SECTION("nested object") {
            check("[1.1,{\"2.1\":2.1,\"2.2\":2.2,\"2.3\":2.3},1.3]").beginArray()
                    .number(1.1)
                    .beginObject()
                            .name("2.1").number(2.1)
                            .name("2.2").number(2.2)
                            .name("2.3").number(2.3)
                            .endObject()
                    .number(1.3)
                    .endArray();
        }
        SECTION("deeply nested array") {
            Checker c = check("[[[[[[[[[[[]]]]]]]]]]]");
            c.beginArray();
            for (int i = 0; i < 10; ++i) {
                c.beginArray();
            }
            for (int i = 0; i < 10; ++i) {
                c.endArray();
            }
            c.endArray();
        }
    }

    SECTION("object") {
        SECTION("empty") {
            check("{}").beginObject().endObject();
        }
        SECTION("single element") {
            check("{\"null\":null}").beginObject()
                    .name("null").null()
                    .endObject();
        }
        SECTION("primitive elements") {
            check("{\"null\":null,\"bool\":true,\"int\":2,\"float\":3.14,\"string\":\"abcd\"}").beginObject()
                    .name("null").null()
                    .name("bool").boolean(true)
                    .name("int").number(2)
                    .name("float").number(3.14)
                    .name("string").string("abcd")
                    .endObject();
        }
        SECTION("nested object") {
            check("{\"1.1\":1.1,\"1.2\":{\"2.1\":2.1,\"2.2\":2.2,\"2.3\":2.3},\"1.3\":1.3}").beginObject()
                    .name("1.1").number(1.1)
                    .name("1.2").beginObject()
                            .name("2.1").number(2.1)
                            .name("2.2").number(2.2)
                            .name("2.3").number(2.3)
                            .endObject()
                    .name("1.3").number(1.3)
                    .endObject();
        }
        SECTION("nested array") {
            check("{\"1.1\":1.1,\"1.2\":[2.1,2.2,2.3],\"1.3\":1.3}").beginObject()
                    .name("1.1").number(1.1)
                    .name("1.2").beginArray()
                            .number(2.1)
                            .number(2.2)
                            .number(2.3)
                            .endArray()
                    .name("1.3").number(1.3)
                    .endObject();
        }
        SECTION("deeply nested object") {
            Checker c = check("{\"1\":{\"2\":{\"3\":{\"4\":{\"5\":{\"6\":{\"7\":{\"8\":{\"9\":{\"10\":{}}}}}}}}}}}");
            c.beginObject();
            for (int i = 1; i <= 10; ++i) {
                c.name(std::to_string(i)).beginObject();
            }
            for (int i = 1; i <= 10; ++i) {
                c.endObject();
            }
            c.endObject();
        }
    }

    SECTION("parsing errors") {
        check("").invalid(); // Empty source data
        check("[").invalid(); // Malformed array
        check("]").invalid();
        check("[1,").invalid();
        check("{").invalid(); // Malformed object
        check("}").invalid();
        check("{null").invalid();
        check("{false").invalid();
        check("{1").invalid();
        check("{\"1\"").invalid();
        check("{\"1\":").invalid();
        check("\"\\x\"").invalid(); // Unknown escaped character
        check("\"\\U0001\"").invalid(); // Uppercase 'U'
        check("\"\\u000x\"").invalid(); // Invalid hex value
        check("\"\\u001\"").invalid();
        check("\"\\u01\"").invalid();
        check("\"\\u\"").invalid();
    }
}

TEST_CASE("Writing JSON") {
    test::OutputStream data;
    JSONStreamWriter json(data);

    SECTION("null") {
        json.nullValue();
        check(data).equals("null");
    }

    SECTION("bool") {
        SECTION("true") {
            json.value(true);
            check(data).equals("true");
        }
        SECTION("false") {
            json.value(false);
            check(data).equals("false");
        }
    }

    SECTION("number") {
        SECTION("int") {
            SECTION("0") {
                json.value(0);
                check(data).equals("0");
            }
            SECTION("1") {
                json.value(1);
                check(data).equals("1");
            }
            SECTION("-1") {
                json.value(-1);
                check(data).equals("-1");
            }
            SECTION("12345") {
                json.value(12345);
                check(data).equals("12345");
            }
            SECTION("-12345") {
                json.value(-12345);
                check(data).equals("-12345");
            }
            SECTION("-2147483648") {
                json.value((int)-2147483648); // INT_MIN
                check(data).equals("-2147483648");
            }
            SECTION("2147483647") {
                json.value((int)2147483647); // INT_MAX
                check(data).equals("2147483647");
            }
        }
        SECTION("float") {
            SECTION("0.0") {
                json.value(0.0);
                check(data).equals("0");
            }
            SECTION("1.0") {
                json.value(1.0);
                check(data).equals("1");
            }
            SECTION("-1.0") {
                json.value(-1.0);
                check(data).equals("-1");
            }
            SECTION("0.5") {
                json.value(0.5);
                check(data).equals("0.5");
            }
            SECTION("-0.5") {
                json.value(-0.5);
                check(data).equals("-0.5");
            }
            SECTION("3.1416") {
                json.value(3.1416);
                check(data).equals("3.1416");
            }
            SECTION("-3.1416") {
                json.value(-3.1416);
                check(data).equals("-3.1416");
            }
            SECTION("1.17549e-38") {
                json.value(1.17549e-38); // ~FLT_MIN
                check(data).equals("1.17549e-38");
            }
            SECTION("3.40282e+38") {
                json.value(3.40282e+38); // ~FLT_MAX
                check(data).equals("3.40282e+38");
            }
        }
    }

    SECTION("string") {
        SECTION("empty") {
            json.value("");
            check(data).equals("\"\"");
        }
        SECTION("test string") {
            json.value("abc");
            check(data).equals("\"abc\"");
        }
        SECTION("single character") {
            json.value("a");
            check(data).equals("\"a\"");
        }
        SECTION("single escaped character") {
            json.value("\"");
            check(data).equals("\"\\\"\"");
        }
        SECTION("named escaped characters") {
            json.value("\"/\\\b\f\n\r\t");
            check(data).equals("\"\\\"/\\\\\\b\\f\\n\\r\\t\""); // '/' is never escaped
        }
        SECTION("control characters") {
            json.value("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16"
                    "\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", 32);
            check(data).equals("\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000b\\f\\r\\u000e"
                    "\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c"
                    "\\u001d\\u001e\\u001f\"");
        }
    }

    SECTION("array") {
        SECTION("empty") {
            json.beginArray();
            json.endArray();
            check(data).equals("[]");
        }
        SECTION("single element") {
            json.beginArray();
            json.nullValue();
            json.endArray();
            check(data).equals("[null]");
        }
        SECTION("primitive elements") {
            json.beginArray();
            json.nullValue().value(true).value(2).value(3.14).value("abcd");
            json.endArray();
            check(data).equals("[null,true,2,3.14,\"abcd\"]");
        }
        SECTION("nested array") {
            json.beginArray();
            json.value(1.1);
            json.beginArray();
            json.value(2.1).value(2.2).value(2.3);
            json.endArray();
            json.value(1.3);
            json.endArray();
            check(data).equals("[1.1,[2.1,2.2,2.3],1.3]");
        }
        SECTION("nested object") {
            json.beginArray();
            json.value(1.1);
            json.beginObject();
            json.name("2.1").value(2.1);
            json.name("2.2").value(2.2);
            json.name("2.3").value(2.3);
            json.endObject();
            json.value(1.3);
            json.endArray();
            check(data).equals("[1.1,{\"2.1\":2.1,\"2.2\":2.2,\"2.3\":2.3},1.3]");
        }
        SECTION("deeply nested array") {
            json.beginArray();
            for (int i = 0; i < 10; ++i) {
                json.beginArray();
            }
            for (int i = 0; i < 10; ++i) {
                json.endArray();
            }
            json.endArray();
            check(data).equals("[[[[[[[[[[[]]]]]]]]]]]");
        }
    }

    SECTION("object") {
        SECTION("empty") {
            json.beginObject();
            json.endObject();
            check(data).equals("{}");
        }
        SECTION("single element") {
            json.beginObject();
            json.name("null").nullValue();
            json.endObject();
            check(data).equals("{\"null\":null}");
        }
        SECTION("primitive elements") {
            json.beginObject();
            json.name("null").nullValue();
            json.name("bool").value(true);
            json.name("int").value(2);
            json.name("float").value(3.14);
            json.name("string").value("abcd");
            json.endObject();
            check(data).equals("{\"null\":null,\"bool\":true,\"int\":2,\"float\":3.14,\"string\":\"abcd\"}");
        }
        SECTION("nested object") {
            json.beginObject();
            json.name("1.1").value(1.1);
            json.name("1.2").beginObject();
            json.name("2.1").value(2.1);
            json.name("2.2").value(2.2);
            json.name("2.3").value(2.3);
            json.endObject();
            json.name("1.3").value(1.3);
            json.endObject();
            check(data).equals("{\"1.1\":1.1,\"1.2\":{\"2.1\":2.1,\"2.2\":2.2,\"2.3\":2.3},\"1.3\":1.3}");
        }
        SECTION("nested array") {
            json.beginObject();
            json.name("1.1").value(1.1);
            json.name("1.2").beginArray();
            json.value(2.1).value(2.2).value(2.3);
            json.endArray();
            json.name("1.3").value(1.3);
            json.endObject();
            check(data).equals("{\"1.1\":1.1,\"1.2\":[2.1,2.2,2.3],\"1.3\":1.3}");
        }
        SECTION("deeply nested object") {
            json.beginObject();
            for (int i = 1; i <= 10; ++i) {
                json.name(std::to_string(i).c_str()).beginObject();
            }
            for (int i = 1; i <= 10; ++i) {
                json.endObject();
            }
            json.endObject();
            check(data).equals("{\"1\":{\"2\":{\"3\":{\"4\":{\"5\":{\"6\":{\"7\":{\"8\":{\"9\":{\"10\":{}}}}}}}}}}}");
        }
        SECTION("escaped name characters") {
            json.beginObject();
            json.name("a\tb\n").value("a\tb\n");
            json.endObject();
            check(data).equals("{\"a\\tb\\n\":\"a\\tb\\n\"}");
        }
    }
}

TEST_CASE("JSONValue") {
    SECTION("construction") {
        const JSONValue v; // Constructs invalid value
        check(v).invalid();
    }

    SECTION("type conversions") {
        SECTION("invalid") {
            const JSONValue v;
            check(v).invalid();
            CHECK(v.toBool() == false);
            CHECK(v.toInt() == 0);
            CHECK(v.toDouble() == 0.0);
            CHECK(v.toString() == "");
        }
        SECTION("null") {
            const JSONValue v = parse("null");
            check(v).null();
            CHECK(v.toBool() == false);
            CHECK(v.toInt() == 0);
            CHECK(v.toDouble() == 0.0);
            CHECK(v.toString() == "");
        }
        SECTION("bool") {
            SECTION("true") {
                const JSONValue v = parse("true");
                check(v).boolean(true);
                CHECK(v.toInt() == 1);
                CHECK(v.toDouble() == 1.0);
                CHECK(v.toString() == "true");
            }
            SECTION("false") {
                const JSONValue v = parse("false");
                check(v).boolean(false);
                CHECK(v.toInt() == 0);
                CHECK(v.toDouble() == 0.0);
                CHECK(v.toString() == "false");
            }
        }
        SECTION("number") {
            SECTION("int") {
                SECTION("0") {
                    const JSONValue v = parse("0");
                    check(v).number(0);
                    CHECK(v.toBool() == false);
                    CHECK(v.toDouble() == 0.0);
                    CHECK(v.toString() == "0");
                }
                SECTION("12345") {
                    const JSONValue v = parse("12345");
                    check(v).number(12345);
                    CHECK(v.toBool() == true);
                    CHECK(v.toDouble() == 12345.0);
                    CHECK(v.toString() == "12345");
                }
            }
            SECTION("float") {
                SECTION("0.0") {
                    const JSONValue v = parse("0.0");
                    check(v).number(0.0);
                    CHECK(v.toBool() == false);
                    CHECK(v.toInt() == 0);
                    CHECK(v.toString() == "0.0");
                }
                SECTION("3.1416") {
                    const JSONValue v = parse("3.1416");
                    check(v).number(3.1416);
                    CHECK(v.toBool() == true);
                    CHECK(v.toInt() == 3);
                    CHECK(v.toString() == "3.1416");
                }
                SECTION("1.0e+1") {
                    const JSONValue v = parse("1.0e+1");
                    check(v).number(1.0e+1);
                    CHECK(v.toBool() == true);
                    CHECK(v.toInt() == 1); // toInt() may produce incorrect results for floating point numbers
                    CHECK(v.toString() == "1.0e+1");
                }
            }
        }
        SECTION("string") {
            SECTION("empty") {
                const JSONValue v = parse("\"\"");
                check(v).string("");
                CHECK(v.toBool() == false);
                CHECK(v.toInt() == 0);
                CHECK(v.toDouble() == 0.0);
            }
            SECTION("abc") {
                const JSONValue v = parse("\"abc\"");
                check(v).string("abc");
                CHECK(v.toBool() == true); // Non-empty string which is not "false"
                CHECK(v.toInt() == 0);
                CHECK(v.toDouble() == 0.0);
            }
            SECTION("true") {
                const JSONValue v = parse("\"true\"");
                check(v).string("true");
                CHECK(v.toBool() == true);
                CHECK(v.toInt() == 0);
                CHECK(v.toDouble() == 0.0);
            }
            SECTION("false") {
                const JSONValue v = parse("\"false\"");
                check(v).string("false");
                CHECK(v.toBool() == false);
                CHECK(v.toInt() == 0);
                CHECK(v.toDouble() == 0.0);
            }
            SECTION("3.1416") {
                const JSONValue v = parse("\"3.1416\"");
                check(v).string("3.1416");
                CHECK(v.toBool() == true);
                CHECK(v.toInt() == 3);
                CHECK(v.toDouble() == 3.1416);
            }
            SECTION("1.0e+1") {
                const JSONValue v = parse("\"1.0e+1\"");
                check(v).string("1.0e+1");
                CHECK(v.toBool() == true);
                CHECK(v.toInt() == 1); // toInt() may produce incorrect results for floating point numbers
                CHECK(v.toDouble() == 1.0e+1);
            }
        }
        SECTION("array") {
            const JSONValue v = parse("[]");
            check(v).beginArray().endArray();
            CHECK(v.toBool() == false);
            CHECK(v.toInt() == 0);
            CHECK(v.toDouble() == 0.0);
            CHECK(v.toString() == "");
        }
        SECTION("object") {
            const JSONValue v = parse("{}");
            check(v).beginObject().endObject();
            CHECK(v.toBool() == false);
            CHECK(v.toInt() == 0);
            CHECK(v.toDouble() == 0.0);
            CHECK(v.toString() == "");
        }
    }

    SECTION("in-place processing vs copying") {
        SECTION("in-place processing") {
            char json[] = "\"abc\"";
            const JSONValue v = JSONValue::parse(json, sizeof(json));
            json[2] = 'B';
            check(v).string("aBc");
        }
        SECTION("copying") {
            char json[] = "\"abc\"";
            const JSONValue v = JSONValue::parseCopy(json, sizeof(json));
            json[2] = 'B';
            check(v).string("abc"); // Source data is not affected
        }
        SECTION("implicit copying") {
            char json[] = "1x";
            const JSONValue v = JSONValue::parse(json, 1); // No space for term. null
            check(v).number(1);
            const char *s = v.toString().data();
            CHECK(s[1] == '\0'); // JSONString::data() returns null-terminated string
            CHECK(json[1] == 'x');
        }
    }
}

TEST_CASE("JSONString") {
    SECTION("construction") {
        JSONString s1; // Constructs empty string
        CHECK(s1 == "");
        CHECK(s1.size() == 0);
        CHECK(s1.isEmpty() == true);
        JSONValue v;
        JSONString s2(v); // Constructing from invalid JSONValue
        CHECK(s2 == "");
        CHECK(s2.size() == 0);
        CHECK(s2.isEmpty() == true);
    }

    SECTION("comparison") {
        const JSONString s1(parse("\"\""));
        const JSONString s2(parse("\"abc\""));
        CHECK(s1 == JSONString()); // JSONString
        CHECK(s1 == s1);
        CHECK(s2 == s2);
        CHECK(s1 != s2);
        CHECK((s1 == "" && "" == s1)); // const char*
        CHECK((s1 != "abc" && "abc" != s1));
        CHECK((s2 == "abc" && "abc" == s2));
        CHECK((s2 != "abcd" && "abcd" != s2));
        CHECK((s1 == String() && String() == s1)); // String
        CHECK((s1 != String("abc") && String("abc") != s1));
        CHECK((s2 == String("abc") && String("abc") == s2));
        CHECK((s2 != String("abcd") && String("abcd") != s2));
    }

    SECTION("casting") {
        const JSONString s1(parse("\"\""));
        const JSONString s2(parse("\"abc\""));
        CHECK(strcmp((const char*)s1, "") == 0); // const char*
        CHECK(strcmp((const char*)s2, "abc") == 0);
        CHECK((String)s1 == String()); // String
        CHECK((String)s2 == String("abc"));
    }
}

TEST_CASE("JSONArrayIterator") {
    SECTION("construction") {
        JSONArrayIterator it1;
        check(it1.value()).invalid();
        CHECK(it1.count() == 0);
        CHECK(it1.next() == false);
        const JSONValue v;
        JSONArrayIterator it2(v); // Constructing from invalid JSONValue
        check(it2.value()).invalid();
        CHECK(it2.count() == 0);
        CHECK(it2.next() == false);
    }
}

TEST_CASE("JSONObjectIterator") {
    SECTION("construction") {
        JSONObjectIterator it1;
        CHECK(it1.name() == "");
        CHECK(it1.value().isValid() == false);
        CHECK(it1.count() == 0);
        CHECK(it1.next() == false);
        const JSONValue v;
        JSONObjectIterator it2(v); // Constructing from invalid JSONValue
        CHECK(it2.name() == "");
        CHECK(it2.value().isValid() == false);
        CHECK(it2.count() == 0);
        CHECK(it2.next() == false);
    }
}

TEST_CASE("JSONStreamWriter") {
    SECTION("construction") {
        test::OutputStream strm;
        JSONStreamWriter w(strm);
        CHECK(w.stream() == &strm);
        check(strm).isEmpty();
    }
}

TEST_CASE("JSONBufferWriter") {
    SECTION("construction") {
        test::Buffer buf; // Empty buffer
        JSONBufferWriter w((char*)buf, buf.size());
        CHECK(w.buffer() == (char*)buf);
        CHECK(w.bufferSize() == buf.size());
        CHECK(w.dataSize() == 0);
    }

    SECTION("exact buffer size") {
        test::Buffer buf(25); // 25 bytes
        JSONBufferWriter w((char*)buf, buf.size());
        w.beginArray().nullValue().value(true).value(2).value(3.14).value("abcd").endArray();
        CHECK(w.dataSize() == 25);
        check(buf).equals("[null,true,2,3.14,\"abcd\"]");
        CHECK(buf.isPaddingValid());
    }

    SECTION("too small buffer") {
        test::Buffer buf;
        JSONBufferWriter w((char*)buf, buf.size());
        w.beginArray().nullValue().value(true).value(2).value(3.14).value("abcd").endArray();
        CHECK(w.dataSize() == 25); // Size of the actual JSON data
        CHECK(buf.isPaddingValid());
    }
}
