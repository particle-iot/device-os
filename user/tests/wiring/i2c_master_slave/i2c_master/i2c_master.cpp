#include "application.h"
#include "unit-test/unit-test.h"

#define MASTER_TEST_MESSAGE "Hello from I2C Master!?"
//#define SLAVE_TEST_MESSAGE_1  "I2C Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define I2C_DELAY 0
#define I2C_ADDRESS 0x32

static uint8_t I2C_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t I2C_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static int I2C_Test_Counter = 2;

static void I2C_Master_Configure()
{
    USE_WIRE.setSpeed(400000);
    USE_WIRE.begin();
}

test(I2C_Master_Slave_Master_Variable_Length_Transfer)
{
    if (I2C_Test_Counter == 2)
        Serial.println("This is Master");
    I2C_Test_Counter--;
    uint32_t requestedLength = I2C_BUFFER_LENGTH;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0 && I2C_Test_Counter != 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Master_Tx_Buffer, 0, sizeof(I2C_Master_Tx_Buffer));
        memset(I2C_Master_Rx_Buffer, 0, sizeof(I2C_Master_Rx_Buffer));

        // delay(I2C_DELAY);

        memcpy(I2C_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Master_Tx_Buffer, TRANSFER_LENGTH_1);
        
        // End with STOP
        USE_WIRE.endTransmission(true);
        // delay(I2C_DELAY);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Master_Rx_Buffer, 0, sizeof(I2C_Master_Rx_Buffer));
        USE_WIRE.requestFrom(I2C_ADDRESS, requestedLength);
        assertEqual(requestedLength, USE_WIRE.available());

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Master_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);

        requestedLength--;
    }

    USE_WIRE.end();
}

test(I2C_Master_Slave_Master_Variable_Length_Transfer_Slave_Tx_Buffer_Underflow)
{
    /* This test requests the slave to prepare N bytes for transmission, but the actual
     * number of bytes clocked by the Master is N + 1. This will cause tx buffer underrun on the Slave.
     * When clock stretching is enabled, the I2C peripheral in Slave mode by default will continue pulling SCL low,
     * until the data is loaded into the DR register. This might cause the bus to enter an unrecoverable state.
     */
    if (I2C_Test_Counter == 2)
        Serial.println("This is Master");
    I2C_Test_Counter--;
    uint32_t requestedLength = I2C_BUFFER_LENGTH - 1;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0 && I2C_Test_Counter != 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Master_Tx_Buffer, 0, sizeof(I2C_Master_Tx_Buffer));
        memset(I2C_Master_Rx_Buffer, 0, sizeof(I2C_Master_Rx_Buffer));

        // delay(I2C_DELAY);

        memcpy(I2C_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Master_Tx_Buffer, TRANSFER_LENGTH_1);
        
        // End with STOP
        USE_WIRE.endTransmission(true);
        // delay(I2C_DELAY);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Master_Rx_Buffer, 0, sizeof(I2C_Master_Rx_Buffer));
        USE_WIRE.requestFrom(I2C_ADDRESS, requestedLength + (requestedLength & 0x01));
        if (USE_WIRE.available() != 0) {
            assertEqual(requestedLength + (requestedLength & 0x01), USE_WIRE.available());

            uint32_t count = 0;
            while(USE_WIRE.available()) {
                I2C_Master_Rx_Buffer[count++] = USE_WIRE.read();
            }
            // Serial.print("< ");
            // Serial.println((const char *)I2C_Master_Rx_Buffer);
            assertTrue(strncmp((const char *)I2C_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);
        } else if (requestedLength & 0x01) {
            Serial.println("Error reading from Slave, checking if we can recover");
        } else {
            Serial.println("Failed to recover");
            assertTrue(false);
        }

        requestedLength--;
    }

    USE_WIRE.end();
}
