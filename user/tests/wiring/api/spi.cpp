
#include "testapi.h"

test(spi_clock)
{
    API_COMPILE(SPI.setClockSpeed(24, MHZ));
    API_COMPILE(SPI.setClockSpeed(24000000));
    API_COMPILE(SPI.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI.setClockDividerReference(SPI_CLK_ARDUINO));

}

test(spi_transfer)
{
    API_COMPILE(SPI.transfer(0));
    API_COMPILE(SPI.transfer(NULL, NULL, 1, NULL));
    API_COMPILE(SPI.transferCancel());
}

// without Arduino.h we should not get a clash redefining SPISettings
class SPISettings {};
