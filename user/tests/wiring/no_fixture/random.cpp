#include "spark_wiring_constants.h"
#include "application.h"
#include "unit-test/unit-test.h"
#include <array>
#include <stdlib.h>

bool rand_value_set = false;

void random_seed_from_cloud(unsigned int value)
{
	rand_value_set = true;
}

test(RANDOM_01_value_set_from_cloud)
{
	assertTrue(rand_value_set);
}

template <std::size_t size> void fillBuf(std::array<int, size>& buf, int min, int max) {
    int count = size;
    while (count-->0) {
        buf[count] = random(min, max);
    }
}

test(RANDOM_02_values_consistent_with_the_same_seed) {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    randomSeed(42);
    fillBuf(buf1, 20, 30);

    randomSeed(42);
    fillBuf(buf2, 20, 30);

    assertTrue(buf1==buf2);
}


test(RANDOM_03_values_different_with_different_seeds) {
    std::array<int, 100> buf1;
    std::array<int, 100> buf2;
    randomSeed(42);
    fillBuf(buf1, 20, 30);

    randomSeed(43);
    fillBuf(buf2, 20, 30);
    assertFalse(buf1==buf2);
}

test(RANDOM_04_zero_returns_zero) {
    assertEqual(random(0), 0);
}

test(RANDOM_05_empty_range_returns_min) {
    assertEqual(random(10, 5),10);
    assertEqual(random(20, 20),20);
}

test(RANDOM_06_closed_range_returns_same_value) {
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

test(RANDOM_07_positive_range_values_are_within_range) {
    assertRandomRange(20,30, 1000);
}

test(RANDOM_08_negative_range_values_are_within_range) {
    assertRandomRange(-30,-20, 1000);
}

test(RANDOM_09_zero_crossing_range_values_are_within_range) {
    assertRandomRange(-10,10, 1000);
}

#if defined(PARTICLE_PROTOCOL) && PARTICLE_PROTOCOL == 0

test(RANDOM_10_reseeded_from_cloud_on_reconnect) {
    rand_value_set = false;
    Particle.disconnect();
    waitFor(Particle.disconnected, 60000);
    Particle.connect();
    waitFor(Particle.connected, 60000);
    delay(1000);
    assertTrue(rand_value_set);
}

#endif // defined(PARTICLE_PROTOCOL) && PARTICLE_PROTOCOL == 0