
#include "application.h"
#include "unit-test/unit-test.h"

void assertClockDivider(unsigned reference, unsigned desired, uint8_t expected_divider, unsigned expected_clock)
{
    unsigned clock;
    uint8_t divider;

    SPI.computeClockDivider(reference, desired, divider, clock);
    assertEqual(expected_divider, divider);
    assertEqual(expected_clock, clock);
}

test(spi_computeClockSpeed)
{
    assertClockDivider(60*MHZ, 120*MHZ, SPI_CLOCK_DIV2, 30*MHZ);
    assertClockDivider(60*MHZ, 30*MHZ, SPI_CLOCK_DIV2, 30*MHZ);
    assertClockDivider(60*MHZ, 20*MHZ, SPI_CLOCK_DIV4, 15*MHZ);
    assertClockDivider(60*MHZ, 8*MHZ, SPI_CLOCK_DIV8, 7500*KHZ);
    assertClockDivider(60*MHZ, 7*MHZ, SPI_CLOCK_DIV16, 3750*KHZ);
    assertClockDivider(60*MHZ, 300*KHZ, SPI_CLOCK_DIV256, 234375*HZ);
    assertClockDivider(60*MHZ, 1*KHZ, SPI_CLOCK_DIV256, 234375*HZ);
}