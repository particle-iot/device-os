#include "application.h"
#include "unit-test/unit-test.h"

#define MASTER_TEST_MESSAGE "Hello from I2C Master!?"
//#define SLAVE_TEST_MESSAGE_1  "I2C Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define I2C_ADDRESS 0x32

static uint8_t I2C_Slave_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t I2C_Slave_Rx_Buffer[TRANSFER_LENGTH_2];

static uint8_t done = 0;
static uint32_t requestedLength = 0;

static int random_range(int minVal, int maxVal)
{
    return rand() % (maxVal - minVal + 1) + minVal;
}

void I2C_Slave_On_Request_Callback(void) {
    // Random delay.
    // Just to be on a safe side delay between 0 and 50ms (I2C EVENT_TIMEOUT / 2)
    delayMicroseconds(random_range(0, 50000));
    memset(I2C_Slave_Tx_Buffer, 0, sizeof(I2C_Slave_Tx_Buffer));
    memcpy(I2C_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength);
    USE_WIRE.write((const uint8_t*)I2C_Slave_Tx_Buffer, requestedLength);   
}

void I2C_Slave_On_Receive_Callback(int) {
    assertEqual(USE_WIRE.available(), TRANSFER_LENGTH_1);
    int count = 0;
    while(USE_WIRE.available()) {
        I2C_Slave_Rx_Buffer[count++] = USE_WIRE.read();
    }
    I2C_Slave_Rx_Buffer[count] = 0x00;

    // Serial.print("< ");
    // Serial.println((const char *)I2C_Slave_Rx_Buffer);

    assertTrue(strncmp((const char *)I2C_Slave_Rx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE)) == 0);

    requestedLength = *((uint32_t *)(I2C_Slave_Rx_Buffer + sizeof(MASTER_TEST_MESSAGE)));
    assertTrue((requestedLength >= 0 && requestedLength <= TRANSFER_LENGTH_2));

    if (requestedLength == 0)
        done = 1;
}

test(I2C_Master_Slave_Slave_Transfer)
{
    Serial.println("This is Slave");
    Serial.blockOnOverrun(false);
    USE_WIRE.begin(I2C_ADDRESS);
    USE_WIRE.stretchClock(true);
    USE_WIRE.onRequest(I2C_Slave_On_Request_Callback);
    USE_WIRE.onReceive(I2C_Slave_On_Receive_Callback);

    while(done == 0) {
        delay(100);
    }

    USE_WIRE.end();
}
