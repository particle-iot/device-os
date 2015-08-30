
#include "testapi.h"
#include "spark_wiring_spi.h"

test(SPI_clock)
{
    API_COMPILE(SPI.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI.setClockDivider(SPI_CLOCK_DIV256));
    API_COMPILE(SPI.setClockSpeed(100, MHZ));
    API_COMPILE(SPI.setClockSpeed(100, KHZ));
    API_COMPILE(SPI.setClockSpeed(ARDUINO));
    API_COMPILE(SPI.setClockSpeed(ARDUINO/4));
    API_COMPILE(SPI.setClockDividerReference(ARDUINO));
}

#if Wiring_SPI1
test(SPI1_begin)
{
    API_COMPILE(SPI1.begin());
}

test(SPI1_clock)
{
    API_COMPILE(SPI1.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI1.setClockDivider(SPI_CLOCK_DIV256));
    API_COMPILE(SPI1.setClockSpeed(100, MHZ));
    API_COMPILE(SPI1.setClockSpeed(100, KHZ));
    API_COMPILE(SPI1.setClockSpeed(ARDUINO));
    API_COMPILE(SPI1.setClockSpeed(ARDUINO/4));
    API_COMPILE(SPI1.setClockDividerReference(ARDUINO));
}
#endif
