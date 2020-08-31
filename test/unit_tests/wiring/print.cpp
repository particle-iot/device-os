#include "spark_wiring_print.h"

#include "catch2/catch.hpp"

namespace {

using namespace particle;

enum TestEnum {
    TEST_ENUM_VALUE = 123,
};

class PrintTest : public Print {
public:
    PrintTest()
            : data_{},
              size_{0} {
    }
    static constexpr const size_t MAX_DATA_BUFFER_SIZE = 200;
    size_t write(const uint8_t* buffer, size_t size) override {
        if (size_ + size >= MAX_DATA_BUFFER_SIZE) {
            return 0;
        }
        memcpy(&data_[size_], buffer, size);
        size_ += size;

        return size;
    }

    size_t write(uint8_t data) {
        return write(&data, 1);
    }

    bool isEqual(const void* buffer) {
        bool ret = memcmp(data_, buffer, strlen((char*)buffer)) == 0;
        clear();
        return ret;
    }

    void clear() {
        memset(data_, 0, size_);
        size_ = 0;
    }

private:
    char data_[MAX_DATA_BUFFER_SIZE];
    size_t  size_;
};

class TestConvertiblePrintable: public Printable {
public:
    virtual size_t printTo(Print& p) const override {
        return p.write("TestConvertiblePrintable");
    }

    operator bool() const {
        return true;
    }
};

} // namespace

TEST_CASE("Print tests for all types") {
    PrintTest printTest;

    SECTION("Print with different bases") {
        char input = '1'; //0x31
        REQUIRE(printTest.print(input));
        REQUIRE(printTest.isEqual("1"));
        REQUIRE(printTest.print(input, HEX));
        REQUIRE(printTest.isEqual("31"));
        REQUIRE(printTest.print(input, DEC));
        REQUIRE(printTest.isEqual("49"));
        REQUIRE(printTest.print(input, OCT));
        REQUIRE(printTest.isEqual("61"));
        REQUIRE(printTest.print(input, BIN));
        REQUIRE(printTest.isEqual("110001"));
    }

    SECTION("Print signed and unsigned type") {
        int8_t signedCharInput = 127;
        REQUIRE(printTest.print(signedCharInput));
        REQUIRE(printTest.isEqual("127"));
        uint8_t unsignedCharInput = 255;
        REQUIRE(printTest.print(unsignedCharInput));
        REQUIRE(printTest.isEqual("255"));
        signed short signedShortInput = 32767;
        REQUIRE(printTest.print(signedShortInput));
        REQUIRE(printTest.isEqual("32767"));
        unsigned short unsignedShortInput = 65535;
        REQUIRE(printTest.print(unsignedShortInput));
        REQUIRE(printTest.isEqual("65535"));
        int intInput = 2147483647;
        REQUIRE(printTest.print(intInput));
        REQUIRE(printTest.isEqual("2147483647"));
        unsigned int unsignedIntInput = 4294967295U;
        REQUIRE(printTest.print(unsignedIntInput));
        REQUIRE(printTest.isEqual("4294967295"));
        long long longLongInput = 9223372036854775807LL;
        REQUIRE(printTest.print(longLongInput));
        REQUIRE(printTest.isEqual("9223372036854775807"));
        unsigned long long unsignedLongLongInput = 18446744073709551615ULL;
        REQUIRE(printTest.print(unsignedLongLongInput));
        REQUIRE(printTest.isEqual("18446744073709551615"));
    }

    SECTION("Print negative numbers") {
        int8_t signedCharInput = -128;
        REQUIRE(printTest.print(signedCharInput));
        REQUIRE(printTest.isEqual("-128"));
        int16_t signedShortInput = -32768;
        REQUIRE(printTest.print(signedShortInput));
        REQUIRE(printTest.isEqual("-32768"));
        int32_t intInput = -2147483647;
        REQUIRE(printTest.print(intInput));
        REQUIRE(printTest.isEqual("-2147483647"));
        int64_t longLongInput = -9223372036854775807L;
        REQUIRE(printTest.print(longLongInput));
        REQUIRE(printTest.isEqual("-9223372036854775807"));
    }

    SECTION("Print bool type") {
        REQUIRE(printTest.print(true));
        REQUIRE(printTest.isEqual("1"));
        REQUIRE(printTest.print(false));
        REQUIRE(printTest.isEqual("0"));
    }

    SECTION("Print float and double") {
        REQUIRE(printTest.print(1.23));
        REQUIRE(printTest.isEqual("1.23"));
        REQUIRE(printTest.print((float)1.23));
        REQUIRE(printTest.isEqual("1.23"));
        REQUIRE(printTest.print((double)1.23));
        REQUIRE(printTest.isEqual("1.23"));
    }

    SECTION("Print Enum") {
        REQUIRE(printTest.print(TEST_ENUM_VALUE));
        REQUIRE(printTest.isEqual("123"));
    }

    SECTION("Print others") {
        REQUIRE(printTest.print("") == 0);
        REQUIRE(printTest.isEqual(""));
    }

    SECTION("Printable objects that are implicitly convertible to integra/unsigned integer types") {
        TestConvertiblePrintable printable;
        REQUIRE(printTest.print(printable));
        REQUIRE(printTest.isEqual("TestConvertiblePrintable"));
        REQUIRE(printTest.println(printable));
        REQUIRE(printTest.isEqual("TestConvertiblePrintable\r\n"));
    }

}
