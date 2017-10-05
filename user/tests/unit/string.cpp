
#include <iostream>
#include <limits.h>
#include "catch.hpp"

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

TEST_CASE("Can print INT_MAX value in binary") {
    REQUIRE(String(INT_MAX, BIN)=="1111111111111111111111111111111");
}

TEST_CASE("Can print INT_MIN value in binary") {
    REQUIRE(String(INT_MIN, BIN)=="-10000000000000000000000000000000");
}

TEST_CASE("Can print UINT_MAX in binary") {
    REQUIRE(String(UINT_MAX, BIN)=="11111111111111111111111111111111");
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

TEST_CASE("Can split a string") {
    auto parts = String("setting=123").split("=");
    REQUIRE(parts.hasNext()==true);
    REQUIRE(parts.next()==String("setting"));
    REQUIRE(parts.hasNext()==true);
    REQUIRE(parts.next()==String("123"));
    REQUIRE(parts.hasNext()==false);
    REQUIRE(parts.next()==String(""));
}

TEST_CASE("When delimiter is not found split returns the whole string") {
    auto parts = String("fullstring").split(",");
    REQUIRE(parts.hasNext()==true);
    REQUIRE(parts.next()==String("fullstring"));
    REQUIRE(parts.hasNext()==false);
    REQUIRE(parts.next()==String(""));
}

TEST_CASE("When string is empty split returns no parts") {
    auto parts = String().split(",");
    REQUIRE(parts.hasNext()==false);
    REQUIRE(parts.next()==String(""));
}

TEST_CASE("Split does not modify the original string") {
    String str("setting=123");
    auto parts = str.split("=");
    parts.next();
    parts.next();
    REQUIRE(str==String("setting=123"));
}

TEST_CASE("Split is reentrant") {
    String str("first=1,second=2");
    String allNames;
    String allValues;

    auto settings = str.split(",");
    while (settings.hasNext()) {
        String setting = settings.next();
        auto parts = setting.split("=");
        String name = parts.next();
        String value = parts.next();

        allNames += name;
        allValues += value;
    }

    REQUIRE(allNames=="firstsecond");
    REQUIRE(allValues=="12");
}
