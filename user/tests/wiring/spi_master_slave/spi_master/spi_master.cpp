#include "application.h"
#include "unit-test/unit-test.h"
#include <functional>

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define SPI_DELAY 1

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;
static int SPI_Test_Counter = 3;

static void SPI_Master_Configure()
{
    SPI.begin(A2);
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

void SPI_Master_Slave_Master_Test_Routine(std::function<void(uint8_t*, uint8_t*, int)> transferFunc) {
    if (SPI_Test_Counter == 3)
        Serial.println("This is Master");
    SPI_Test_Counter--;

    uint32_t requestedLength = TRANSFER_LENGTH_2;
    SPI_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0 && SPI_Test_Counter != 0)
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
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        digitalWrite(A2, HIGH);
        delay(SPI_DELAY);
        
        if (requestedLength == 0)
            break;

        digitalWrite(A2, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        transferFunc(NULL, SPI_Master_Rx_Buffer, requestedLength);
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);

        // Deselect
        digitalWrite(A2, HIGH);
        delay(SPI_DELAY);

        requestedLength--;
    }

    SPI.end();
}

test(SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA)
{
    SPI_Master_Slave_Master_Test_Routine(SPI_Master_Transfer_No_DMA);
}

test(SPI_Master_Slave_Master_Variable_Length_Transfer_DMA)
{
    using namespace std::placeholders;
    SPI_Master_Slave_Master_Test_Routine(std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback));
}

test(SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous)
{
    using namespace std::placeholders;
    SPI_Master_Slave_Master_Test_Routine(std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (HAL_SPI_DMA_UserCallback)NULL));
}
