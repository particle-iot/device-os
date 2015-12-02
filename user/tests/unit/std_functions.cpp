
#include "catch.hpp"

#include "spark_wiring_constants.h"

TEST_CASE("Can use min() in main namespace in an application") {
    REQUIRE(min(10l, 5)==5);
}

TEST_CASE("Can use max() in main namespace in an application") {
    REQUIRE(min(10.0, 5)==5.0);
}

TEST_CASE("Can use round() in main namespace in an application") {
    REQUIRE(round(10.7)==11);
}
