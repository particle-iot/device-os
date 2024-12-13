
#include <iostream>
#include <limits.h>
#include <cstring>
#include "util/catch.h"

#include "spark_wiring_string.h"

TEST_CASE("Can use HEX radix with String numeric conversion constructors") {

    REQUIRE(!strcmp(String(32, HEX),"20"));
}

TEST_CASE("Can use DEC radix with String numeric conversion constructors") {
    REQUIRE(String(32,DEC)=="32");
}

TEST_CASE("Can use BIN radix with String numeric conversion constructors") {
    REQUIRE(String(32, BIN)=="100000");
}

TEST_CASE("Can print INT_MIN value in decimal") {
    REQUIRE(String(INT_MIN, DEC)=="-2147483648");
}

TEST_CASE("Can print INT_MAX value in decimal") {
    REQUIRE(String(INT_MAX, DEC)=="2147483647");
}

TEST_CASE("Can print UINT_MAX in decimal") {
    REQUIRE(String(UINT_MAX, DEC)=="4294967295");
}

TEST_CASE("Can print INT64_MAX/MIN value in decimal") {
    // Must make an explicit `long long` cast for x86_64 architectures that
    // use `long` for 64-bit numbers.
    REQUIRE(String((long long)INT64_MAX, DEC)=="9223372036854775807");
    REQUIRE(String((long long)INT64_MIN, DEC)=="-9223372036854775808");
}

TEST_CASE("Can print UINT64_MAX in decimal") {
    // Must make an explicit `unsigned long long` cast for x86_64 architectures that
    // use `unsigned long` for 64-bit numbers.
    REQUIRE(String((unsigned long long)UINT64_MAX, DEC)=="18446744073709551615");
}

TEST_CASE("Can print INT_MAX value in binary") {
    REQUIRE(String(INT_MAX, BIN)=="1111111111111111111111111111111");
}

TEST_CASE("Can print INT_MIN value in binary") {
    REQUIRE(String(INT_MIN, BIN)=="-10000000000000000000000000000000");
}

TEST_CASE("Can print UINT_MAX in binary") {
    REQUIRE(String(UINT_MAX, BIN)=="11111111111111111111111111111111");
}

TEST_CASE("Can print INT64_MAX/MIN value in binary") {
    // Must make an explicit `long long` cast for x86_64 architectures that
    // use `long` for 64-bit numbers.
    REQUIRE(String((long long)INT64_MAX, BIN)=="111111111111111111111111111111111111111111111111111111111111111");
    REQUIRE(String((long long)INT64_MIN, BIN)=="-1000000000000000000000000000000000000000000000000000000000000000");
}

TEST_CASE("Can print UINT64_MAX in binary") {
    // Must make an explicit `unsigned long long` cast for x86_64 architectures that
    // use `unsigned long` for 64-bit numbers.
    REQUIRE(String((unsigned long long)UINT64_MAX, BIN)=="1111111111111111111111111111111111111111111111111111111111111111");
}

TEST_CASE("Can convert float to string with default precision of 6") {
    REQUIRE(String(1.0)=="1.000000");
}

TEST_CASE("Can convert long float to string with default precision of 6") {
    REQUIRE(String(123456789.0,3)=="123456789.000");
}

TEST_CASE("Can convert negative float to string") {
    REQUIRE(String(-123.123, 3)=="-123.123");
}

TEST_CASE("Can convert float no decimals rounding up") {
    REQUIRE(String(123.5, 0)=="124");
}

TEST_CASE("Can convert float no decimals rounding down") {
    REQUIRE(String(123.2, 0)=="123");
}

TEST_CASE("Can convert negative float no decimals rounding down") {
    REQUIRE(String(-123.5, 0)=="-124");
}

TEST_CASE("Can convert negative float no decimals rounding up") {
    REQUIRE(String(-123.2, 0)=="-123");
}

TEST_CASE("Can format a string using printf like syntax") {
    REQUIRE(String::format("%d %s %s please", 3, "lemon", "curries")==String("3 lemon curries please"));
}

TEST_CASE("Can convert a string to lowercase") {
    REQUIRE(String("In LOWERCAse").toLowerCase()==String("in lowercase"));
}

TEST_CASE("Substring allows selecting with left only") {
    REQUIRE(String("test1234").substring(4)==String("1234"));
}

TEST_CASE("Substring with left after the end returns empty string") {
    REQUIRE(String("test123").substring(99)==String());
}

TEST_CASE("Substring allows selecting with left and right") {
    REQUIRE(String("test123").substring(3, 5)==String("t1"));
}

TEST_CASE("Substring with left after the end and right returns empty string") {
    REQUIRE(String("test123").substring(99, 100)==String(""));
}

TEST_CASE("Substring with left and right after the end returns until the end of string") {
    REQUIRE(String("test123").substring(4, 99)==String("123"));
}

TEST_CASE("Substring with flipped left and right returns the correct substring") {
    REQUIRE(String("test123").substring(5, 3)==String("t1"));
}

TEST_CASE("String operator +=", "[Particle::String]") {

    SECTION("Appending char array") {
        String str1;
        str1 += "World!";
        REQUIRE(str1 == "World!");
    }

    SECTION("Appending another String object") {
        String str1("Hello");
        String str2(", World!");
        str1 += str2;
        REQUIRE(str1 == "Hello, World!");
    }

    SECTION("Appending long long") {
        String str1("n=");
        str1 += (long long)9223372036854775807;
        REQUIRE(str1 == "n=9223372036854775807");
    }

    SECTION("Appending unsigned long long") {
        String str1("n=");
        str1 += (unsigned long long)18446744073709551615ULL;
        REQUIRE(str1 == "n=18446744073709551615");
    }
}

TEST_CASE("String concat", "[ParticleString]") {

    SECTION("Concatenating char array") {
        String str1("Hello");
        str1.concat(", World!");
        REQUIRE(str1 == "Hello, World!");
    }

    SECTION("Concatenating another String object") {
        String str1("Hello");
        String str2(", World!");
        str1.concat(str2);
        REQUIRE(str1 == "Hello, World!");
    }

    // TODO: Use of int type causes problems on x86_64, fine on ARM
    // SECTION("Concatenating int") {
    //     String str1("Hello ");
    //     str1.concat((int)-2147483647);
    //     REQUIRE(str1 == "Hello -2147483647");
    // }

    // TODO: Use of unsigned int type causes problems on x86_64, fine on ARM
    // SECTION("Concatenating unsigned int") {
    //     String str1("Hello ");
    //     str1.concat((unsigned int)4294967295);
    //     REQUIRE(str1 == "Hello 4294967295");
    // }

    SECTION("Concatenating long") {
        String str1("Hello ");
        str1.concat((long)-2147483647L);
        REQUIRE(str1 == "Hello -2147483647");
    }

    SECTION("Concatenating unsigned long") {
        String str1("Hello ");
        str1.concat((unsigned long)4294967295UL);
        REQUIRE(str1 == "Hello 4294967295");
    }

    SECTION("Concatenating long long") {
        String str1("Hello ");
        str1.concat((long long)-9223372036854775807);
        REQUIRE(str1 == "Hello -9223372036854775807");
    }

    SECTION("Concatenating unsigned long long") {
        String str1("Hello ");
        str1.concat((unsigned long long)18446744073709551615ULL);
        REQUIRE(str1 == "Hello 18446744073709551615");
    }
}

TEST_CASE("String resize") {
    SECTION("can increase the string length") {
        String s;
        CHECK(s.resize(5));
        CHECK(s.capacity() == 5);
        CHECK(s.length() == 5);
        CHECK(std::memcmp(s.c_str(), "\0\0\0\0\0\0", 6) == 0);
        s.setCharAt(4, 'a');
        CHECK(s.resize(6));
        CHECK(s.capacity() == 6);
        CHECK(s.length() == 6);
        CHECK(std::memcmp(s.c_str(), "\0\0\0\0a\0\0", 7) == 0);
    }

    SECTION("can decrease the string length") {
        String s("abcde");
        CHECK(s.resize(4));
        CHECK(s.capacity() == 5);
        CHECK(s.length() == 4);
        CHECK(std::memcmp(s.c_str(), "abcd\0", 5) == 0);
        CHECK(s.resize(0));
        CHECK(s.capacity() == 5);
        CHECK(s.length() == 0);
        CHECK(std::memcmp(s.c_str(), "\0", 1) == 0);
    }
}

TEST_CASE("Comparison operators") {
    SECTION("operator==") {
        CHECK(String("") == String(""));
        CHECK(String("") == "");
        CHECK("" == String(""));
        CHECK(String("abc") == String("abc"));
        CHECK(String("abc") == "abc");
        CHECK("abc" == String("abc"));
        CHECK(!(String("abc") == String("Abc")));
    }

    SECTION("operator!=") {
        CHECK(String("") != String("Abc"));
        CHECK(String("") != "Abc");
        CHECK("" != String("Abc"));
        CHECK(String("abc") != String("Abc"));
        CHECK(String("abc") != "Abc");
        CHECK("abc" != String("Abc"));
        CHECK(!(String("abc") != String("abc")));
    }

    SECTION("operator<") {
        CHECK(String("") < String("abc"));
        CHECK(String("") < "abc");
        CHECK("" < String("abc"));
        CHECK(String("abc") < String("bbc"));
        CHECK(String("abc") < "bbc");
        CHECK("abc" < String("bbc"));
        CHECK(!(String("") < String("")));
        CHECK(!(String("abc") < String("abc")));
        CHECK(!(String("bbc") < String("abc")));
    }

    SECTION("operator<=") {
        CHECK(String("") <= String(""));
        CHECK(String("") <= String("abc"));
        CHECK(String("") <= "abc");
        CHECK("" <= String("abc"));
        CHECK(String("abc") <= String("abc"));
        CHECK(String("abc") <= String("bbc"));
        CHECK(String("abc") <= "bbc");
        CHECK("abc" <= String("bbc"));
        CHECK(!(String("bbc") <= String("abc")));
    }

    SECTION("operator>") {
        CHECK(String("abc") > String(""));
        CHECK("abc" > String(""));
        CHECK(String("abc") > "");
        CHECK(String("bbc") > String("abc"));
        CHECK("bbc" > String("abc"));
        CHECK(String("bbc") > "abc");
        CHECK(!(String("") > String("")));
        CHECK(!(String("abc") > String("abc")));
        CHECK(!(String("abc") > String("bbc")));
    }

    SECTION("operator>=") {
        CHECK(String("") >= String(""));
        CHECK(String("abc") >= String(""));
        CHECK("abc" >= String(""));
        CHECK(String("abc") >= "");
        CHECK(String("abc") >= String("abc"));
        CHECK(String("bbc") >= String("abc"));
        CHECK("bbc" >= String("abc"));
        CHECK(String("bbc") >= "abc");
        CHECK(!(String("abc") >= String("bbc")));
    }
}
