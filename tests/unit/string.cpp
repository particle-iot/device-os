
#include <iostream>
#include <limits.h>
#include "catch.hpp"

#include "spark_wiring_string.h"

TEST_CASE("Can use HEX radix with String numeric conversion constructors") {
    
    REQUIRE(String(32, HEX)=="20");
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

// add String printing function 
namespace Catch {

    std::string toString( String const& value ) {
        return std::string(value.c_str());
    }

}