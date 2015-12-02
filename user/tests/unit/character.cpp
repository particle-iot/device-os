
#include "catch.hpp"

#include "spark_wiring_character.h"

// smoke tests for character APIs

TEST_CASE("isAlpha") {
    CHECK(isAlpha('a'));
    CHECK(isAlpha('Z'));
    CHECK(!isAlpha('@'));
}

TEST_CASE("isAlphaNumeric") {
    CHECK(isAlphaNumeric('a'));
    CHECK(isAlphaNumeric('Z'));
    CHECK(!isAlphaNumeric('@'));
    CHECK(isAlphaNumeric('5'));
}

TEST_CASE("isAscii") {
    CHECK(isAscii('a'));
    CHECK(isAscii('Z'));
    CHECK(isAscii('~'));
    CHECK(!isAscii(0x80));
}

TEST_CASE("isControl") {
    for (int i=0; i<0x1f; i++) {
        CHECK(isControl(i));
    }
    CHECK(!isControl('r'));
    CHECK(!isControl('F'));
}

TEST_CASE("isDigit") {
    for (int i=0; i<256; i++) {
        CHECK(isDigit(i)== (i>='0' && i<='9'));
    }
}

TEST_CASE("isGraph") {
    CHECK(!isGraph(' '));
    CHECK(isGraph('G'));
}

TEST_CASE("isHexadecimalDigit") {
    CHECK(isHexadecimalDigit('A'));
    CHECK(isHexadecimalDigit('f'));
    CHECK(!isHexadecimalDigit('G'));
    CHECK(isHexadecimalDigit('9'));
}

TEST_CASE("isLowerCase") {
    CHECK(isLowerCase('l'));
    CHECK(!isLowerCase('L'));
    CHECK(!isLowerCase('5'));
}

TEST_CASE("isPrintable") {
    CHECK(!isPrintable(0x1f));
    CHECK(isPrintable(' '));
    CHECK(isPrintable('q'));
}

TEST_CASE("isPunct") {
    CHECK(isPunct('.'));
    CHECK(!isPunct(' '));
    CHECK(isPunct('#'));
    CHECK(isPunct('$'));
}

TEST_CASE("isSpace") {
    CHECK(isSpace(' '));
    CHECK(isSpace('\t'));
    CHECK(isSpace('\n'));
    CHECK(!isSpace('_'));
}

TEST_CASE("isUpperCase") {
    CHECK(isUpperCase('A'));
    CHECK(!isUpperCase('5'));
    CHECK(!isUpperCase('f'));
}

TEST_CASE("isWhitespace") {
    CHECK(isWhitespace(' '));
    CHECK(isWhitespace('\t'));
    CHECK(!isWhitespace('\n'));
    CHECK(!isWhitespace('_'));
    CHECK(!isWhitespace('0'));
}

TEST_CASE("toLowerCase") {
    CHECK(toLowerCase('A')=='a');
    CHECK(toLowerCase('Z')=='z');
    CHECK(toLowerCase('6')=='6');
}

TEST_CASE("toUpperCase") {
    CHECK(toUpperCase('a')=='A');
    CHECK(toUpperCase('z')=='Z');
    CHECK(toUpperCase('6')=='6');
}
