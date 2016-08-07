
#include "application.h"
#include "unit-test/unit-test.h"

static volatile uint8_t DMA_Completed_Flag = 0;

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

void assertClockDivider(unsigned reference, unsigned desired, uint8_t expected_divider, unsigned expected_clock)
{
    unsigned clock;
    uint8_t divider;

    SPI.computeClockDivider(reference, desired, divider, clock);
    assertEqual(expected_divider, divider);
    assertEqual(expected_clock, clock);
}

test(SPI_01_computeClockSpeed)
{
    assertClockDivider(60*MHZ, 120*MHZ, SPI_CLOCK_DIV2, 30*MHZ);
    assertClockDivider(60*MHZ, 30*MHZ, SPI_CLOCK_DIV2, 30*MHZ);
    assertClockDivider(60*MHZ, 20*MHZ, SPI_CLOCK_DIV4, 15*MHZ);
    assertClockDivider(60*MHZ, 8*MHZ, SPI_CLOCK_DIV8, 7500*KHZ);
    assertClockDivider(60*MHZ, 7*MHZ, SPI_CLOCK_DIV16, 3750*KHZ);
    assertClockDivider(60*MHZ, 300*KHZ, SPI_CLOCK_DIV256, 234375*HZ);
    assertClockDivider(60*MHZ, 1*KHZ, SPI_CLOCK_DIV256, 234375*HZ);
}

test(SPI_02_SPI_DMA_Transfers_Work_Correctly)
{
    SPI.begin();
    uint8_t tempBuf[256];
    uint8_t tempBuf1[256];
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI.transfer(tempBuf, 0, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI.transfer(0, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI.transfer(tempBuf1, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    SPI.end();
}

#if Wiring_SPI1
test(SPI_03_SPI1_DMA_Transfers_Work_Correctly)
{
    SPI1.begin();
    uint8_t tempBuf[256];
    uint8_t tempBuf1[256];
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI1.transfer(tempBuf, 0, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI1.transfer(0, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI1.transfer(tempBuf1, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    SPI1.end();
}
#endif

#if Wiring_SPI2
test(SPI_04_SPI2_DMA_Transfers_Work_Correctly)
{
    SPI2.begin();
    uint8_t tempBuf[256];
    uint8_t tempBuf1[256];
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI2.transfer(tempBuf, 0, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI2.transfer(0, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    memset(tempBuf, 0xAA, sizeof(tempBuf));
    DMA_Completed_Flag = 0;
    SPI2.transfer(tempBuf1, tempBuf, sizeof(tempBuf), SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (const auto& v : tempBuf) {
        assertNotEqual(v, 0xAA);
    }

    SPI2.end();
}
#endif
