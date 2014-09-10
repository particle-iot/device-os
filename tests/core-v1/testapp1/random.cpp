#include <stdlib.h>
#include <array>
#include "application.h"
#include "unit-test/unit-test.h"

// the headers have to be declared in this order, or the #define min() in the spark wiring clashes with std::min)

template <std::size_t size> void fillBuf(std::array<int, size>& buf, int min, int max) {
    int count = size;
    while (count-->0) {
        buf[count] = random(min, max);
    }
}

test(Random_values_consistent_with_the_same_seed) {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    randomSeed(42);
    fillBuf(buf1, 20, 30);

    randomSeed(42);
    fillBuf(buf2, 20, 30);

    assertTrue(buf1==buf2);
}


test(Random_values_different_with_different_seeds) {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    randomSeed(42);
    fillBuf(buf1, 20, 30);

    randomSeed(43);
    fillBuf(buf2, 20, 30);
    assertFalse(buf1==buf2);    
}

test(random_zero_returns_zero) {
    assertEqual(random(0), 0);
}

test(empty_range_returns_min) {
    assertEqual(random(10, 5),10);
    assertEqual(random(20, 20),20);
}

test(closed_range_returns_same_value) {
    assertEqual(random(10,10),10);
    assertEqual(random(11,11),11);
    assertEqual(random(-3,-3),-3);
}

void assertRandomRange(int min, int max, int iterations) {    
    while (iterations-->0) {
        int r = random(min, max);
        assertMoreOrEqual(r,  min);
        assertLess(r, max);
    }        
}

test(Positive_range_values_are_within_range) {
    assertRandomRange(20,30, 1000);
}

test(Negative_range_values_are_within_range) {
    assertRandomRange(-30,-20, 1000);
}

test(ZeroCrossing_range_values_are_within_range) {
    assertRandomRange(-10,10, 1000);
}
