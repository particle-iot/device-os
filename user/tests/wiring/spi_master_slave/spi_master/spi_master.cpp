#include "application.h"
#include "unit-test/unit-test.h"
#include <functional>

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define SPI_DELAY 5

#ifndef USE_SPI
#error Define USE_SPI
#endif // #ifndef USE_SPI

#ifndef USE_CS
#error Define USE_CS
#endif // #ifndef USE_CS


#if HAL_PLATFORM_RTL872X

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS SS
#pragma message "Compiling for SPI, MY_CS set to SS"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS SS1
#pragma message "Compiling for SPI1, MY_CS set to SS1"
#elif (USE_SPI == 2)
#error "SPI2 not supported for p2/TrackerM/MSoM"
#else
#error "Not supported for P2/TrackerM/MSoM"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#elif HAL_PLATFORM_NRF52840

#if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_B5SOM)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D8
#pragma message "Compiling for SPI, MY_CS set to D8"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for asom, bsom or b5som"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#elif (PLATFORM_ID == PLATFORM_TRACKER)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D0 // FIXME
#pragma message "Compiling for SPI, MY_CS set to D0"
#elif (USE_SPI == 1) || (USE_SPI == 2)
#error "SPI2 not supported for tracker"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#else // argon, boron

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D14
#pragma message "Compiling for SPI, MY_CS set to D14"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for argon or boron"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#endif // #if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_B5SOM)

#else

#error "Unsupported platform"

#endif // #if HAL_PLATFORM_RTL872X


#if defined(_SPI) && (USE_CS != 255)
#pragma message "Overriding default CS selection"
#undef MY_CS
#if (USE_CS == 0)
#define MY_CS A2
#pragma message "_CS pin set to A2"
#elif (USE_CS == 1)
#define MY_CS D5
#pragma message "_CS pin set to D5"
#elif (USE_CS == 2)
#define MY_CS D8
#pragma message "_CS pin set to D8"
#elif (USE_CS == 3)
#define MY_CS D14
#pragma message "_CS pin set to D14"
#elif (USE_CS == 4)
#define MY_CS C0
#pragma message "_CS pin set to C0"
#endif // (USE_CS == 0)
#endif // defined(_SPI) && (USE_CS != 255)

/*
 *
 * Gen 3 SoM Wiring diagrams
 *
 * SPI/SPI1                       SPI1/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D8  <-------> D5 CS       CS   D5 <---------> D5 CS
 * MISO D11 <-------> D4 MISO     MISO D4 <---------> D4 MISO
 * MOSI D12 <-------> D3 MOSI     MOSI D3 <---------> D3 MOSI
 * SCK  D13 <-------> D2 SCK      SCK  D2 <---------> D2 SCK
 *
 *********************************************************************************************
 *
 * Gen 3 Non-SoM Wiring diagrams
 *
 * SPI/SPI1                       SPI1/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D14 <-------> D5 CS       CS   D5 <---------> D5 CS
 * MISO D11 <-------> D4 MISO     MISO D4 <---------> D4 MISO
 * MOSI D12 <-------> D3 MOSI     MOSI D3 <---------> D3 MOSI
 * SCK  D13 <-------> D2 SCK      SCK  D2 <---------> D2 SCK
 *
 *********************************************************************************************
 *
 * P2 Wiring diagrams
 *
 * SPI1/SPI1                       SPI/SPI1 (SPI can't be used as slave)
 * Master: SPI1  (USE_SPI=SPI1)    Master: SPI (USE_SPI=SPI)
 * Slave:  SPI1  (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D5 <-------> D5 CS        CS   S3 <---------> D5 CS
 * MISO D3 <-------> D3 MISO      MISO S1 <---------> D3 MISO
 * MOSI D2 <-------> D2 MOSI      MOSI S0 <---------> D2 MOSI
 * SCK  D4 <-------> D4 SCK       SCK  S2 <---------> D4 SCK
 *
 *********************************************************************************************
 *
 * MSoM Wiring diagrams
 *
 * SPI/SPI                        SPI/SPI1 (SPI1 can't be used as slave)
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI  (USE_SPI=SPI)     Slave:  SPI (USE_SPI=SPI)
 *
 * Master             Slave       Master              Slave
 * CS   D8  <-------> D8  CS        CS   D3  <---------> D8  CS
 * MISO D13 <-------> D13 MISO      MISO D2  <---------> D13 MISO
 * MOSI D11 <-------> D11 MOSI      MOSI D10 <---------> D11 MOSI
 * SCK  D12 <-------> D12 SCK       SCK  D9  <---------> D12 SCK
 *
 *********************************************************************************************
 */

#ifdef MY_SPI

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer_Supper[1024];
static volatile uint8_t DMA_Completed_Flag = 0;

static const char* txString =
"urjlU1tW177HwJsR6TylreMKge225qyLaIizW5IhXHkWgTGpH2fZtm2Od20Ne3Q81fxfUl7zoFaF\
Z6smPzkpTGNSxGg7TCEiE2f19951tKxjFCB4Se86R4CaWW2YZF0mogirgsu2qRMGe4mC9QlJkCgXP\
bgSVV1mc2xsZcu4bj0pbmPIhxkuyAHe4cVK3gLpWEGTadtAn2k66rOFNBdfPaE0cUY3wwXlVQ9yDl\
OxexepQcC2WTrbUe4z85OSae9s8A6BwUCRBYYfEH01cnGCzYGCEOEm5jl4nJ3HqWckHI5K2NeWS4x\
EhkgMqG3RwfOTM85SQ7q7NLIhgprCTsBTzv2YpGgbAB7oSX0joGQHxfyndxIyCVIHknvEj1hynXtw\
uebA6i7JBFiGkk4AnRzk7v3dNjHt6weuYWtf6yj3aVzhbMaWFCR6HOKFc3i8XzBsnLTc4Uzft61a0\
qV8ZssHdHO7sbiojOmA37RkrNUFxX1aODUXWNEntkTylwvhxKpsAb6Lsopzve4ea2G17WpW62Z12x\
mNgTZQHOo3fCZDy8L7WfVwCJiJunHPXu9jw6g11NJFcpo2AakkZQDgUGZoeZgDB6GfRheAiurAEB5\
Ym4EVIQB9AvVBf4zY84R8D4bnfjwwLDwiZSo9y2Z5JsVQ0yRdqPdxv0cV2Kp0AaevITeubJseCXOg\
LkFiaeDTBoR7kyMyoJvJl4vjLmiV03RNSAl9JpZkBfTHzalZw8oaRHMMiTVVGdieJOIbANoaXyRbe\
xSYU1t5dOe8wxybwfBBlPIswpVJ45kXd4Bu8NCLXPAbgJCOVSlTQsfvzVKZykp9V1DBQ3PwyeBXJB\
QsLDslIOHOKbfqB8njXotpE3Dz46Wi6QtpinLsSiviZmz62qLW5Pd9M7SDCarrxFk8SBHyJl2KdjH\
5Lx1LmkW8gMiYquOctT9xhFNs406BxWrPcTc5kwaSJ6RJQyohQEJk9ojchrbSo4ucfZGQzEMBEIJs";

static void SPI_Master_Configure()
{
    if (!MY_SPI.isEnabled()) {
        // Run SPI bus at 1 MHz, so that this test works reliably even if the
        // devices are connected with jumper wires
        MY_SPI.setClockSpeed(1, MHZ);
        MY_SPI.begin(MY_CS);

        // Clock dummy byte just in case
        if (!HAL_PLATFORM_RTL872X) { // Since SPI is enabled, the dummy byte will generate gabage data in the fifo on P2
            (void)MY_SPI.transfer(0xff);
        }
    }
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static void SPI_Master_Transfer_No_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length) {
    for(int count = 0; count < length; count++)
    {
        uint8_t tmp = MY_SPI.transfer(txbuf ? txbuf[count] : 0xff);
        if (rxbuf)
            rxbuf[count] = tmp;
    }
}

static void SPI_Master_Transfer_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length, hal_spi_dma_user_callback cb) {
    if (cb) {
        DMA_Completed_Flag = 0;
        MY_SPI.transfer(txbuf, rxbuf, length, cb);
        while(DMA_Completed_Flag == 0);
        assertEqual(length, MY_SPI.available());
    } else {
        MY_SPI.transfer(txbuf, rxbuf, length, NULL);
        assertEqual(length, MY_SPI.available());
    }
}

bool SPI_Master_Slave_Change_Mode(uint8_t mode, uint8_t bitOrder, std::function<void(uint8_t*, uint8_t*, int)> transferFunc) {
    // Use previous settings
    SPI_Master_Configure();

    uint32_t requestedLength = 0;
    uint32_t switchMode = ((uint32_t)mode) | ((uint32_t)bitOrder << 8) | (0x8DC6 << 16);
    memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));

    memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));
    memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));

    // Select
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);

    memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t),
           (void*)&switchMode, sizeof(uint32_t));

    transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);

    digitalWrite(MY_CS, HIGH);
    delay(SPI_DELAY * 10);

    bool ret = (strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

    // Apply new settings here
    MY_SPI.setDataMode(mode);
    MY_SPI.setBitOrder(bitOrder);
    SPI_Master_Configure();

    return ret;
}

void SPI_Master_Slave_Master_Test_Routine(std::function<void(uint8_t*, uint8_t*, int)> transferFunc, bool end = false) {
    uint32_t requestedLength = TRANSFER_LENGTH_2;
    SPI_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0 && !end)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));

        int ret = hal_spi_sleep(MY_SPI.interface(), false, nullptr);
        // assertEqual(ret, (int)SYSTEM_ERROR_NONE); // Device may be awake

        // Select
        // Workaround for some platforms requiring the CS to be high when configuring
        // the DMA buffers
        digitalWrite(MY_CS, LOW);
        delay(SPI_DELAY);
        digitalWrite(MY_CS, HIGH);
        delay(SPI_DELAY);
        digitalWrite(MY_CS, LOW);
        delay(SPI_DELAY);

        memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);
        digitalWrite(MY_CS, HIGH);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        delay(SPI_DELAY);
        if (requestedLength == 0)
            break;

        digitalWrite(MY_CS, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        transferFunc(NULL, SPI_Master_Rx_Buffer, requestedLength);
        // Deselect
        digitalWrite(MY_CS, HIGH);
        delay(SPI_DELAY);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);

        requestedLength--;

        ret = hal_spi_sleep(MY_SPI.interface(), true, nullptr);
        assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    }
}

/*
 * Default mode: SPI_MODE3, MSBFIRST
 */
test(00_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_Default_MODE3_MSB)
{
    Serial.println("This is Master");
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(01_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Default_MODE3_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(02_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_Default_MODE3_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE3, LSBFIRST
 */
test(03_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE3_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(04_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE3_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(05_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE3_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE0, MSBFIRST
 */
test(06_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE0_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(07_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE0_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(08_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE0_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE0, LSBFIRST
 */
test(09_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE0_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(10_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE0_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(11_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE0_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE1, MSBFIRST
 */
test(12_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE1_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(13_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE1_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(14_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE1_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE1, LSBFIRST
 */
test(15_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE1_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(16_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE1_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(17_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE1_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE2, MSBFIRST
 */
test(18_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE2_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(19_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE2_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(20_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE2_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE2, LSBFIRST
 */
test(21_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE2_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(22_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE2_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(23_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE2_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc, true);
}

// The following tests inherit the last mode and bit order being set above
test(24_SPI_Master_Slave_Master_Const_String_Transfer_DMA)
{
    // Select
    // Workaround for some platforms requiring the CS to be high when configuring
    // the DMA buffers
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);
    digitalWrite(MY_CS, HIGH);
    delay(SPI_DELAY);
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);

    // Gen3: Send a big chunk of const data to verify truncated transfer
    DMA_Completed_Flag = 0;

    // FIXME: The rx_buffer has to be nullptr, otherwise, the the rx_buffer will overflow
    // as the length of the rx buffer given in this file is not equal to the length of tx data
    MY_SPI.transfer(txString, nullptr, strlen(txString), SPI_DMA_Completed_Callback);
    while(DMA_Completed_Flag == 0);

    digitalWrite(MY_CS, HIGH);
    assertEqual(MY_SPI.available(), strlen(txString));
}

test(25_SPI_Master_Slave_Master_Reception)
{
    memset(SPI_Master_Rx_Buffer_Supper, '\0', sizeof(SPI_Master_Rx_Buffer_Supper));

    // Select
    // Workaround for some platforms requiring the CS to be high when configuring
    // the DMA buffers
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);
    digitalWrite(MY_CS, HIGH);
    delay(SPI_DELAY);
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);

    DMA_Completed_Flag = 0;

    MY_SPI.transfer(nullptr, SPI_Master_Rx_Buffer_Supper, strlen(txString), SPI_DMA_Completed_Callback);
    while(DMA_Completed_Flag == 0);
    digitalWrite(MY_CS, HIGH);

    // Serial.printf("Length: %d\r\n", strlen((const char *)SPI_Master_Rx_Buffer_Supper));
    // for (size_t len = 0; len < strlen((const char *)SPI_Master_Rx_Buffer_Supper); len++) {
    //     Serial.printf("%c", SPI_Master_Rx_Buffer_Supper[len]);
    // }

    assertEqual(MY_SPI.available(), strlen(txString));
    assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer_Supper, txString, strlen(txString)) == 0);
}

#endif // #ifdef _SPI
