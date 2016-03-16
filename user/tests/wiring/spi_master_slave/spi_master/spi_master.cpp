#include "application.h"
#include "unit-test/unit-test.h"

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define SPI_DELAY 1

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;
static int SPI_Test_Counter = 2;

static void SPI_Master_Configure()
{
    SPI.begin(A2);
}

test(SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA)
{
    if (SPI_Test_Counter == 2)
        Serial.println("This is Master");
    SPI_Test_Counter--;
    uint32_t requestedLength = TRANSFER_LENGTH_2;
    SPI_Master_Configure();

    while (requestedLength >= 0)
    {
        memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));

        // Select
        digitalWrite(A2, LOW);
        delay(SPI_DELAY);

        memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        uint32_t count = 0;
        while (count < TRANSFER_LENGTH_1)
        {
            SPI_Master_Rx_Buffer[count] = SPI.transfer(SPI_Master_Tx_Buffer[count]);
            count++;
        }
        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        digitalWrite(A2, HIGH);
        if (requestedLength == 0)
            break;
        delay(SPI_DELAY);
        digitalWrite(A2, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        count = 0;
        while (count < requestedLength)
        {
            SPI_Master_Rx_Buffer[count++] = SPI.transfer(0xff);
        }
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

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

test(SPI_Master_Slave_Master_Variable_Length_Transfer_DMA)
{
    if (SPI_Test_Counter == 2)
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

        DMA_Completed_Flag = 0;
        SPI.transfer(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1, SPI_DMA_Completed_Callback);
        while(DMA_Completed_Flag == 0);
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
        DMA_Completed_Flag = 0;
        SPI.transfer(NULL, SPI_Master_Rx_Buffer, requestedLength, SPI_DMA_Completed_Callback);
        while(DMA_Completed_Flag == 0);
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
