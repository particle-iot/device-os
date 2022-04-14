
#include "util/catch.h"
#include "spark_wiring_print.h"
#include <string>

class BufferPrint : public Print
{
    std::string value;

public:
    size_t write(uint8_t c)
    {
        value += (char)c;
        return 1;
    }

    const std::string& result()
    {
        return value;
    }
};


SCENARIO("Print.printf() with a small string", "[print]")
{
    BufferPrint print;
    print.printf("abcd %d", 10);
    REQUIRE("abcd 10" == print.result());
}

SCENARIO("Print.printlnf() with a small string", "[print]")
{
    BufferPrint print;
    print.printlnf("abcd %d", 10);
    REQUIRE("abcd 10\r\n" == print.result());
}


SCENARIO("Print.printf() with a 19-char string", "[print]")
{
    BufferPrint print;
    print.printf("abcdabcdabcdabcd %d", 10);
    REQUIRE("abcdabcdabcdabcd 10" == print.result());
}

SCENARIO("Print.printf() with a 20-char string", "[print]")
{
    BufferPrint print;
    print.printf("abcdabcdabcdabcd %d", 100);
    REQUIRE("abcdabcdabcdabcd 100" == print.result());
}

SCENARIO("Print.printf() with a a > 20 char string", "[print]")
{
    BufferPrint print;
    print.printf("abcdabcdabcdabcd %d xyzxyzxyzxyzxyzxyzxyzxyz", 100);
    REQUIRE("abcdabcdabcdabcd 100 xyzxyzxyzxyzxyzxyzxyzxyz" == print.result());
}
