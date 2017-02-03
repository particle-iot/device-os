#include "spark_wiring_error.h"

#include "tools/catch.h"

using namespace particle;

TEST_CASE("Error") {
    SECTION("Error()") {
        Error e;
        CHECK(e.type() == Error::UNKNOWN);
        CHECK(strcmp(e.message(), "Unknown error") == 0);
    }
    SECTION("Error(type)") {
        Error e(Error::INTERNAL);
        CHECK(e.type() == Error::INTERNAL);
        CHECK(strcmp(e.message(), "Internal error") == 0);
    }
    SECTION("Error(type, msg)") {
        // Error message is copied
        char msg[] = "abc";
        Error e1(Error::INTERNAL, (const char*)msg);
        msg[0] = 'A';
        CHECK(e1.type() == Error::INTERNAL);
        CHECK(strcmp(e1.message(), "abc") == 0);
        // Using nullptr results in a default message
        Error e2(Error::INTERNAL, nullptr);
        CHECK(strcmp(e2.message(), "Internal error") == 0);
    }
    SECTION("Error(error)") {
        // Copy constructor
        Error e1(Error::INTERNAL, "abc");
        Error e2(e1);
        CHECK(e2.type() == Error::INTERNAL);
        CHECK(strcmp(e2.message(), "abc") == 0);
        // Move constructor
        Error e3(std::move(e1));
        CHECK(e3.type() == Error::INTERNAL);
        CHECK(strcmp(e3.message(), "abc") == 0);
        CHECK(e1.type() == Error::UNKNOWN);
        CHECK(strcmp(e1.message(), "Unknown error") == 0);
    }
    SECTION("operator==(error)") {
        Error e1(Error::UNKNOWN, "e1");
        Error e2(Error::UNKNOWN, "e2");
        Error e3(Error::INTERNAL, "e3");
        Error e4(Error::INTERNAL, "e4");
        CHECK(e1 == e2); // Error message is ignored
        CHECK(e1 != e3);
        CHECK(e3 == e4); // ditto
        CHECK(e3 != e2);
    }
    SECTION("operator==(type)") {
        Error e1(Error::UNKNOWN);
        Error e2(Error::INTERNAL);
        CHECK(e1 == Error::UNKNOWN);
        CHECK(e1 != Error::INTERNAL);
        CHECK(e2 == Error::INTERNAL);
        CHECK(e2 != Error::UNKNOWN);
    }
}
