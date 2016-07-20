#include "application.h"
#include "unit-test/unit-test.h"
#include <functional>

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define SPI_DELAY 5

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;

static void SPI_Master_Configure()
{
    if (!SPI.isEnabled()) {
        // Run SPI bus at 1 MHz, so that this test works reliably even if the
        // devices are connected with jumper wires
        SPI.setClockSpeed(1, MHZ);
        SPI.begin(A2);

        // Clock dummy byte just in case
        (void)SPI.transfer(0xff);
    }
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static void SPI_Master_Transfer_No_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length) {
    for(int count = 0; count < length; count++)
    {
        uint8_t tmp = SPI.transfer(txbuf ? txbuf[count] : 0xff);
        if (rxbuf)
            rxbuf[count] = tmp;
    }
}

static void SPI_Master_Transfer_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length, HAL_SPI_DMA_UserCallback cb) {
    if (cb) {
        DMA_Completed_Flag = 0;
        SPI.transfer(txbuf, rxbuf, length, cb);
        while(DMA_Completed_Flag == 0);
        assertEqual(length, SPI.available());
    } else {
        SPI.transfer(txbuf, rxbuf, length, NULL);
        assertEqual(length, SPI.available());
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
    digitalWrite(A2, LOW);
    delay(SPI_DELAY);

    memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t),
           (void*)&switchMode, sizeof(uint32_t));

    transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);

    digitalWrite(A2, HIGH);
    delay(SPI_DELAY * 10);

    bool ret = (strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

    // Apply new settings here
    SPI.setDataMode(mode);
    SPI.setBitOrder(bitOrder);
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
        digitalWrite(A2, LOW);
        delay(SPI_DELAY);

        memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);
        digitalWrite(A2, HIGH);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        delay(SPI_DELAY);
        if (requestedLength == 0)
            break;

        digitalWrite(A2, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        transferFunc(NULL, SPI_Master_Rx_Buffer, requestedLength);
        // Deselect
        digitalWrite(A2, HIGH);
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
