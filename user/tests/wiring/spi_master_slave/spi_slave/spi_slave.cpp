#include "application.h"
#include "unit-test/unit-test.h"

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

static uint8_t SPI_Slave_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Slave_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;
static volatile uint8_t SPI_Selected_State = 0;

static void SPI_On_Select(uint8_t state)
{
    if (state)
        SPI_Selected_State = state;
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static inline void SPI_Transfer_DMA(uint8_t *tx, uint8_t *rx, int length, HAL_SPI_DMA_UserCallback cb)
{
    while (true)
    {
        DMA_Completed_Flag = cb ? 0 : 1;
        SPI.transfer(tx, rx, length, cb);
        while(DMA_Completed_Flag == 0);
        if (SPI.available() == 0)
        {
            /* 0 length transfer means that we got deselected by master and nothing got clocked in. Try again? */
            continue;
        }
        break;
    }
}

test(SPI_Master_Slave_Slave_Transfer)
{
    /* Test will alternate between asynchronous and synchronous SPI.transfer() */
    Serial.println("This is Slave");
    SPI.begin(SPI_MODE_SLAVE, A2);
    SPI.onSelect(SPI_On_Select);

    uint32_t requestedLength = 0;

    uint32_t count = 0;

    while (true)
    {
        memset(SPI_Slave_Tx_Buffer, 0, sizeof(SPI_Slave_Tx_Buffer));
        memset(SPI_Slave_Rx_Buffer, 0, sizeof(SPI_Slave_Rx_Buffer));

        /* Preload SLAVE_TEST_MESSAGE_1 reply */
        memcpy(SPI_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1));

        /* Wait for Master to select us */
        while(SPI_Selected_State == 0);
        SPI_Selected_State = 0;

        /* Receive up to TRANSFER_LENGTH_2 bytes */
        SPI_Transfer_DMA(SPI_Slave_Tx_Buffer, SPI_Slave_Rx_Buffer, TRANSFER_LENGTH_2, count % 2 == 0 ? &SPI_DMA_Completed_Callback : NULL);
        /* Check how many bytes we have received */
        assertEqual(SPI.available(), TRANSFER_LENGTH_1);

        // Serial.print("< ");
        // Serial.println((const char *)SPI_Slave_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Slave_Rx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE)) == 0);

        requestedLength = *((uint32_t *)(SPI_Slave_Rx_Buffer + sizeof(MASTER_TEST_MESSAGE)));
        assertTrue((requestedLength >= 0 && requestedLength <= TRANSFER_LENGTH_2));

        if (requestedLength == 0)
            break;

        memcpy(SPI_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength);
        SPI_Slave_Tx_Buffer[requestedLength] = '\0';
        // Serial.print("> ");
        // Serial.println((const char *)SPI_Slave_Tx_Buffer);

        SPI_Transfer_DMA(SPI_Slave_Tx_Buffer, SPI_Slave_Rx_Buffer, requestedLength, count % 2 == 0 ? &SPI_DMA_Completed_Callback : NULL);
        /* Check that we have transferred requestedLength bytes as requested */
        assertEqual(SPI.available(), requestedLength);

        /* Just a sanity check that we received only 0xff */
        for (int i = 0; i < requestedLength; i++)
        {
            assertEqual(SPI_Slave_Rx_Buffer[i], 0xff);
        }

        count++;
    }
}
