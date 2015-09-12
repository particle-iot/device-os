
#include "testapi.h"

test(spi_clock)
{
    API_COMPILE(SPI.setClockSpeed(24, MHZ));
    API_COMPILE(SPI.setClockSpeed(24000000));
    API_COMPILE(SPI.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI.setClockDividerReference(SPI_CLK_ARDUINO));

}
