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
#endif

#ifndef USE_CS
#error Define USE_CS
#endif

/*
 * Wiring diagrams
 *
 * SPI/SPI                        SPI1/SPI                       SPI2/SPI
 * Master: SPI (USE_SPI=SPI)      Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI (USE_SPI=SPI)      Slave:  SPI  (USE_SPI=SPI)     Slave:  SPI  (USE_SPI=SPI)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> A5 MOSI    MOSI D2 <---------> A5 MOSI    MOSI C1 <---------> A5 MOSI
 * MISO A4 <---------> A4 MISO    MISO D3 <---------> A4 MISO    MISO C2 <---------> A4 MISO
 * SCK  A3 <---------> A3 SCK     SCK  D4 <---------> A3 SCK     SCK  C3 <---------> A3 SCK
 * CS   A2 <---------> A2 CS      CS   D5 <---------> A2 CS      CS   C0 <---------> A2 CS
 *
 *********************************************************************************************
 *
 * SPI/SPI1                       SPI1/SPI1                      SPI2/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> D2 MOSI    MOSI D2 <---------> D2 MOSI    MOSI C1 <---------> D2 MOSI
 * MISO A4 <---------> D3 MISO    MISO D3 <---------> D3 MISO    MISO C2 <---------> D3 MISO
 * SCK  A3 <---------> D4 SCK     SCK  D4 <---------> D4 SCK     SCK  C3 <---------> D4 SCK
 * CS   A2 <---------> D5 CS      CS   D5 <---------> D5 CS      CS   C0 <---------> D5 CS
 *
 *********************************************************************************************
 *
 * SPI/SPI2                       SPI1/SPI2                      SPI2/SPI2
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI2 (USE_SPI=SPI2     Slave:  SPI2 (USE_SPI=SPI2)    Slave:  SPI2 (USE_SPI=SPI2)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> C1 MOSI    MOSI D2 <---------> C1 MOSI    MOSI C1 <---------> C1 MOSI
 * MISO A4 <---------> C2 MISO    MISO D3 <---------> C2 MISO    MISO C2 <---------> C2 MISO
 * SCK  A3 <---------> C3 SCK     SCK  D4 <---------> C3 SCK     SCK  C3 <---------> C3 SCK
 * CS   A2 <---------> C0 CS      CS   D5 <---------> C0 CS      CS   C0 <---------> C0 CS
 *
 *********************************************************************************************
 */

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;

static void SPI_Master_Configure()
{
    if (!USE_SPI.isEnabled()) {
        // Run SPI bus at 1 MHz, so that this test works reliably even if the
        // devices are connected with jumper wires
        USE_SPI.setClockSpeed(1, MHZ);
        USE_SPI.begin(USE_CS);

        // Clock dummy byte just in case
        (void)USE_SPI.transfer(0xff);
    }
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static void SPI_Master_Transfer_No_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length) {
    for(int count = 0; count < length; count++)
    {
        uint8_t tmp = USE_SPI.transfer(txbuf ? txbuf[count] : 0xff);
        if (rxbuf)
            rxbuf[count] = tmp;
    }
}

static void SPI_Master_Transfer_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length, HAL_SPI_DMA_UserCallback cb) {
    if (cb) {
        DMA_Completed_Flag = 0;
        USE_SPI.transfer(txbuf, rxbuf, length, cb);
        while(DMA_Completed_Flag == 0);
        assertEqual(length, USE_SPI.available());
    } else {
        USE_SPI.transfer(txbuf, rxbuf, length, NULL);
        assertEqual(length, USE_SPI.available());
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
    digitalWrite(USE_CS, LOW);
    delay(SPI_DELAY);

    memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t),
           (void*)&switchMode, sizeof(uint32_t));

    transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);

    digitalWrite(USE_CS, HIGH);
    delay(SPI_DELAY * 10);

    bool ret = (strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

    // Apply new settings here
    USE_SPI.setDataMode(mode);
    USE_SPI.setBitOrder(bitOrder);
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

        // Select
        digitalWrite(USE_CS, LOW);
        delay(SPI_DELAY);

        memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);
        digitalWrite(USE_CS, HIGH);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        delay(SPI_DELAY);
        if (requestedLength == 0)
            break;

        digitalWrite(USE_CS, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        transferFunc(NULL, SPI_Master_Rx_Buffer, requestedLength);
        // Deselect
        digitalWrite(USE_CS, HIGH);
        delay(SPI_DELAY);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);

        requestedLength--;
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
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
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc, true);
}
