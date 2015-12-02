#include "catch.hpp"
#include "spark_wiring_random.h"

#include <cstring>
#include <array>

template <std::size_t size> void fillBuf(std::array<int, size>& buf, int min, int max) {
    int count = size;
    while (count-->0) {
        buf[count] = random(min, max);
    }
}

SCENARIO("Random values consistent with the same seed", "[random]") {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    WHEN("Filling both buffers with the same seeded random sequence") {
        randomSeed(42);
        fillBuf(buf1, 20, 30);

        randomSeed(42);
        fillBuf(buf2, 20, 30);

        THEN("Both buffers should contain the same values") {
            REQUIRE(buf1==buf2);
        }
    }
}


SCENARIO("Random values different with different seeds", "[random]") {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    WHEN("Filling both buffers with the same seeded random sequence") {
        randomSeed(42);
        fillBuf(buf1, 20, 30);

        randomSeed(43);
        fillBuf(buf2, 20, 30);
    }
    THEN("Both buffers should contain the same values") {
        REQUIRE(!(buf1 == buf2));
    }
}

SCENARIO("random(0) returns zero") {
    REQUIRE(random(0)==0);
}

SCENARIO("min >= max returns min") {
    REQUIRE(random(10, 5)==10);
    REQUIRE(random(20, 20)==20);
}

SCENARIO("random of a closed range returns the same value") {
    CHECK(random(10,10)==10);
    CHECK(random(11,11)==11);
    CHECK(random(-3,-3)==-3);
}

bool assertRandomRange(int min, int max, int iterations) {
    bool ok = true;
    while (iterations-->0) {
        int r = random(min, max);
        CHECK(r >= min);
        CHECK(r < max);
    }
    return ok;
}

SCENARIO("Positive range values are within range") {
    REQUIRE(assertRandomRange(20,30, 1000));
}

SCENARIO("Negative range values are within range") {
    REQUIRE(assertRandomRange(-30,-20, 1000));
}

SCENARIO("Zero-crossing range values are within range") {
    REQUIRE(assertRandomRange(-10,10, 1000));
}
