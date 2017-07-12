
#include "application.h"
#include "unit-test/unit-test.h"

#define SPI_BUF_SIZE 256

static volatile uint8_t DMA_Completed_Flag = 0;
static uint8_t* tempBuf = nullptr;
static uint8_t* tempBuf1 = nullptr;

using particle::__SPISettings;

static void querySpiInfo(HAL_SPI_Interface spi, hal_spi_info_t* info)
{
  memset(info, 0, sizeof(hal_spi_info_t));
  info->version = HAL_SPI_INFO_VERSION_1;
  HAL_SPI_Info(spi, info, nullptr);
}

static __SPISettings spiSettingsFromSpiInfo(hal_spi_info_t* info)
{
  if (!info->enabled || info->default_settings)
    return __SPISettings();
  return __SPISettings(info->clock, info->bit_order, info->data_mode);
}

static bool spiSettingsApplyCheck(SPIClass& spi, const __SPISettings& settings, HAL_SPI_Interface interface = HAL_SPI_INTERFACE1)
{
    hal_spi_info_t info;
    spi.beginTransaction(settings);
    querySpiInfo(interface, &info);
    auto current = spiSettingsFromSpiInfo(&info);
    spi.endTransaction();
    // Serial.print(settings);
    // Serial.print(" - ");
    // Serial.println(current);
    unsigned clock;
    uint8_t divider;

    SPI.computeClockDivider(info.system_clock, settings.getClock(), divider, clock);
    return (current == settings) || (current <= settings && clock == current.getClock());
}

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

test(SPI_00_Allocate_Buffers)
{
    tempBuf = (uint8_t*)malloc(SPI_BUF_SIZE);
    tempBuf1 = (uint8_t*)malloc(SPI_BUF_SIZE);

    assertTrue(tempBuf != nullptr && tempBuf1 != nullptr);
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

#if PLATFORM_ID != PLATFORM_SPARK_CORE
test(SPI_02_SPI_DMA_Transfers_Work_Correctly)
{
    assertTrue(tempBuf != nullptr && tempBuf1 != nullptr);
    SPI.begin();
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI.transfer(tempBuf, 0, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI.transfer(0, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI.transfer(tempBuf1, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    SPI.end();
}
#endif // PLATFORM_SPARK_CORE

#if Wiring_SPI1
test(SPI_03_SPI1_DMA_Transfers_Work_Correctly)
{
    assertTrue(tempBuf != nullptr && tempBuf1 != nullptr);
    SPI1.begin();
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI1.transfer(tempBuf, 0, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI1.transfer(0, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI1.transfer(tempBuf1, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    SPI1.end();
}
#endif

#if Wiring_SPI2
test(SPI_04_SPI2_DMA_Transfers_Work_Correctly)
{
    assertTrue(tempBuf != nullptr && tempBuf1 != nullptr);
    SPI2.begin();
    uint32_t m;

    DMA_Completed_Flag = 0;
    SPI2.transfer(tempBuf, 0, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI2.transfer(0, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }
    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    memset(tempBuf, 0xAA, SPI_BUF_SIZE);
    DMA_Completed_Flag = 0;
    SPI2.transfer(tempBuf1, tempBuf, SPI_BUF_SIZE, SPI_DMA_Completed_Callback);
    m = millis();
    while(!DMA_Completed_Flag) {
        assertLessOrEqual((millis() - m), 2000);
    }

    for (uint8_t* v = tempBuf; v < tempBuf + SPI_BUF_SIZE; v++) {
        assertNotEqual(*v, 0xAA);
    }

    SPI2.end();
}
#endif

#if PLATFORM_THREADING
test(SPI_05_SPI_Can_Be_Locked)
{
    SPI.begin();
    assertTrue(SPI.trylock());
    assertFalse(SPI.trylock());
    SPI.unlock();
    assertTrue(SPI.trylock());
    SPI.unlock();
    SPI.end();
}

#if Wiring_SPI2
test(SPI_06_SPI2_Can_Be_Locked)
{
    SPI2.begin();
    assertTrue(SPI2.trylock());
    assertFalse(SPI2.trylock());
    SPI2.unlock();
    assertTrue(SPI2.trylock());
    SPI2.unlock();
    SPI2.end();
}
#endif // Wiring_SPI2
#endif // PLATFORM_THREADING

test(SPI_07_SPI_Settings_Are_Applied_In_Begin_Transaction)
{
    // Just in case
    SPI.end();

    SPI.begin();

    // Get current SPISettings
    hal_spi_info_t info;
    __SPISettings current, settings;
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    settings = spiSettingsFromSpiInfo(&info);

    // Configure SPI with default settings
    SPI.beginTransaction(__SPISettings());
    querySpiInfo(HAL_SPI_INTERFACE1, &info);
    current = spiSettingsFromSpiInfo(&info);
    assertEqual(current, settings);
    SPI.endTransaction();

    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3)));

    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2)));
    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3)));

    assertTrue(spiSettingsApplyCheck(SPI, __SPISettings()));

    SPI.end();
}

#if Wiring_SPI1
test(SPI_08_SPI1_Settings_Are_Applied_In_Begin_Transaction)
{
    // Just in case
    SPI1.end();

    SPI1.begin();

    // Get current SPISettings
    hal_spi_info_t info;
    __SPISettings current, settings;

    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    settings = spiSettingsFromSpiInfo(&info);

    // Configure SPI with default settings
    SPI1.beginTransaction(__SPISettings());
    querySpiInfo(HAL_SPI_INTERFACE2, &info);
    current = spiSettingsFromSpiInfo(&info);
    assertEqual(current, settings);
    SPI1.endTransaction();

    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3), HAL_SPI_INTERFACE2));

    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2), HAL_SPI_INTERFACE2));
    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3), HAL_SPI_INTERFACE2));

    assertTrue(spiSettingsApplyCheck(SPI1, __SPISettings(), HAL_SPI_INTERFACE2));

    SPI1.end();
}
#endif // Wiring_SPI1

#if Wiring_SPI2
test(SPI_09_SPI2_Settings_Are_Applied_In_Begin_Transaction)
{
    // Just in case
    SPI2.end();

    SPI2.begin();

    // Get current SPISettings
    hal_spi_info_t info;
    __SPISettings current, settings;

    querySpiInfo(HAL_SPI_INTERFACE3, &info);
    settings = spiSettingsFromSpiInfo(&info);

    // Configure SPI with default settings
    SPI2.beginTransaction(__SPISettings());
    querySpiInfo(HAL_SPI_INTERFACE3, &info);
    current = spiSettingsFromSpiInfo(&info);
    assertEqual(current, settings);
    SPI2.endTransaction();

    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3), HAL_SPI_INTERFACE3));

    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE0), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE1), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(15*MHZ, MSBFIRST, SPI_MODE2), HAL_SPI_INTERFACE3));
    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(10*MHZ, LSBFIRST, SPI_MODE3), HAL_SPI_INTERFACE3));

    assertTrue(spiSettingsApplyCheck(SPI2, __SPISettings(), HAL_SPI_INTERFACE3));

    SPI2.end();
}
#endif // Wiring_SPI2

#if PLATFORM_THREADING
test(SPI_10_SPI_Begin_Transaction_Locks)
{
    // Just in case
    SPI.end();

    SPI.begin();
    assertTrue(SPI.trylock());
    SPI.unlock();
    SPI.beginTransaction(__SPISettings());
    assertFalse(SPI.trylock());
    SPI.endTransaction();
    assertTrue(SPI.trylock());
    SPI.unlock();
    SPI.end();
}

#if Wiring_SPI1
test(SPI_11_SPI1_Begin_Transaction_Locks)
{
    // Just in case
    SPI1.end();

    SPI1.begin();
    assertTrue(SPI1.trylock());
    SPI1.unlock();
    SPI1.beginTransaction(__SPISettings());
    assertFalse(SPI1.trylock());
    SPI1.endTransaction();
    assertTrue(SPI1.trylock());
    SPI1.unlock();
    SPI1.end();
}
#endif // Wiring_SPI1

#if Wiring_SPI2
test(SPI_12_SPI2_Begin_Transaction_Locks)
{
    // Just in case
    SPI2.end();

    SPI2.begin();
    assertTrue(SPI2.trylock());
    SPI2.unlock();
    SPI2.beginTransaction(__SPISettings());
    assertFalse(SPI2.trylock());
    SPI2.endTransaction();
    assertTrue(SPI2.trylock());
    SPI2.unlock();
    SPI2.end();
}
#endif // Wiring_SPI1

test(SPI_13_Free_Buffers)
{
    if (tempBuf != nullptr) {
        free(tempBuf);
    }

    if (tempBuf1 != nullptr) {
        free(tempBuf1);
    }

    tempBuf = tempBuf1 = nullptr;
}

#endif // PLATFORM_THREADING
